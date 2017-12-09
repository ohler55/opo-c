// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "opo.h"
#include "internal.h"
#include "client.h"

#define MIN_SLEEP	(1.0 / (double)CLOCKS_PER_SEC)
// lower gives faster response but burns more CPU. This is a reasonable compromise.
#define RETRY_SECS	0.0001
#define NOT_WAITING	0
#define WAITING		1
#define NOTIFIED	2

typedef enum {
    Q_CLEAR	= 0,
    Q_SENT	= 's',
    Q_READY	= 'r',
    Q_TAKEN	= 'd',
} QueryState;

typedef struct _Query {
    uint64_t		id;
    opoQueryCallback	cb;
    void		*ctx;
    double		when;
    atomic_char		state;
    opoMsg		resp;
} *Query;

struct _opoClient {
    volatile bool	active;
    int			sock;
    double		timeout;
    pthread_t		recv_thread;
    atomic_ullong	next_id;
    opoStatusCallback	status_callback;

    Query		q;
    Query		end;
    size_t		pending_max;
    volatile Query	head;
    volatile Query	tail;
    Query		on_deck;

    atomic_flag		head_lock;
    atomic_flag		tail_lock;
    atomic_flag		take_lock;

    atomic_int		waiting;
    int			rsock;
    int			wsock;

    opoDict		dict;
};

static void
query_set_state(Query q, QueryState state) {
    atomic_store(&q->state, state);
}

