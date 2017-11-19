// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <stdio.h>
#include <unistd.h>

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
query_test() {
    struct _opoErr	err = OPO_ERR_INIT;
    opoClient		client = opo_client_connect(&err, "localhost", 6364, NULL);

    ut_same_int(OPO_ERR_OK, err.code, "error connecting. %s", err.msg);

    struct _opoBuilder	builder;
    uint8_t		query[1024];

    opo_builder_init(&err, &builder, query, sizeof(query));
    opo_builder_push_object(&err, &builder, NULL, -1);
    opo_builder_push_int(&err, &builder, 123, "rid", -1);
    opo_builder_push_null(&err, &builder, "where", -1);
    opo_builder_push_null(&err, &builder, "select", -1);
    opo_builder_finish(&builder);

    opo_client_query(&err, client, query, query_cb, NULL);
    opo_client_close(client);

    for (int i = 10; 0 < i; i--) {
	client = opo_client_connect(&err, "localhost", 6364, NULL);
	opo_client_query(&err, client, query, query_cb, NULL);
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
