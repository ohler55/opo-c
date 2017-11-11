// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

#include "opo/builder.h"
#include "opo/val.h"
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
    struct _opoValCallbacks	callbacks = {
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

    opo_val_iterate(&err, builder.head, &callbacks, buf);
    
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
    struct _opoValCallbacks	callbacks = {
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

    opo_val_iterate(&err, builder.head, &callbacks, buf);
    
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

    size_t	size = opo_val_bsize(builder.head);

    ut_same_int(90, size, "incorrect size");
    opo_builder_cleanup(&builder);
}

static void
size_null_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_null(&err, &builder, NULL, 0);
    ut_same_int(0, opo_val_size(builder.head), "incorrect size");
}

static void
size_bool_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, true, NULL, 0);
    ut_same_int(0, opo_val_size(builder.head), "incorrect size");
}

static void
size_int_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 23, NULL, 0);
    ut_same_int(1, opo_val_size(builder.head), "small incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, -129, NULL, 0);
    ut_same_int(2, opo_val_size(builder.head), "medium incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 55555, NULL, 0);
    ut_same_int(4, opo_val_size(builder.head), "large incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 2147483649, NULL, 0);
    ut_same_int(8, opo_val_size(builder.head), "huge incorrect size");
}

static void
size_double_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_double(&err, &builder, 1.23, NULL, 0);
    ut_same_int(4, opo_val_size(builder.head), "incorrect size");
}

static void
size_str_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_string(&err, &builder, "shorty", -1, NULL, 0);
    ut_same_int(6, opo_val_size(builder.head), "small incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_string(&err, &builder, "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", -1, NULL, 0);
    ut_same_int(256, opo_val_size(builder.head), "medium incorrect size");
}

static void
size_uuid_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_uuid_string(&err, &builder, "123e4567-e89b-12d3-a456-426655440000", NULL, 0);
    ut_same_int(16, opo_val_size(builder.head), "incorrect size");
}

static void
size_time_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_time(&err, &builder, 1489504166123456789LL, NULL, 0);
    ut_same_int(8, opo_val_size(builder.head), "incorrect size");
}

static void
type_null_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_null(&err, &builder, NULL, 0);
    ut_same_int(OPO_VAL_NULL, opo_val_type(builder.head), "incorrect type");
}

static void
type_bool_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, false, NULL, 0);
    ut_same_int(OPO_VAL_BOOL, opo_val_type(builder.head), "incorrect type");
}

static void
type_int_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 12345, NULL, 0);
    ut_same_int(OPO_VAL_INT, opo_val_type(builder.head), "incorrect type");
}

static void
type_double_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_double(&err, &builder, 12.345, NULL, 0);
    ut_same_int(OPO_VAL_DEC, opo_val_type(builder.head), "incorrect type");
}

static void
type_str_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_string(&err, &builder, "hello", -1, NULL, 0);
    ut_same_int(OPO_VAL_STR, opo_val_type(builder.head), "incorrect type");
}

static void
type_uuid_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_uuid_string(&err, &builder, "123e4567-e89b-12d3-a456-426655440000", NULL, 0);
    ut_same_int(OPO_VAL_UUID, opo_val_type(builder.head), "incorrect type");
}

static void
type_time_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_time(&err, &builder, 1489504166123456789LL, NULL, 0);
    ut_same_int(OPO_VAL_TIME, opo_val_type(builder.head), "incorrect type");
}

static void
type_object_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_object(&err, &builder, NULL, 0);
    opo_builder_pop(&err, &builder);
    ut_same_int(OPO_VAL_OBJ, opo_val_type(builder.head), "incorrect type");
}

static void
type_array_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_array(&err, &builder, NULL, 0);
    opo_builder_pop(&err, &builder);
    ut_same_int(OPO_VAL_ARRAY, opo_val_type(builder.head), "incorrect type");
}

static void
value_bool_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, true, NULL, 0);
    ut_same_int(true, opo_val_bool(&err, builder.head), "incorrect size");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 1, NULL, 0);
    opo_val_bool(&err, builder.head);
    ut_same_int(OPO_ERR_TYPE, err.code, "incorrect error code");
}

static void
value_int_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 23, NULL, 0);
    ut_same_int(23, opo_val_int(&err, builder.head), "small incorrect value");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, -129, NULL, 0);
    ut_same_int(-129, opo_val_int(&err, builder.head), "medium incorrect value");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 55555, NULL, 0);
    ut_same_int(55555, opo_val_int(&err, builder.head), "large incorrect value");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_int(&err, &builder, 2147483649, NULL, 0);
    ut_same_int(2147483649, opo_val_int(&err, builder.head), "huge incorrect value");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, false, NULL, 0);
    opo_val_int(&err, builder.head);
    ut_same_int(OPO_ERR_TYPE, err.code, "incorrect error code");
}

