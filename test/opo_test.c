// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

void
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
	ut_same_int(jl->len, opo_ojc_msg_size(val), "%s", jl->json);
    }
}

static const char	*expect_sample_dump = "\
7B 00 00 00 55 6B 03 6E  69 6C 00 5A 6B 03 79 65   {...Uk.n il.Zk.ye\n\
73 00 74 6B 02 6E 6F 00  66 6B 03 69 6E 74 00 32   s.tk.no. fk.int.2\n\
30 39 6B 05 61 72 72 61  79 00 5B 00 00 00 2B 69   09k.arra y.[...+i\n\
E9 64 04 31 2E 32 33 73  06 73 74 72 69 6E 67 00   .d.1.23s .string.\n\
75 12 3E 45 67 E8 9B 12  D3 A4 56 42 66 55 44 00   u.>Eg... ..VBfUD.\n\
00 54 14 AB C8 25 B9 40  C9 15                     .T...%.@ ..\n";

void
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

    ut_hex_dump_buf(msg, (int)opo_val_bsize(msg), buf);
    ut_same(expect_sample_dump, buf, "hex dump mismatch");
    free((uint8_t*)msg);
}

void
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

void
append_opo_tests(utTest tests) {
    ut_appenda(tests, "opo.ojc_msg_size", ojc_msg_size_test, NULL);
    ut_appenda(tests, "opo.ojc_to_msg", ojc_to_msg_test, NULL);
    ut_appenda(tests, "opo.msg_to_ojc", msg_to_ojc_test, NULL);
}
