// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

#include <ojc/buf.h>

#include "opo/opo.h"
#include "ut.h"

typedef struct _Jlen {
    const char	*json;
    int		len;
} *Jlen;

extern int	build_sample_msg(opoBuilder builder); // defined in builder_test.c

static void
ojc_msg_size_test() {
    ojcVal		val;
    struct _ojcErr	err;
    struct _Jlen	data[] = {
	{ "null", 1 },
	{ "false", 1 },
	{ "true", 1 },
	{ "127", 2 },
	{ "128", 3 },
	{ "-40000", 5 },
	{ "\"hello\"", 8 },
	{ "12.3", 6 },
	{ "{}", 5 },
	{ "[]", 5 },
	{ "[1,2,3]", 11 },
	{ "{\"abc\":1,\"def\":2}", 21 },
	{ "{\"abc\":1,\"def\":[1,2]}", 28 },
	{ "\"123e4567-e89b-12d3-a456-426655440000\"", 17 },
	{ "\"2017-03-14T15:09:26.123456789Z\"", 9 },
	{ NULL, 0 }};

    for (Jlen jl = data; NULL != jl->json; jl++) {
	ojc_err_init(&err);
	val = ojc_parse_str(&err, jl->json, 0, 0);
	ut_same_int(OPO_ERR_OK, err.code, "error writing data. %s", err.msg);
	ut_same_int(jl->len + 8, opo_ojc_msg_size(val), "%s", jl->json);
    }
}

static const char	*expect_sample_dump = "\
00 00 00 00 00 00 00 00  7B 00 00 00 55 6B 03 6E   ........ {...Uk.n\n\
69 6C 00 5A 6B 03 79 65  73 00 74 6B 02 6E 6F 00   il.Zk.ye s.tk.no.\n\
66 6B 03 69 6E 74 00 32  30 39 6B 05 61 72 72 61   fk.int.2 09k.arra\n\
79 00 5B 00 00 00 2B 69  E9 64 04 31 2E 32 33 73   y.[...+i .d.1.23s\n\
06 73 74 72 69 6E 67 00  75 12 3E 45 67 E8 9B 12   .string. u.>Eg...\n\
D3 A4 56 42 66 55 44 00  00 54 14 AB C8 25 B9 40   ..VBfUD. .T...%.@\n\
C9 15                                              ..\n";

static void
ojc_to_msg_test() {
    struct _ojcErr	err = OJC_ERR_INIT;
    ojcVal		val = ojc_parse_str(&err, "{\n\
  \"nil\":null,\n\
  \"yes\":true,\n\
  \"no\":false,\n\
  \"int\":12345,\n\
  \"array\":[\n\
    -23,\n\
    1.23,\n\
    \"string\",\n\
    \"123e4567-e89b-12d3-a456-426655440000\",\n\
    \"2017-03-14T15:09:26.123456789Z\"\n\
  ]\n\
}", 0, 0);

    ut_same_int(OJC_OK, err.code, "error parsing JSON. %s", err.msg);

    struct _opoErr	oerr = OPO_ERR_INIT;
    opoVal		msg = opo_ojc_to_msg(&oerr, val);

    ut_same_int(OPO_ERR_OK, err.code, "error creating message. %s", oerr.msg);

    char	buf[1024];

    ut_hex_dump_buf(msg, (int)opo_msg_bsize(msg), buf);
    ut_same(expect_sample_dump, buf, "hex dump mismatch");
    free((uint8_t*)msg);
}

static void
msg_to_ojc_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    build_sample_msg(&builder);

    ojcVal	val = opo_msg_to_ojc(&err, builder.head);

    ut_same_int(OPO_ERR_OK, err.code, "error creating ojc. %s", err.msg);
    opo_builder_cleanup(&builder);

    struct _Buf	buf;

    buf_init(&buf, 0);
    ojc_buf(&buf, val, 2, 0);
    ut_same("{\n\
  \"nil\":null,\n\
  \"yes\":true,\n\
  \"no\":false,\n\
  \"int\":12345,\n\
  \"array\":[\n\
    -23,\n\
    1.23,\n\
    \"string\",\n\
    \"123e4567-e89b-12d3-a456-426655440000\",\n\
    \"2017-03-14T15:09:26.123456789Z\"\n\
  ]\n\
}", buf.head, "JSON did not match");
}

static void
bench_msg_to_ojc_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    build_sample_msg(&builder);

    int		iter = 100000;
    ojcVal	val;
    double	dt;
    double	start = dtime();

    for (int i = iter; 0 < i; i--) {
	if (NULL != (val = opo_msg_to_ojc(&err, builder.head))) {
	    ojc_destroy(val);
	} else {
	    ut_same_int(OPO_ERR_OK, err.code, "error transforming. %s", err.msg);
	}
    }
    dt = dtime() - start;
    printf("--- opo_msg_to_ojc rate: %d in %0.3f secs  %d transforms/sec\n", iter, dt, (int)((double)iter / dt));
}

static void
bench_ojc_to_msg_test() {
    struct _ojcErr	err = OJC_ERR_INIT;
    ojcVal		val = ojc_parse_str(&err, "{\n\
  \"nil\":null,\n\
  \"yes\":true,\n\
  \"no\":false,\n\
  \"int\":12345,\n\
  \"array\":[\n\
    -23,\n\
    1.23,\n\
    \"string\",\n\
    \"123e4567-e89b-12d3-a456-426655440000\",\n\
    \"2017-03-14T15:09:26.123456789Z\"\n\
  ]\n\
}", 0, 0);

    ut_same_int(OJC_OK, err.code, "error parsing JSON. %s", err.msg);

    struct _opoErr	oerr = OPO_ERR_INIT;
    opoVal		msg;
    int			iter = 100000;
    double		dt;
    double		start = dtime();

    for (int i = iter; 0 < i; i--) {
	if (NULL != (msg = opo_ojc_to_msg(&oerr, val))) {
	    free((uint8_t*)msg);
	} else {
	    ut_same_int(OPO_ERR_OK, err.code, "error transforming. %s", err.msg);
	}
    }
    dt = dtime() - start;
    printf("--- opo_ojc_to_msg rate: %d in %0.3f secs  %d transforms/sec\n", iter, dt, (int)((double)iter / dt));
}

void
append_opo_tests(utTest tests) {
    ut_appenda(tests, "opo.ojc_msg_size", ojc_msg_size_test, NULL);
    ut_appenda(tests, "opo.ojc_to_msg", ojc_to_msg_test, NULL);
    ut_appenda(tests, "opo.msg_to_ojc", msg_to_ojc_test, NULL);
    ut_appenda(tests, "opo.bench.msg_to_ojc", bench_msg_to_ojc_test, NULL);
    ut_appenda(tests, "opo.bench.ojc_to_msg", bench_ojc_to_msg_test, NULL);
}
