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

extern int	build_sample_msg(opoBuilder builder); // defined in builder_test.c

static bool
begin_object(opoErr err, void *ctx) {
    strcat((char*)ctx, "{");
    return true;
}

static bool
end_object(opoErr err, void *ctx) {
    strcat((char*)ctx, "}");
    return true;
}

static bool
begin_array(opoErr err, void *ctx) {
    strcat((char*)ctx, "[");
    return true;
}

static bool
end_array(opoErr err, void *ctx) {
    strcat((char*)ctx, "]");
    return true;
}

static bool
key(opoErr err, const char *key, int len, void *ctx) {
    strncat((char*)ctx, key, len);
    strcat((char*)ctx, ":");
    return true;
}

static bool
null(opoErr err, void *ctx) {
    strcat((char*)ctx, "null ");
    return true;
}

static bool
boolean(opoErr err, bool b, void *ctx) {
    if (b) {
	strcat((char*)ctx, "true ");
    } else {
	strcat((char*)ctx, "false ");
    }
    return true;
}

static bool
fixnum(opoErr err, int64_t num, void *ctx) {
    char	buf[32];

    sprintf(buf, "%lld ", (unsigned long long)num);
    strcat((char*)ctx, buf);
    return true;
}

static bool
decimal(opoErr err, double num, void *ctx) {
    char	buf[32];

    sprintf(buf, "%f ", num);
    strcat((char*)ctx, buf);
    return true;
}

static bool
string(opoErr err, const char *str, int len, void *ctx) {
    strncat((char*)ctx, str, len);
    strcat((char*)ctx, " ");
    return true;
}

static bool
uuid_str(opoErr err, const char *str, void *ctx) {
    strcat((char*)ctx, str);
    strcat((char*)ctx, " ");
    return true;
}

static bool
time_str(opoErr err, const char *str, void *ctx) {
    strcat((char*)ctx, str);
    strcat((char*)ctx, " ");

    return true;
}

static void
iterate_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    build_sample_msg(&builder);

    char			buf[1024];
    struct _opoMsgCallbacks	callbacks = {
	.begin_object = begin_object,
	.end_object = end_object,
	.begin_array = begin_array,
	.end_array = end_array,
	.key = key,
	.null = null,
	.boolean = boolean,
	.fixnum = fixnum,
	.decimal = decimal,
	.string = string,
	.uuid_str = uuid_str,
	.time_str = time_str,
    };
    *buf = '\0';

    opo_val_iterate(&err, opo_msg_top(builder.head), &callbacks, buf);
    
    ut_same("{nil:null yes:true no:false int:12345 array:[-23 1.230000 string 123e4567-e89b-12d3-a456-426655440000 2017-03-14T15:09:26.123456789Z ]}", buf,
	"incorrect output");
    opo_builder_cleanup(&builder);
}

static bool
fixnum_stop(opoErr err, int64_t num, void *ctx) {
    char	buf[32];

    sprintf(buf, "%lld ", (unsigned long long)num);
    strcat((char*)ctx, buf);
    return false;
}

static void
iterate_stop_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    build_sample_msg(&builder);

    char			buf[1024];
    struct _opoMsgCallbacks	callbacks = {
	.begin_object = begin_object,
	.end_object = end_object,
	.begin_array = begin_array,
	.end_array = end_array,
	.key = key,
	.null = null,
	.boolean = boolean,
	.fixnum = fixnum_stop,
	.decimal = decimal,
	.string = string,
	.uuid_str = uuid_str,
	.time_str = time_str,
    };
    *buf = '\0';

    opo_val_iterate(&err, opo_msg_top(builder.head), &callbacks, buf);
    
    ut_same("{nil:null yes:true no:false int:12345 ", buf, "incorrect output");
    opo_builder_cleanup(&builder);
}


// Tests each with a correct cumulative size.
static void
bsize_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    build_sample_msg(&builder);

    size_t	size = opo_val_bsize(opo_msg_top(builder.head));

    ut_same_int(84, size, "incorrect size");
    opo_builder_cleanup(&builder);
}

static void
size_null_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_null(&err, &builder);
    ut_same_int(0, opo_val_size(opo_msg_top(builder.head)), "incorrect size");
}

static void
size_bool_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, true);
    ut_same_int(0, opo_val_size(opo_msg_top(builder.head)), "incorrect size");
}

static void
size_int_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 23);
    ut_same_int(1, opo_val_size(opo_msg_top(builder.head)), "small incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, -129);
    ut_same_int(2, opo_val_size(opo_msg_top(builder.head)), "medium incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 55555);
    ut_same_int(4, opo_val_size(opo_msg_top(builder.head)), "large incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 2147483649);
    ut_same_int(8, opo_val_size(opo_msg_top(builder.head)), "huge incorrect size");
}

static void
size_double_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_double(&err, &builder, 1.23);
    ut_same_int(4, opo_val_size(opo_msg_top(builder.head)), "incorrect size");
}

static void
size_str_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_string(&err, &builder, "shorty", -1);
    ut_same_int(6, opo_val_size(opo_msg_top(builder.head)), "small incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_string(&err, &builder, "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", -1);
    ut_same_int(256, opo_val_size(opo_msg_top(builder.head)), "medium incorrect size");
}

static void
size_uuid_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_uuid_string(&err, &builder, "123e4567-e89b-12d3-a456-426655440000");
    ut_same_int(16, opo_val_size(opo_msg_top(builder.head)), "incorrect size");
}

