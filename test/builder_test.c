// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

#include "opo/builder.h"
#include "ut.h"

static const char	*expect_sample_dump = "\
7B 00 00 00 55 6B 03 6E  69 6C 00 5A 6B 03 79 65   {...Uk.n il.Zk.ye\n\
73 00 74 6B 02 6E 6F 00  66 6B 03 69 6E 74 00 32   s.tk.no. fk.int.2\n\
30 39 6B 05 61 72 72 61  79 00 5B 00 00 00 2B 69   09k.arra y.[...+i\n\
E9 64 04 31 2E 32 33 73  06 73 74 72 69 6E 67 00   .d.1.23s .string.\n\
75 12 3E 45 67 E8 9B 12  D3 A4 56 42 66 55 44 00   u.>Eg... ..VBfUD.\n\
00 54 14 AB C8 25 B9 40  C9 15                     .T...%.@ ..\n";

int
build_sample_msg(opoBuilder builder) {
    struct _opoErr	err = OPO_ERR_INIT;
    
    opo_builder_push_object(&err, builder, NULL, 0);

    opo_builder_push_null(&err, builder, "nil", -1);
    opo_builder_push_bool(&err, builder, true, "yes", -1);
    opo_builder_push_bool(&err, builder, false, "no", -1);
    opo_builder_push_int(&err, builder, 12345, "int", -1);
    opo_builder_push_array(&err, builder, "array", -1);
    
    opo_builder_push_int(&err, builder, -23, NULL, 0);
    opo_builder_push_double(&err, builder, 1.23, NULL, 0);
    opo_builder_push_string(&err, builder, "string", -1, NULL, 0);
    opo_builder_push_uuid_string(&err, builder, "123e4567-e89b-12d3-a456-426655440000", NULL, 0);
    opo_builder_push_time(&err, builder, 1489504166123456789LL, NULL, 0);

    opo_builder_finish(&err, builder);
    ut_same_int(OPO_ERR_OK, err.code, "error writing data. %s", err.msg);

    return OPO_ERR_OK;
}

void
builder_build_buf_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    build_sample_msg(&builder);

    char	buf[1024];

    ut_hex_dump_buf(data, (int)opo_builder_length(&builder), buf);
    ut_same(expect_sample_dump, buf, "hex dump mismatch");
    opo_builder_cleanup(&builder);
}

void
builder_build_alloc_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    
    opo_builder_init(&err, &builder, NULL, 0);
    build_sample_msg(&builder);

    char	buf[1024];

    ut_hex_dump_buf(builder.head, (int)opo_builder_length(&builder), buf);
    ut_same(expect_sample_dump, buf, "hex dump mismatch");
    opo_builder_cleanup(&builder);
}

void
builder_build_val_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    uint8_t		child[1024];
    
    opo_builder_init(&err, &builder, child, sizeof(child));
    opo_builder_push_array(&err, &builder, NULL, 0);
    opo_builder_push_int(&err, &builder, -23, NULL, 0);
    opo_builder_push_double(&err, &builder, 1.23, NULL, 0);
    opo_builder_push_string(&err, &builder, "string", -1, NULL, 0);
    opo_builder_push_uuid_string(&err, &builder, "123e4567-e89b-12d3-a456-426655440000", NULL, 0);
    opo_builder_push_time(&err, &builder, 1489504166123456789LL, NULL, 0);
    opo_builder_finish(&err, &builder);
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_object(&err, &builder, NULL, 0);
    opo_builder_push_null(&err, &builder, "nil", -1);
    opo_builder_push_bool(&err, &builder, true, "yes", -1);
    opo_builder_push_bool(&err, &builder, false, "no", -1);
    opo_builder_push_int(&err, &builder, 12345, "int", -1);
    opo_builder_push_val(&err, &builder, child, "array", -1);
    opo_builder_finish(&err, &builder);

    char	buf[1024];

    ut_hex_dump_buf(data, (int)opo_builder_length(&builder), buf);
    ut_same(expect_sample_dump, buf, "hex dump mismatch");
    opo_builder_cleanup(&builder);
}

void
append_builder_tests(utTest tests) {
    ut_appenda(tests, "opo.builder.buf", builder_build_buf_test, NULL);
    ut_appenda(tests, "opo.builder.alloc", builder_build_alloc_test, NULL);
    ut_appenda(tests, "opo.builder.val", builder_build_val_test, NULL);
}