static double
dtime() {
    struct timeval	tv;

    gettimeofday(&tv, NULL);

    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

static double
dsleep(double t) {
    struct timespec	req, rem;

    if (MIN_SLEEP > t) {
	t = MIN_SLEEP;
    }
    req.tv_sec = (time_t)t;
    req.tv_nsec = (long)(1000000000.0 * (t - (double)req.tv_sec));
    if (nanosleep(&req, &rem) == -1 && EINTR == errno) {
	return (double)rem.tv_sec + (double)rem.tv_nsec / 1000000000.0;
    }
    return 0.0;
}

static void
status_callback(opoClient client, bool connected, opoErrCode code, const char *fmt, ...) {
    char	buf[256];
    va_list	ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    client->status_callback(client, connected, code, buf);
    va_end(ap);
}

static void
wait_for_ready(opoClient client, double timeout) {
    if (0 == client->rsock) {
	dsleep(timeout);
	return;
    }
    atomic_fetch_add(&client->waiting, 1);

    struct pollfd	pa;
    
    pa.fd = client->rsock;
    pa.events = POLLIN;
    pa.revents = 0;
    if (1 == poll(&pa, 1, (int)(timeout / 1000.0))) {
	if (0 != (pa.revents & POLLIN)) {
	    char	buf[8];
	    
	    while (0 < read(client->rsock, buf, sizeof(buf))) {
	    }
	}
    }
    atomic_fetch_sub(&client->waiting, 1);
}

static void
wake_ready(opoClient client) {
    if (0 == client->wsock || 0 >= atomic_load(&client->waiting)) {
	return;
    }
    if (write(client->wsock, ".", 1)) {}
}

static Query
take_next_ready(opoClient client, double timeout) {
    while (atomic_flag_test_and_set(&client->head_lock)) {
	dsleep(RETRY_SECS);
    }
    if (Q_READY != atomic_load(&client->head->state)) {
	if (0.0 < timeout) {
	    double	give_up = dtime() + timeout;

	    while (Q_READY != atomic_load(&client->head->state)) {
		if (give_up < dtime()) {
		    atomic_flag_clear(&client->head_lock);
		    return NULL;
		}
		wait_for_ready(client, 0.01);
	    }
	} else {
	    atomic_flag_clear(&client->head_lock);
	    return NULL;
	}
    }
    Query	q = client->head;
    
    client->head++;
    if (client->end <= client->head) {
	client->head = client->q;
    }
    atomic_flag_clear(&client->head_lock);

    query_set_state(q, Q_TAKEN);

    return q;
}

// Returns addrinfo for a host and port.
static struct addrinfo*
get_addr_info(opoErr err, const char *host, int port) {
    struct addrinfo	hints;
    struct addrinfo	*res;
    char		sport[32];
    int			stat;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    sprintf(sport, "%d", port);
    if (0 != (stat = getaddrinfo(host, sport, &hints, &res))) {
	opo_err_no(err, "Failed to resolve %s:%d.", host, port);
	return NULL;
    }
    return res;
}

// Find the pending query with the id by starting with on_dec and work towards
// the tail.
static Query
pending_find(opoClient client, uint64_t id) {
    Query	q = client->on_deck;
    bool	looped = false;

    do {
	if (q->id == id) {
	    return q;
	}
	q++;
	if (client->end <= q) {
	    if (looped) { // safety check in case the tail was missed
		break;
	    }
	    q = client->q;
	    looped = true;
	}
    } while (q != client->tail);

    return NULL;
}

static void
processs_msg(opoClient client, opoMsg msg) {
    uint64_t	id = opo_msg_id(msg);
    Query	q = client->on_deck;;

    if (id != client->on_deck->id) {
	if (NULL == (q = pending_find(client, id))) {
	    status_callback(client, true, OPO_ERR_NOT_FOUND, "Pending query %llu not found.", (unsigned long long)id);
	    free((uint8_t*)msg);
	    return;
	}
	while (q != client->on_deck) {
	    if (NULL != client->on_deck->cb) {
		uint8_t			lost_msg[1024];
		struct _opoBuilder	b;
		struct _opoErr		err = OPO_ERR_INIT;
	    
		opo_builder_init(&err, &b, lost_msg, sizeof(lost_msg));
		opo_builder_push_object(&err, &b, NULL, 0);
		opo_builder_push_int(&err, &b, OPO_ERR_LOST, "code", 4);
		opo_builder_push_string(&err, &b, "no response", -1, "error", 5);
		opo_builder_finish(&b);
		opo_msg_set_id(b.head, id);
		client->on_deck->resp = opo_builder_take(&b);
		query_set_state(client->on_deck, Q_READY);
	    }
	    client->on_deck++;
	    if (client->end <= client->on_deck) {
		client->on_deck = client->q;
	    }
	}
    }
    if (Q_SENT != atomic_load(&q->state)) {
	status_callback(client, true, OPO_ERR_TOO_MANY, "Duplicate response to query %llu.", (unsigned long long)id);
	free((uint8_t*)msg);
	return;
    }
    q->resp = msg;
    query_set_state(q, Q_READY);
    client->on_deck++;
    if (client->end <= client->on_deck) {
	client->on_deck = client->q;
    }
    wake_ready(client);
}

void*
recv_loop(void *ctx) {
    opoClient		client = (opoClient)ctx;
    struct pollfd	pa[1];
    int			i;
    uint8_t		buf[4096];
    uint8_t		*msg = NULL;
    ssize_t		msize = 0;
    ssize_t		mcnt = 0;
    ssize_t		bcnt = 0;
    ssize_t		cnt;
    size_t		size;

    while (client->active) {
	pa->fd = client->sock;
	pa->events = POLLIN;
	pa->revents = 0;
	if (0 > (i = poll(pa, 1, 100))) {
	    if (EAGAIN == errno) {
		continue;
	    }
	    // Either a signal or something bad like out of memory. Might as well exit.
	    if (NULL != client->status_callback) {
		client->status_callback(client, false, errno, "connection polling error");
	    }
	    break;
	}
	if (0 == i) { // nothing to read or write
	    continue;
	}
	if (0 != (pa->revents & POLLIN)) {
	    if (NULL == msg) {
		if (0 > (cnt = recv(client->sock, buf + bcnt, sizeof(buf) - bcnt, 0))) {
		    if (client->active && NULL != client->status_callback) {
			client->status_callback(client, false, errno, "read failed");
		    }
		} else if (0 == cnt) {
		    if (client->active && 0 < client->sock && NULL != client->status_callback) {
			client->status_callback(client, false, OPO_ERR_READ, "connection closed");
		    }
		    close(client->sock);
		    client->sock = 0;
		} else { // read something
		    bcnt += cnt;
		    while (13 <= bcnt) { // need 8 bytes for id, 1 for type, and 4 for length of val
			size = opo_msg_bsize((opoMsg)buf);
			if (NULL == (msg = (uint8_t*)malloc(size))) {
			    if (NULL != client->status_callback) {
				char	err_msg[256];

				sprintf(err_msg, "failed to allocate memory for message of size %lu.", (unsigned long)size);
				client->status_callback(client, true, OPO_ERR_MEMORY, err_msg);
			    }
			    break;
			}
			if (size <= bcnt) {
			    memcpy(msg, buf, size);
			    if (size < bcnt) {
				memmove(buf, buf + size, bcnt - size);
			    }
			    processs_msg(client, msg);
			    msg = NULL;
			    msize = 0;
			    mcnt = 0;
			    bcnt -= size;
			} else {
			    memcpy(msg, buf, bcnt);
			    msize = size;
			    mcnt = bcnt;
			    bcnt = 0;
			}
		    }
		}
	    } else { // msg has been allocated
		if (0 > (cnt = recv(client->sock, msg + mcnt, msize - mcnt, 0))) {
		    if (NULL != client->status_callback) {
			char	err_msg[256];

			sprintf(err_msg, "read failed. %s.", strerror(errno));
			client->status_callback(client, false, errno, err_msg);
		    }
		    close(client->sock);
		    client->sock = 0;
		} else {
		    mcnt += cnt;
		    if (msize == mcnt) {
			processs_msg(client, msg);
			msg = NULL;
			msize = 0;
			mcnt = 0;
		    }
		}
	    }
	}
	if (0 != (pa->revents & (POLLERR | POLLHUP | POLLNVAL))) {
	    if (0 == bcnt && NULL == msg) {
		if (client->active && NULL != client->status_callback) {
		    client->status_callback(client, false, OPO_ERR_OK, "connection closed");
		}
		client->sock = 0;
	    } else if (0 != (pa->revents & (POLLHUP | POLLNVAL))) {
		if (NULL != client->status_callback) {
		    client->status_callback(client, false, OPO_ERR_READ, "closed with outstanding responses");
		}
		client->sock = 0;
	    } else {
		if (NULL != client->status_callback) {
		    char	err_msg[256];

		    sprintf(err_msg, "socket error. %s.", strerror(errno));
		    client->status_callback(client, false, errno, err_msg);
		}
		close(client->sock);
		client->sock = 0;
	    }
	}
    }
    return NULL;
}

static opoDict
read_dict(opoErr err, int sock) {
    uint8_t	buf[1024];
    int		cnt;
    int		bcnt = 0;

    for (int i = 1000; 0 < i; i--) {
	if (0 > (cnt = recv(sock, buf + bcnt, sizeof(buf) - bcnt, 0))) {
	    if (EAGAIN == errno) {
		usleep(1000);
		continue;
	    } else {
		opo_err_set(err, OPO_ERR_READ, "failed to establish connection to server. %s", strerror(errno));
		return NULL;
	    }
	} else {
	    bcnt += cnt;
	    if (opo_msg_size_ok(buf, bcnt)) {
		break;
	    }
	}
    }
    size_t	size = opo_msg_bsize((opoMsg)buf);
    uint8_t	*msg;
    
    if (NULL == (msg = (uint8_t*)malloc(size))) {
	opo_err_set(err, OPO_ERR_READ, "failed to establish connection to server");
	return NULL;
    }
    memcpy(msg, buf, bcnt);
    while (bcnt < size) {
	if (0 > (cnt = recv(sock, msg + bcnt, size - bcnt, 0))) {
	    if (EAGAIN == errno) {
		usleep(100);
		continue;
	    } else {
		opo_err_set(err, OPO_ERR_READ, "failed to read dictionary from server. %s", strerror(errno));
		return NULL;
	    }
	} else {
	    bcnt += cnt;
	}
    }
    const char	*words[256];
    const char	**wp = words;
    opoVal	m = opo_val_members(err, opo_msg_val(msg));
    uint8_t	*end = msg + size;

    for (; m < end; m = opo_val_next(m)) {
	if (NULL != (*wp = opo_val_string(err, m, NULL, NULL))) {
	    wp++;
	}
    }
    *wp = NULL;

    return opo_dict_create(err, words);
}

opoClient
opo_client_connect(opoErr err, const char *host, int port, opoClientOptions options) {
    struct addrinfo	*res = get_addr_info(err, host, port);
    int			sock;
    int			optval = 1;

    if (NULL == res) {
	return NULL;
    }
    if (0 > (sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol))) {
	opo_err_no(err, "error creating socket to %s:%d", host, port);
	return NULL;
    }
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
    //setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    fcntl(sock, F_SETFL, O_NONBLOCK);

    double	giveup = dtime() + 2.0;

    while (0 > connect(sock, res->ai_addr, res->ai_addrlen)) {
	if (EISCONN == errno) {
	    break;
	}
	if (giveup < dtime()) {
	    opo_err_no(err, "error connecting socket to %s:%d", host, port);
	    close(sock);
	    return NULL;
	}
	usleep(1000);
    }
    opoClient	client = (opoClient)malloc(sizeof(struct _opoClient));

    if (NULL == client) {
	opo_err_set(err, OPO_ERR_MEMORY, "failed to allocate memory for a opoClient.");
    } else {
	int	stat;
	int	pending_max = 4096;
	
	client->sock = sock;
	client->dict = NULL;
	if (NULL == options) {
	    client->timeout = 2.0;
	    client->status_callback = NULL;
	} else {
	    client->timeout = options->timeout;
	    client->status_callback = options->status_callback;
	    pending_max = options->pending_max;
	    if (pending_max < 1) {
		pending_max = 1;
	    }
	}
	client->q = (Query)malloc(sizeof(struct _Query) * pending_max);
	client->end = client->q + pending_max;
	client->pending_max = pending_max;
	memset(client->q, 0, sizeof(struct _Query) * pending_max);
	client->head = client->q;
	client->tail = client->q;
	client->on_deck = client->q;

	atomic_init(&client->next_id, 1);
	atomic_flag_clear(&client->head_lock);
	atomic_flag_clear(&client->tail_lock);
	atomic_flag_clear(&client->take_lock);
	atomic_init(&client->waiting, 0);

	int	fd[2];

	if (0 == pipe(fd)) {
	    fcntl(fd[0], F_SETFL, O_NONBLOCK);
	    fcntl(fd[1], F_SETFL, O_NONBLOCK);
	    client->rsock = fd[0];
	    client->wsock = fd[1];
	}
	client->active = true; // outside the thread create to avoid race condition on immediate close
	if (0 != (stat = pthread_create(&client->recv_thread, NULL, recv_loop, client))) {
	    client->active = false;
	    opo_err_set(err, stat, "failed create receiving thread. %s", strerror(stat));
	    opo_client_close(client);
	    return NULL;
	}
	if (0 < client->sock && NULL != client->status_callback) {
	    status_callback(client, true, OPO_ERR_OK, "connected to %s:%d", host, port);
	}
	if (NULL == (client->dict = read_dict(err, client->sock))) {
	    opo_client_close(client);
	    return NULL;
	}
    }
    return client;
}

