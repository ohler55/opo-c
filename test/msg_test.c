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

static int
begin_object(opoErr err, void *ctx) {
    strcat((char*)ctx, "{");
    return OPO_ERR_OK;
}

static int
end_object(opoErr err, void *ctx) {
    strcat((char*)ctx, "}");
    return OPO_ERR_OK;
}

static int
begin_array(opoErr err, void *ctx) {
    strcat((char*)ctx, "[");
    return OPO_ERR_OK;
}

static int
end_array(opoErr err, void *ctx) {
    strcat((char*)ctx, "]");
    return OPO_ERR_OK;
}

static int
key(opoErr err, const char *key, int len, void *ctx) {
    strncat((char*)ctx, key, len);
    strcat((char*)ctx, ":");
    return OPO_ERR_OK;
}

static int
null(opoErr err, void *ctx) {
    strcat((char*)ctx, "null ");
    return OPO_ERR_OK;
}

static int
boolean(opoErr err, bool b, void *ctx) {
    if (b) {
	strcat((char*)ctx, "true ");
    } else {
	strcat((char*)ctx, "false ");
    }
    return OPO_ERR_OK;
}

static int
fixnum(opoErr err, int64_t num, void *ctx) {
    char	buf[32];

    sprintf(buf, "%lld ", (unsigned long long)num);
    strcat((char*)ctx, buf);
    return OPO_ERR_OK;
}

static int
decimal(opoErr err, double num, void *ctx) {
    char	buf[32];

    sprintf(buf, "%f ", num);
    strcat((char*)ctx, buf);
    return OPO_ERR_OK;
}

static int
string(opoErr err, const char *str, int len, void *ctx) {
    strncat((char*)ctx, str, len);
    strcat((char*)ctx, " ");
    return OPO_ERR_OK;
}

static int
uuid_str(opoErr err, const char *str, void *ctx) {
    strcat((char*)ctx, str);
    strcat((char*)ctx, " ");
    return OPO_ERR_OK;
}

static int
time_str(opoErr err, const char *str, void *ctx) {
    strcat((char*)ctx, str);
    strcat((char*)ctx, " ");

    return OPO_ERR_OK;
}

void
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
    opo_msg_iterate(&err, builder.head, &callbacks, buf);
    
    ut_same("{nil:null yes:true no:false int:12345 array:[-23 1.230000 string 123e4567-e89b-12d3-a456-426655440000 2017-03-14T15:09:26.123456789Z ]}", buf,
	"incorrect output");
    opo_builder_cleanup(&builder);
}

void
append_msg_tests(utTest tests) {
    ut_appenda(tests, "opo.msg.iterate", iterate_test, NULL);
    
    // TBD tests for get
}
