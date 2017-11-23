// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

//#include <ojc/ojc.h>
#include <opo/opo.h>

#include <opo/client.h>
#include <opo/builder.h>

#include "ut.h"

static double
dtime() {
    struct timeval	tv;

    gettimeofday(&tv, NULL);

    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}


static void
connect_test() {
    struct _opoErr	err = OPO_ERR_INIT;
    opoClient		client = opo_client_connect(&err, "localhost", 6364, NULL);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    opo_client_close(client);
}

static void
query_cb(opoRef ref, opoVal response, void *ctx) {
    int	*cntp = (int*)ctx;

    opoVal	top = opo_msg_val(response);
    opoVal	cval = opo_val_get(top, "code");
    struct _opoErr	err = OPO_ERR_INIT;
    int64_t		code = opo_val_int(&err, cval);

    if (0 != code) {
	printf("*** code not zero\n");
    }

#if 0
    struct _opoErr	oe = OPO_ERR_INIT;
    ojcVal		v = opo_msg_to_ojc(&oe, response);
    char		*json = ojc_to_str(v, 2);

    printf("%llu: %s\n", (unsigned long long)opo_msg_id(response), json);
    ojc_destroy(v);
    free(json);
#endif

    *cntp += 1;
}

static void*
process_loop(void *ctx) {
    opo_client_process((opoClient)ctx, 0, 0.5);
    return NULL;
}

static void
build_query(uint8_t *query, size_t qsize, int64_t rid) {
    struct _opoErr	err = OPO_ERR_INIT;
    struct _opoBuilder	builder;

    opo_builder_init(&err, &builder, query, qsize);
    opo_builder_push_object(&err, &builder, NULL, -1);
    opo_builder_push_int(&err, &builder, rid, "rid", -1);
#if 0
    opo_builder_push_array(&err, &builder, "where", -1);
    opo_builder_push_string(&err, &builder, "EQ", 2, NULL, -1);
    opo_builder_push_string(&err, &builder, "kind", 4, NULL, -1);
    opo_builder_push_string(&err, &builder, "Foo", 3, NULL, -1);
    opo_builder_pop(&err, &builder);
#else
    opo_builder_push_int(&err, &builder, (int64_t)0x000000000000050bULL, "where", -1);
    //opo_builder_push_int(&err, &builder, (int64_t)0x000000000000000bULL, "where", -1);
#endif
    opo_builder_push_string(&err, &builder, "$", 1, "select", -1);
    opo_builder_finish(&builder);
}

static void
status_callback(opoClient client, bool connected, opoErrCode code, const char *msg) {
    printf("*** %s\n", msg);
}

static void
query_test() {
    struct _opoErr	err = OPO_ERR_INIT;
    struct _opoClientOptions	options = {
	.timeout = 0.2,
	.pending_max = 4096,
	.status_callback = status_callback,
    };
    opoClient		client = opo_client_connect(&err, "localhost", 6364, &options);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    uint8_t	query[1024];
    int64_t	rid = 123;
    int		cnt = 0;
    pthread_t	thread;

    pthread_create(&thread, NULL, process_loop, client);
    build_query(query, sizeof(query), rid++);
    opo_client_query(&err, client, query, query_cb, &cnt);

    // Wait for processing thread to get started.
    for (int i = 100; 0 < i; i--) {
	if (0 < cnt) {
	    break;
	}
	usleep(1000);
    }
    int		iter = 100000;
    double	dt = dtime() + 5.0; // used as timeout first
    double	start = dtime();
    
    for (int i = iter; 0 < i; i--) {
	build_query(query, sizeof(query), rid++);
	opo_client_query(&err, client, query, query_cb, &cnt);
	if (OPO_ERR_OK != err.code) {
	    printf("*** error sending %s\n", err.msg);
	}
    }
    // Wait for all to complete

    while (cnt <= iter && dtime() < dt) {
	usleep(100);
    }
    dt = dtime() - start;
    cnt--;
    printf("--- query rate: %d in %0.3f secs  %d queries/sec\n", cnt, dt, (int)((double)cnt / dt));

    pthread_join(thread, NULL);

    // TBD sometimes out of sequence

    opo_client_close(client);
}

void
append_client_tests(utTest tests) {
    ut_appenda(tests, "opo.client.connect", connect_test, NULL);
    ut_appenda(tests, "opo.client.query", query_test, NULL);
    // TBD query, get response, close
    // TBD multiple calls, async
}
