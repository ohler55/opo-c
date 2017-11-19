// Copyright 2017 by Peter Ohler, All Rights Reserved

//#include <errno.h>
//#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "client.h"

struct _opoClient {
    int		sock;
    double	timeout;
};

// Returns addrinfo for a host[:port] string with the default port of 80.
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

    if (0 > connect(sock, res->ai_addr, res->ai_addrlen)) {
	opo_err_no(err, "error connecting socket to %s:%d", host, port);
	close(sock);
	return NULL;
    }
    opoClient	client = (opoClient)malloc(sizeof(struct _opoClient));

    if (NULL == client) {
	opo_err_set(err, OPO_ERR_MEMORY, "failed to allocate memory for a opoClient.");
    } else {
	client->sock = sock;
	if (NULL == options) {
	    client->timeout = 2.0;
	} else {
	    client->timeout = options->timeout;
	}
    }
    return client;
}

void
opo_client_close(opoClient client) {
    // TBD clear out waiting
    if (0 < client->sock) {
	close(client->sock);
    }
    free(client);
}

opoRef
opo_client_query(opoErr err, opoClient client, opoVal query, opoQueryCallback cb, void *ctx) {
    size_t	size = opo_val_bsize(query);

    if (size != write(client->sock, query, size)) {
	opo_err_no(err, "write failed");
    }
    
    // TBD
    return 0;
}

bool
opo_client_cancel(opoClient client, opoRef ref, bool silent) {
    // TBD
    return false;
}

int
opo_client_process(opoClient client, int max, double wait) {
    // TBD
    return 0;
}
