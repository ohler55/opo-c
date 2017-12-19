// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <opo/opo.h>

#include "ut.h"

static void
connect_test() {
    struct _opoErr	err = OPO_ERR_INIT;
    opoClient		client = opo_client_connect(&err, "127.0.0.1", 6364, NULL);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    opo_client_close(client);
}

static void
status_callback(opoClient client, bool connected, opoErrCode code, const char *msg) {
    printf("--- %s\n", msg);
}

void
print_msg(opoMsg msg) {
    struct _opoErr	oe = OPO_ERR_INIT;
    ojcVal		v = opo_msg_to_ojc(&oe, msg);
    char		*json = ojc_to_str(v, 2);

    printf("%llu: %s\n", (unsigned long long)opo_msg_id(msg), json);
    ojc_destroy(v);
    free(json);
}

static void*
process_loop(void *ctx) {
    opo_client_process((opoClient)ctx, 0, 0.5);
    return NULL;
}

static void
build_query(uint8_t *query, size_t qsize, int64_t rid, uint64_t ref) {
    struct _opoErr	err = OPO_ERR_INIT;
    struct _opoBuilder	builder;

    opo_builder_init(&err, &builder, query, qsize);
    opo_builder_push_object(&err, &builder, NULL, -1);
    if (0 < rid) {
	opo_builder_push_int(&err, &builder, rid, "rid", 3);
    }
#if 0
    // A query instead of a get/fetch.
    opo_builder_push_int(&err, &builder, 1, "limit", 5);
    opo_builder_push_array(&err, &builder, "where", 5);
    opo_builder_push_string(&err, &builder, "EQ", 2, NULL, -1);
    opo_builder_push_string(&err, &builder, "kind", 4, NULL, -1);
    opo_builder_push_string(&err, &builder, "Trade", 5, NULL, -1);
    opo_builder_pop(&err, &builder);
#else
    opo_builder_push_int(&err, &builder, (int64_t)ref, "where", 5);
#endif
    opo_builder_push_string(&err, &builder, "$", 1, "select", 6);
    opo_builder_finish(&builder);
}

static void
delete_records(opoClient client) {
    int			cnt;
    uint8_t		query[1024];
    struct _opoErr	err = OPO_ERR_INIT;
    struct _opoBuilder	builder;

    // First delete existing 'Test' records
    opo_builder_init(&err, &builder, query, sizeof(query));
    opo_builder_push_object(&err, &builder, NULL, -1);
    opo_builder_push_array(&err, &builder, "where", -1);
    opo_builder_push_string(&err, &builder, "EQ", 2, NULL, -1);
    opo_builder_push_string(&err, &builder, "kind", 4, NULL, -1);
    opo_builder_push_string(&err, &builder, "Trade", -1, NULL, -1);
    opo_builder_pop(&err, &builder);
    opo_builder_push_null(&err, &builder, "delete", -1);
    opo_builder_finish(&builder);

    opo_client_query(&err, client, query, NULL, NULL);
    cnt = opo_client_process(client, 1, 1.0);
    ut_same_int(1, cnt, "failed to process a query", err.msg);
}

static void
add_cb(opoRef ref, opoVal response, void *ctx) {
    uint64_t		*refp = (uint64_t*)ctx;
    opoVal		top = opo_msg_val(response);
    opoVal		rval = opo_val_get(top, "ref");
    struct _opoErr	err = OPO_ERR_INIT;

    *refp = (uint64_t)opo_val_int(&err, rval);
    //print_msg(response);
}