void
opo_client_close(opoClient client) {
    client->active = false;
    if (0 < client->sock) {
	close(client->sock);
    }
    if (NULL != client->status_callback) {
	client->status_callback(client, false, OPO_ERR_OK, "connection closed");
    }
    pthread_join(client->recv_thread, NULL);
    free(client->q);
    client->q = NULL;
    if (0 < client->wsock) {
	close(client->wsock);
    }
    if (0 < client->rsock) {
	close(client->rsock);
    }
    if (NULL != client->dict) {
	opo_dict_destroy(client->dict);
    }
    free(client);
}

opoRef
opo_client_query(opoErr err, opoClient client, opoMsg query, opoQueryCallback cb, void *ctx) {
    size_t	size = opo_msg_bsize(query);

    while (atomic_flag_test_and_set(&client->tail_lock)) {
	dsleep(RETRY_SECS);
    }
    if (Q_CLEAR != atomic_load(&client->tail->state)) {
	if (0.0 < client->timeout) {
	    double	give_up = dtime() + client->timeout;

	    while (Q_CLEAR != atomic_load(&client->tail->state)) {
		if (give_up < dtime()) {
		    atomic_flag_clear(&client->tail_lock);
		    opo_err_set(err, EAGAIN, "write failed, busy");
		    return 0;
		}
		dsleep(RETRY_SECS);
	    }
	} else {
	    atomic_flag_clear(&client->tail_lock);
	    opo_err_set(err, EAGAIN, "write failed, busy");
	    return 0;
	}
    }
    Query	q = client->tail;

    q->id = atomic_fetch_add(&client->next_id, 1);
    q->cb = cb;
    q->ctx = ctx;
    q->when = dtime();
    query_set_state(q, Q_SENT);

    client->tail++;
    if (client->end <= client->tail) {
	client->tail = client->q;
    }
    opo_msg_set_id((uint8_t*)query, q->id);

    if (size != write(client->sock, query, size)) {
	opo_err_no(err, "write failed");
    }
    atomic_flag_clear(&client->tail_lock);

    return q->id;
}

int
opo_client_process(opoClient client, int max, double wait) {
    Query	q;
    int		cnt = 0;

    while (0 >= max || cnt < max) {
	if (NULL != (q = take_next_ready(client, wait))) {
	    if (NULL != q->cb) {
		q->cb(client, q->id, q->resp, q->ctx);
	    }
	    free((uint8_t*)q->resp);
	    q->id = 0;
	    q->resp = NULL;
	    query_set_state(q, Q_CLEAR); // must be last modification to query
	    cnt++;
	} else if (0.0 <= wait) {
	    break;
	}
    }
    return cnt;
}

int
opo_client_pending_count(opoClient client) {
    return (int)((client->tail + client->pending_max - client->on_deck) % client->pending_max);
}

int
opo_client_ready_count(opoClient client) {
    return (int)((client->on_deck + client->pending_max - client->head) % client->pending_max);
}

void
opo_client_set_dictionary(opoClient client, opoBuilder builder) {
    builder->dict = client->dict;
}

opoDict
opo_client_dictionary(opoClient client) {
    return client->dict;
}