static void
size_time_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_time(&err, &builder, 1489504166123456789LL);
    ut_same_int(8, opo_val_size(opo_msg_top(builder.head)), "incorrect size");
}

static void
type_null_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_null(&err, &builder);
    ut_same_int(OPO_VAL_NULL, opo_val_type(opo_msg_top(builder.head)), "incorrect type");
}

static void
type_bool_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, false);
    ut_same_int(OPO_VAL_BOOL, opo_val_type(opo_msg_top(builder.head)), "incorrect type");
}

static void
type_int_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 12345);
    ut_same_int(OPO_VAL_INT, opo_val_type(opo_msg_top(builder.head)), "incorrect type");
}

static void
type_double_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_double(&err, &builder, 12.345);
    ut_same_int(OPO_VAL_DEC, opo_val_type(opo_msg_top(builder.head)), "incorrect type");
}

static void
type_str_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_string(&err, &builder, "hello", -1);
    ut_same_int(OPO_VAL_STR, opo_val_type(opo_msg_top(builder.head)), "incorrect type");
}

static void
type_uuid_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_uuid_string(&err, &builder, "123e4567-e89b-12d3-a456-426655440000");
    ut_same_int(OPO_VAL_UUID, opo_val_type(opo_msg_top(builder.head)), "incorrect type");
}

static void
type_time_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_time(&err, &builder, 1489504166123456789LL);
    ut_same_int(OPO_VAL_TIME, opo_val_type(opo_msg_top(builder.head)), "incorrect type");
}

static void
type_object_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_object(&err, &builder);
    opo_builder_pop(&err, &builder);
    ut_same_int(OPO_VAL_OBJ, opo_val_type(opo_msg_top(builder.head)), "incorrect type");
}

static void
type_array_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_array(&err, &builder);
    opo_builder_pop(&err, &builder);
    ut_same_int(OPO_VAL_ARRAY, opo_val_type(opo_msg_top(builder.head)), "incorrect type");
}

static void
value_bool_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, true);
    ut_same_int(true, opo_val_bool(&err, opo_msg_top(builder.head)), "incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 1);
    opo_val_bool(&err, opo_msg_top(builder.head));
    ut_same_int(OPO_ERR_TYPE, err.code, "huge incorrect size");
}

static void
value_int_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 23);
    ut_same_int(23, opo_val_int(&err, opo_msg_top(builder.head)), "small incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, -129);
    ut_same_int(-129, opo_val_int(&err, opo_msg_top(builder.head)), "medium incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 55555);
    ut_same_int(55555, opo_val_int(&err, opo_msg_top(builder.head)), "large incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 2147483649);
    ut_same_int(2147483649, opo_val_int(&err, opo_msg_top(builder.head)), "huge incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, false);
    opo_val_int(&err, opo_msg_top(builder.head));
    ut_same_int(OPO_ERR_TYPE, err.code, "huge incorrect size");
}



void
append_val_tests(utTest tests) {
    ut_appenda(tests, "opo.val.iterate", iterate_test, NULL);
    ut_appenda(tests, "opo.val.iterate.stop", iterate_stop_test, NULL);

    ut_appenda(tests, "opo.val.bsize", bsize_test, NULL);
    ut_appenda(tests, "opo.val.size.null", size_null_test, NULL);
    ut_appenda(tests, "opo.val.size.bool", size_bool_test, NULL);
    ut_appenda(tests, "opo.val.size.int", size_int_test, NULL);
    ut_appenda(tests, "opo.val.size.double", size_double_test, NULL);
    ut_appenda(tests, "opo.val.size.str", size_str_test, NULL);
    ut_appenda(tests, "opo.val.size.uuid", size_uuid_test, NULL);
    ut_appenda(tests, "opo.val.size.time", size_time_test, NULL);

    ut_appenda(tests, "opo.val.type.null", type_null_test, NULL);
    ut_appenda(tests, "opo.val.type.bool", type_bool_test, NULL);
    ut_appenda(tests, "opo.val.type.int", type_int_test, NULL);
    ut_appenda(tests, "opo.val.type.double", type_double_test, NULL);
    ut_appenda(tests, "opo.val.type.str", type_str_test, NULL);
    ut_appenda(tests, "opo.val.type.uuid", type_uuid_test, NULL);
    ut_appenda(tests, "opo.val.type.time", type_time_test, NULL);
    ut_appenda(tests, "opo.val.type.object", type_object_test, NULL);
    ut_appenda(tests, "opo.val.type.array", type_array_test, NULL);

    ut_appenda(tests, "opo.val.value.bool", value_bool_test, NULL);
    ut_appenda(tests, "opo.val.value.int", value_int_test, NULL);
    //ut_appenda(tests, "opo.val.value.double", value_double_test, NULL);
    //ut_appenda(tests, "opo.val.value.str", value_str_test, NULL);
    //ut_appenda(tests, "opo.val.value.uuid", value_uuid_test, NULL);
    //ut_appenda(tests, "opo.val.value.time", value_time_test, NULL);

    // TBD tests for get and aget
    // TBD tests for members and next
}