static uint64_t
add_records(opoClient client) {
    uint64_t		ref = 0;
    int			cnt;
    uint8_t		query[1024];
    struct _opoErr	err = OPO_ERR_INIT;
    struct _opoBuilder	builder;

    for (int i = 10; 0 < i; i--) {
	opo_builder_init(&err, &builder, query, sizeof(query));
	opo_builder_push_object(&err, &builder, NULL, -1);
	opo_builder_push_object(&err, &builder, "insert", -1);

	opo_builder_push_string(&err, &builder, "Trade", 5, "kind", 4);
	opo_builder_push_int(&err, &builder, 1512247371000000000LL + i, "when", 4);
	opo_builder_push_string(&err, &builder, "OPO", 3, "symbol", 6);
	opo_builder_push_int(&err, &builder, 100, "quantity", 8);
	opo_builder_push_int(&err, &builder, 100 + i, "price", 5);

	opo_builder_finish(&builder);

	opo_client_query(&err, client, query, add_cb, &ref);
    }
    cnt = opo_client_process(client, 10, 1.0);
    ut_same_int(10, cnt, "failed to process a query", err.msg);

    return ref;
}

static uint64_t
setup_records(opoClient client) {
    delete_records(client);

    return add_records(client);
}

static void
query_cb(opoRef ref, opoVal response, void *ctx) {
    int			*cntp = (int*)ctx;
    opoVal		top = opo_msg_val(response);
    opoVal		cval = opo_val_get(top, "code");
    struct _opoErr	err = OPO_ERR_INIT;
    int64_t		code = opo_val_int(&err, cval);

    ut_same_int(0, code, "code not zero.");
    *cntp += 1;
    //print_msg(response);
}

static void
query_test() {
    struct _opoErr		err = OPO_ERR_INIT;
    struct _opoClientOptions	options = {
	.timeout = 0.2,
	.pending_max = 1024,
	.status_callback = status_callback,
    };
    opoClient	client = opo_client_connect(&err, "127.0.0.1", 6364, &options);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    uint64_t	ref = setup_records(client);
    uint8_t	query[1024];
    int		cnt = 0;
    pthread_t	thread;

    pthread_create(&thread, NULL, process_loop, client);
    build_query(query, sizeof(query), 0, ref);
    opo_client_query(&err, client, query, query_cb, &cnt);

    // Wait for processing thread to get started.
    for (int i = 100; 0 < i; i--) {
	if (0 < cnt) {
	    break;
	}
	usleep(1000);
    }
    cnt--;
    int		iter = 100000;
    double	dt = dtime() + 5.0; // used as timeout first
    double	start = dtime();
    
    for (int i = iter; 0 < i; i--) {
	build_query(query, sizeof(query), 0, ref);
	opo_client_query(&err, client, query, query_cb, &cnt);
	if (OPO_ERR_OK != err.code) {
	    printf("*** error sending %s\n", err.msg);
	}
    }
    // Wait for all to complete
    while (cnt < iter && dtime() < dt) {
	usleep(100);
    }
    dt = dtime() - start;
    printf("--- query rate: %d in %0.3f secs  %d queries/sec\n", cnt, dt, (int)((double)cnt / dt));

    pthread_join(thread, NULL);
    opo_client_close(client);
}

typedef struct _ACtx {
    uint64_t		ref;
    atomic_int_fast64_t	pending;
} *ACtx;

static void
async_query_cb(opoRef ref, opoVal response, void *ctx) {
    ACtx		ax = (ACtx)ctx;
    opoVal		top = opo_msg_val(response);
    struct _opoErr	err = OPO_ERR_INIT;
    int64_t		code = opo_val_int(&err, opo_val_get(top, "code"));
    int64_t		rref = opo_val_int(&err, opo_val_get(top, "ref"));

    //print_msg(response);
    ut_same_int(0, code, "code not zero.");
    if (0 != rref) {
	ax->ref = rref;
    } else {
	atomic_fetch_sub(&ax->pending, 1);
    }
}