static void
value_double_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_double(&err, &builder, 1.23, NULL, 0);
    ut_true(1.23 == opo_val_double(&err, builder.head), "incorrect value");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, false, NULL, 0);
    opo_val_double(&err, builder.head);
    ut_same_int(OPO_ERR_TYPE, err.code, "incorrect error code");
}

static void
value_str_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_string(&err, &builder, "hello", -1, NULL, 0);
    ut_same("hello", opo_val_string(&err, builder.head, NULL), "incorrect value");

    int	len = 0;
    
    ut_same("hello", opo_val_string(&err, builder.head, &len), "incorrect value2");
    ut_same_int(5, len, "incorrect length");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, false, NULL, 0);
    opo_val_string(&err, builder.head, NULL);
    ut_same_int(OPO_ERR_TYPE, err.code, "incorrect error code");
}

static void
value_uuid_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    char		uuid[40];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_uuid_string(&err, &builder, "123e4567-e89b-12d3-a456-426655440000", NULL, 0);
    opo_val_uuid_str(&err, builder.head, uuid);
    ut_same("123e4567-e89b-12d3-a456-426655440000", uuid, "incorrect value");

    uint64_t	hi;
    uint64_t	lo;
    
    opo_val_uuid(&err, builder.head, &hi, &lo);
    ut_same_int(0x123e4567e89b12d3ULL, hi, "uuid high incorrect");
    ut_same_int(0xa456426655440000ULL, lo, "uuid low incorrect");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, false, NULL, 0);
    opo_val_uuid(&err, builder.head, &hi, &lo);
    ut_same_int(OPO_ERR_TYPE, err.code, "incorrect error code");
}

static void
value_time_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_time(&err, &builder, 1489504166123456789LL, NULL, 0);
    ut_same_int(1489504166123456789LL, opo_val_time(&err, builder.head), "incorrect value");

    opo_builder_init(&err, &builder, data, sizeof(data));
    opo_builder_push_bool(&err, &builder, false, NULL, 0);
    opo_val_time(&err, builder.head);
    ut_same_int(OPO_ERR_TYPE, err.code, "incorrect error code");
}

static void
get_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    build_sample_msg(&builder);

    opoVal	top = builder.head;
    opoVal	v;
    
    v = opo_val_get(top, "int");
    ut_same_int(12345, opo_val_int(&err, v), "incorrect value");

    ut_null(opo_val_get(top, "bad"), "expected null from get bad");
    opo_builder_cleanup(&builder);
}

static void
get_array_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    build_sample_msg(&builder);

    opoVal	top = builder.head;
    opoVal	v;
    
    v = opo_val_get(top, "array.2");
    ut_same("string", opo_val_string(&err, v, NULL), "incorrect value");
    opo_builder_cleanup(&builder);
}

static void
members_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    build_sample_msg(&builder);

    opoVal	top = builder.head;
    opoVal	members = opo_val_members(&err, top);

    ut_not_null(members, "members should not be null for an object");
    // first member should be a key
    ut_same("nil", opo_val_key(&err, members, NULL), "first key mismatch");

    char	keys[256];
    const char	*s;
    opoVal	end = top + opo_val_bsize(top);

    *keys = '\0';

    for (; members < end; members = opo_val_next(members)) {
	if (NULL != (s = opo_val_key(&err, members, NULL))) {
	    strcat(keys, s);
	    strcat(keys, " ");
	}
	opo_val_next(members);
    }
    ut_same("nil yes no int array ", keys, "keys mismatch");
    
    opo_builder_cleanup(&builder);
}

static void
member_count_test() {
    struct _opoBuilder	builder;
    struct _opoErr	err = OPO_ERR_INIT;
    uint8_t		data[1024];
    
    opo_builder_init(&err, &builder, data, sizeof(data));
    build_sample_msg(&builder);

    ut_same_int(5, opo_val_member_count(&err, builder.head), "count mismatch");
    opo_builder_cleanup(&builder);
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
    ut_appenda(tests, "opo.val.value.double", value_double_test, NULL);
    ut_appenda(tests, "opo.val.value.str", value_str_test, NULL);
    ut_appenda(tests, "opo.val.value.uuid", value_uuid_test, NULL);
    ut_appenda(tests, "opo.val.value.time", value_time_test, NULL);

    ut_appenda(tests, "opo.val.get.one", get_test, NULL);
    ut_appenda(tests, "opo.val.get.array", get_array_test, NULL);

    ut_appenda(tests, "opo.val.members", members_test, NULL);
    ut_appenda(tests, "opo.val.member.count", member_count_test, NULL);
}
