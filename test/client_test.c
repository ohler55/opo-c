// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <stdio.h>
#include <unistd.h>

//#include <ojc/ojc.h>
#include <opo/opo.h>

#include <opo/client.h>
#include <opo/builder.h>

#include "ut.h"


static void
connect_test() {
    struct _opoErr	err = OPO_ERR_INIT;
    opoClient		client = opo_client_connect(&err, "localhost", 6364, NULL);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    opo_client_close(client);
}

static void
query_cb(opoRef ref, opoVal response, void *ctx) {
}

static void
build_query(uint8_t *query, size_t qsize, int64_t rid) {
    struct _opoErr	err = OPO_ERR_INIT;
    struct _opoBuilder	builder;

    opo_builder_init(&err, &builder, query, qsize);
    opo_builder_push_object(&err, &builder, NULL, -1);
    opo_builder_push_int(&err, &builder, rid, "rid", -1);
    opo_builder_push_array(&err, &builder, "where", -1);
    opo_builder_push_string(&err, &builder, "EQ", 2, NULL, -1);
    opo_builder_push_string(&err, &builder, "kind", 4, NULL, -1);
    opo_builder_push_string(&err, &builder, "Foo", 3, NULL, -1);
    opo_builder_pop(&err, &builder);
    opo_builder_push_string(&err, &builder, "$", 1, "select", -1);
    opo_builder_finish(&builder);
}

static void
query_test() {
    struct _opoErr	err = OPO_ERR_INIT;
    opoClient		client = opo_client_connect(&err, "localhost", 6364, NULL);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    uint8_t	query[1024];
    int64_t	rid = 123;

    build_query(query, sizeof(query), rid++);
    opo_client_query(&err, client, query, query_cb, NULL);

    opo_client_process(client, 1, 1.0);

    opo_client_close(client);

    for (int i = 2; 0 < i; i--) {
	client = opo_client_connect(&err, "localhost", 6364, NULL);
	build_query(query, sizeof(query), rid++);
	opo_client_query(&err, client, query, query_cb, NULL);

	opo_client_process(client, 1, 1.0);

	opo_client_close(client);
    }
}

void
append_client_tests(utTest tests) {
    ut_appenda(tests, "opo.client.connect", connect_test, NULL);
    ut_appenda(tests, "opo.client.query", query_test, NULL);
    // TBD query, get response, close
    // TBD multiple calls, async
}