static void
async_query_test() {
    struct _opoErr	err = OPO_ERR_INIT;
    struct _ACtx	ctx;

    ctx.ref = 0;
    atomic_init(&ctx.pending, 0);

    struct _opoClientOptions	options = {
	.timeout = 0.2,
	.pending_max = 1024,
	.status_callback = status_callback,
	.query_callback = async_query_cb,
	.query_ctx = &ctx,
    };
    opoClient	client = opo_client_connect(&err, "127.0.0.1", 6364, &options);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    setup_records(client);

    uint8_t	query[1024];
    pthread_t	thread;

    pthread_create(&thread, NULL, process_loop, client);

    int		iter = 100000;
    double	dt = dtime() + 5.0; // used as timeout first
    double	start = dtime();
    
    for (int i = iter; 0 < i; i--) {
	build_query(query, sizeof(query), 0, ctx.ref);
	opo_client_query(&err, client, query, NULL, NULL);
	if (OPO_ERR_OK != err.code) {
	    printf("*** error sending: %s\n", err.msg);
	    opo_err_clear(&err);
	}
    }
    // Wait for all to complete
    while (0 < atomic_load(&ctx.pending) && dtime() < dt) {
	usleep(100);
    }
    dt = dtime() - start;
    printf("--- query rate: %d in %0.3f secs  %d queries/sec\n", iter, dt, (int)((double)iter / dt));

    pthread_join(thread, NULL);
    opo_client_close(client);
}

static void
dual_query_test() {
    struct _opoErr		err = OPO_ERR_INIT;
    struct _opoClientOptions	options = {
	.timeout = 0.2,
	.pending_max = 1024,
	.status_callback = status_callback,
    };
    opoClient	c1 = opo_client_connect(&err, "127.0.0.1", 6364, &options);
    opoClient	c2 = opo_client_connect(&err, "127.0.0.1", 6364, &options);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    uint64_t	ref = setup_records(c1);
    uint8_t	query[1024];
    int		cnt1 = 0;
    int		cnt2 = 0;
    pthread_t	t1;
    pthread_t	t2;

    pthread_create(&t1, NULL, process_loop, c1);
    pthread_create(&t2, NULL, process_loop, c2);
    build_query(query, sizeof(query), 0, ref);
    opo_client_query(&err, c1, query, query_cb, &cnt1);
    opo_client_query(&err, c2, query, query_cb, &cnt2);

    // Wait for processing thread to get started.
    for (int i = 100; 0 < i; i--) {
	if (0 < cnt1 && 0 < cnt2) {
	    break;
	}
	usleep(1000);
    }
    cnt1--;
    cnt2--;
    int		iter = 100000;
    double	dt = dtime() + 5.0; // used as timeout first
    double	start = dtime();
    
    for (int i = iter; 0 < i; i--) {
	//build_query(query, sizeof(query), 0, ref);
	opo_client_query(&err, c1, query, query_cb, &cnt1);
	opo_client_query(&err, c2, query, query_cb, &cnt2);
	if (OPO_ERR_OK != err.code) {
	    printf("*** error sending %s\n", err.msg);
	}
    }
    // Wait for all to complete
    while (cnt1 < iter && cnt2 < iter && dtime() < dt) {
	usleep(100);
    }
    dt = dtime() - start;
    printf("--- query rate: %d in %0.3f secs  %d queries/sec\n", cnt1 + cnt2, dt, (int)((double)(cnt1 + cnt2) / dt));

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    opo_client_close(c1);
    opo_client_close(c2);
}

static void
dual_async_test() {
    struct _opoErr		err = OPO_ERR_INIT;
    struct _ACtx		ctx1;
    struct _ACtx		ctx2;

    ctx1.ref = 0;
    ctx2.ref = 0;
    atomic_init(&ctx1.pending, 0);
    atomic_init(&ctx2.pending, 0);
    struct _opoClientOptions	options = {
	.timeout = 0.2,
	.pending_max = 4096,
	.status_callback = status_callback,
	.query_callback = async_query_cb,
	.query_ctx = &ctx1,
    };
    opoClient	c1 = opo_client_connect(&err, "127.0.0.1", 6364, &options);
    options.query_ctx = &ctx2;
    opoClient	c2 = opo_client_connect(&err, "127.0.0.1", 6364, &options);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    setup_records(c1);
    
    uint8_t	query[1024];
    pthread_t	t1;
    pthread_t	t2;

    pthread_create(&t1, NULL, process_loop, c1);
    pthread_create(&t2, NULL, process_loop, c2);
    build_query(query, sizeof(query), 0, ctx1.ref);

    int		iter = 100000;
    double	dt = dtime() + 5.0; // used as timeout first
    double	start = dtime();
    
    for (int i = iter; 0 < i; i--) {
	//build_query(query, sizeof(query), 0, ref);
	opo_client_query(&err, c1, query, NULL, NULL);
	opo_client_query(&err, c2, query, NULL, NULL);
	if (OPO_ERR_OK != err.code) {
	    printf("*** error sending %s\n", err.msg);
	}
    }
    // Wait for all to complete
    while (0 < atomic_load(&ctx1.pending) && 0 < atomic_load(&ctx2.pending) && dtime() < dt) {
	usleep(100);
    }
    dt = dtime() - start;
    printf("--- query rate: %d in %0.3f secs  %d queries/sec\n", iter * 2, dt, (int)((double)(iter * 2) / dt));

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    opo_client_close(c1);
    opo_client_close(c2);
}

typedef struct _Lat {
    int		cnt;
    int		max_rid;
    double	*times;
} *Lat;
    
static void
latency_cb(opoRef ref, opoVal response, void *ctx) {
    Lat			lat = (Lat)ctx;
    opoVal		top = opo_msg_val(response);
    opoVal		rval = opo_val_get(top, "rid");
    struct _opoErr	err = OPO_ERR_INIT;
    int64_t		rid = opo_val_int(&err, rval);

    if (0 < rid && rid <= lat->max_rid) {
	lat->times[rid - 1] = dtime() - lat->times[rid - 1];
    }
    lat->cnt++;
}

static void
latency_test() {
    struct _opoErr		err = OPO_ERR_INIT;
    struct _opoClientOptions	options = {
	.timeout = 0.2,
	.pending_max = 10,
	.status_callback = status_callback,
    };
    opoClient	client = opo_client_connect(&err, "127.0.0.1", 6364, &options);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    uint64_t	ref = setup_records(client);
    uint8_t	query[1024];
    int		cnt = 0;
    pthread_t	thread;
    int		iter = 1000;
    double	times[iter];
    struct _Lat	lat = {
	.cnt = 0,
	.max_rid = iter,
	.times = times,
    };

    pthread_create(&thread, NULL, process_loop, client);
    build_query(query, sizeof(query), iter + 1, ref);
    opo_client_query(&err, client, query, latency_cb, &lat);

    // Wait for processing thread to get started.
    for (int i = 100; 0 < i; i--) {
	if (0 < cnt) {
	    break;
	}
	usleep(1000);
    }
    lat.cnt = 0;

    double	done = dtime() + 5.0;
    
    for (int i = iter; 0 < i; i--) {
	lat.times[i - 1] = dtime();
	build_query(query, sizeof(query), i, ref);
	opo_client_query(&err, client, query, latency_cb, &lat);
	ut_same_int(OPO_ERR_OK, err.code, "error sending. %s", err.msg);
    }
    // Wait for all to complete
    while (lat.cnt < iter && dtime() < done) {
	usleep(100);
    }
    double	sum = 0.0;
    
    for (int i = iter; 0 < i; i--) {
	sum += lat.times[i - 1];
	//printf("*** %f usecs\n", lat.times[i - 1] * 1000000.0);
    }
    printf("--- query latency: %d usecs/query\n", (int)((sum / iter) * 1000000.0));

    pthread_join(thread, NULL);
    opo_client_close(client);
}

void
append_client_tests(utTest tests) {
    ut_appenda(tests, "opo.client.connect", connect_test, NULL);
    ut_appenda(tests, "opo.client.query", query_test, NULL);
    ut_appenda(tests, "opo.client.async.query", async_query_test, NULL);
    ut_appenda(tests, "opo.client.dual.query", dual_query_test, NULL);
    ut_appenda(tests, "opo.client.dual.async", dual_async_test, NULL);
    ut_appenda(tests, "opo.client.latency", latency_test, NULL);
}
