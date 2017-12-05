// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "internal.h"
#include "opo.h"

#define NO_TIME		0xffffffffffffffffULL
#define TIME_STR_LEN	30
#define UUID_STR_LEN	36

typedef struct _StackNode {
    ojcVal	val;
    opoVal	end;
} *StackNode;

typedef struct _ParseCtx {
    ojcVal		top;
    struct _StackNode	stack[OPO_MSG_MAX_DEPTH];
    StackNode		cur;
    StackNode		end;
    const char		*key;
    int			klen;
} *ParseCtx;

static const char	uuid_char_map[257] = "\
................................\
.............-..hhhhhhhhhh......\
................................\
.hhhhhh.........................\
................................\
................................\
................................\
................................";

static const char	uuid_map[37] = "hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhh";

static bool
detect_uuid(const char *str, int len) {
    if (UUID_STR_LEN == len) {
	const char	*u = uuid_map;

	for (; 0 < len; len--, str++, u++) {
	    if (*u != uuid_char_map[*(uint8_t*)str]) {
		return false;
	    }
	}
	return true;
    }
    return false;
}

static const char*
read_num(const char *s, int len, int *vp) {
    uint32_t	v = 0;

    for (; 0 < len; len--, s++) {
	if ('0' <= *s && *s <= '9') {
	    v = v * 10 + *s - '0';
	} else {
	    return NULL;
	}
    }
    *vp = (int)v;

    return s;
}

static int64_t
time_parse(const char *s, int len) {
    struct tm	tm;
    long	nsecs = 0;
    int		i;
    time_t	secs;
    
    memset(&tm, 0, sizeof(tm));
    if (NULL == (s = read_num(s, 4, &tm.tm_year))) {
	return NO_TIME;
    }
    tm.tm_year -= 1900;
    if ('-' != *s) {
	return NO_TIME;
    }
    s++;
    if (NULL == (s = read_num(s, 2, &tm.tm_mon))) {
	return NO_TIME;
    }
    tm.tm_mon--;
    if ('-' != *s) {
	return NO_TIME;
    }
    s++;
    if (NULL == (s = read_num(s, 2, &tm.tm_mday))) {
	return NO_TIME;
    }
    if ('T' != *s) {
	return NO_TIME;
    }
    s++;
    if (NULL == (s = read_num(s, 2, &tm.tm_hour))) {
	return NO_TIME;
    }
    if (':' != *s) {
	return NO_TIME;
    }
    s++;
    if (NULL == (s = read_num(s, 2, &tm.tm_min))) {
	return NO_TIME;
    }
    if (':' != *s) {
	return NO_TIME;
    }
    s++;
    if (NULL == (s = read_num(s, 2, &tm.tm_sec))) {
	return NO_TIME;
    }
    if ('.' != *s) {
	return NO_TIME;
    }
    s++;
    for (i = 9; 0 < i; i--, s++) {
	if ('0' <= *s && *s <= '9') {
	    nsecs = nsecs * 10 + *s - '0';
	} else {
	    return NO_TIME;
	}
    }
    if ('Z' != *s) {
	return NO_TIME;
    }
    secs = (time_t)timegm(&tm);

    return (int64_t)secs * 1000000000ULL + (int64_t)nsecs;
}

static const uint8_t*
read_uint16(const uint8_t *b, uint16_t *nump) {
    uint16_t	num = (uint16_t)*b++;

    num = (num << 8) | (uint16_t)*b++;
    *nump = num;
    
    return b;
}

static const uint8_t*
read_uint32(const uint8_t *b, uint32_t *nump) {
    const uint8_t	*end = b + 4;
    uint32_t		num = 0;

    for (; b < end; b++) {
	num = (num << 8) | (uint32_t)*b;
    }
    *nump = num;
    
    return b;
}

static const uint8_t*
read_uint64(const uint8_t *b, uint64_t *nump) {
    const uint8_t	*end = b + 8;
    uint64_t		num = 0;

    for (; b < end; b++) {
	num = (num << 8) | (uint64_t)*b;
    }
    *nump = num;
    
    return b;
}

static opoErrCode
push_parse_value(opoErr err, ParseCtx pc, ojcVal val) {
    if (pc->stack <= pc->cur) {
	ojcVal	parent = pc->cur->val;
	
	if (OJC_OBJECT == ojc_type(parent)) {
	    if (NULL == pc->key) {
		return opo_err_set(err, OPO_ERR_PARSE, "missing key");
	    }
	    ojc_object_nappend(NULL, parent, pc->key, pc->klen, val);
	    pc->key = NULL;
	    pc->klen = 0;
	} else { // array
	    ojc_array_append(NULL, parent, val);
	}
    } else {
	pc->top = val;
    }
    return OPO_ERR_OK;
}

static opoErrCode
ojc_builder(opoErr err, ojcVal val, opoBuilder b, const char *key, int klen) {
    switch (ojc_type(val)) {
    case OJC_NULL:
	opo_builder_push_null(err, b, key, klen);
	break;
    case OJC_TRUE:
	opo_builder_push_bool(err, b, true, key, klen);
	break;
    case OJC_FALSE:
	opo_builder_push_bool(err, b, false, key, klen);
	break;
    case OJC_STRING: {
	const char	*str = ojc_str(NULL, val);
	int64_t		t;
	int		len = ojc_str_len(NULL, val);
	
	if (UUID_STR_LEN == len && detect_uuid(str, UUID_STR_LEN)) {
	    opo_builder_push_uuid_string(err, b, str, key, klen);
	} else if (TIME_STR_LEN == len && NO_TIME != (t = time_parse(str, TIME_STR_LEN))) {
	    opo_builder_push_time(err, b, t, key, klen);
	} else {
	    opo_builder_push_string(err, b, str, len, key, klen);
	}
	break;
    }
    case OJC_NUMBER: {
	const char	*str = ojc_number(NULL, val);
	int		len = ojc_number_len(NULL, val);

	opo_builder_push_string(err, b, str, len, key, klen);
	break;
    }
    case OJC_FIXNUM:
	opo_builder_push_int(err, b, ojc_int(NULL, val), key, klen);
	break;
    case OJC_DECIMAL:
	opo_builder_push_double(err, b, ojc_double(NULL, val), key, klen);
	break;
    case OJC_ARRAY:
	opo_builder_push_array(err, b, key, klen);
	for (ojcVal m = ojc_members(NULL, val); NULL != m; m = ojc_next(m)) {
	    ojc_builder(err, m, b, NULL, 0);
	}
	opo_builder_pop(err, b);
	break;
    case OJC_OBJECT:
	opo_builder_push_object(err, b, key, klen);
	for (ojcVal m = ojc_members(NULL, val); NULL != m; m = ojc_next(m)) {
	    ojc_builder(err, m, b, ojc_key(m), ojc_key_len(m));
	}
	opo_builder_pop(err, b);
	break;
    case OJC_WORD:
	opo_builder_push_string(err, b, ojc_word(NULL, val), -1, key, klen);
	break;
    default:
	break;
    }
    return err->code;
}

opoErrCode
opo_ojc_fill_msg(opoErr err, ojcVal val, uint8_t *buf, size_t size, opoDict dict) {
    struct _opoBuilder	b;
    
    opo_builder_init(err, &b, buf, size);
    b.dict = dict;

    ojc_builder(err, val, &b, NULL, 0);

    return err->code;
}

opoMsg
opo_ojc_to_msg(opoErr err, ojcVal val, opoDict dict) {
    if (NULL == val) {
	return NULL;
    }
    struct _opoBuilder	b;
    
    opo_builder_init(err, &b, NULL, 0);
    b.dict = dict;
    
    opoMsg	msg = NULL;
    
    if (OPO_ERR_OK == ojc_builder(err, val, &b, NULL, 0)) {
	msg = opo_builder_take(&b);
    }
    opo_builder_cleanup(&b);

    return msg;
}

// Did not use the callbacks as that adds 10% to 20% over this more direct
// approach.
ojcVal
opo_msg_to_ojc(opoErr err, opoVal msg, opoDict dict) {
    return opo_val_to_ojc(err, msg + 8, dict);
}

ojcVal
opo_val_to_ojc(opoErr err, opoVal val, opoDict dict) {
    const uint8_t	*end = val + opo_val_bsize(val);
    struct _ParseCtx	ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.top = NULL;
    ctx.cur = ctx.stack - 1;
    ctx.end = ctx.stack + sizeof(ctx.stack) / sizeof(*ctx.stack);

    while (val < end) {
	if (ctx.stack <= ctx.cur && ctx.cur->end <= val) {
	    ctx.cur--;
	}
	switch (*val++) {
	case VAL_NULL:
	    push_parse_value(err, &ctx, ojc_create_null());
	    break;
	case VAL_TRUE:
	    push_parse_value(err, &ctx, ojc_create_bool(true));
	    break;
	case VAL_FALSE:
	    push_parse_value(err, &ctx, ojc_create_bool(false));
	    break;
	case VAL_INT1:
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int8_t)*val));
	    val++;
	    break;
	case VAL_INT2: {
	    uint16_t	num;;

	    val = read_uint16(val, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int16_t)num));
	    break;
	}
	case VAL_INT4: {
	    uint32_t	num;;

	    val = read_uint32(val, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int32_t)num));
	    break;
	}
	case VAL_INT8: {
	    uint64_t	num;

	    val = read_uint64(val, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)num));
	    break;
	}
	case VAL_STR1: {
	    uint8_t	len = *val++;

	    push_parse_value(err, &ctx, ojc_create_str((const char*)val, (int)len));
	    val += len + 1;
	    break;
	}
	case VAL_STR2: {
	    uint16_t	len;
	    
	    val = read_uint16(val, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)val, (int)len));
	    val += len + 1;
	    break;
	}
	case VAL_STR4: {
	    uint32_t	len;
	    
	    val = read_uint32(val, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)val, (int)len));
	    val += len + 1;
	    break;
	}
	case VAL_KEY1: {
	    uint8_t	len = *val++;

	    ctx.key = (const char*)val;
	    ctx.klen = len;
	    val += len + 1;
	    break;
	}
	case VAL_KEY2: {
	    uint16_t	len;
	    
	    val = read_uint16(val, &len);
	    ctx.key = (const char*)val;
	    ctx.klen = len;
	    val += len + 1;
	    break;
	}
	case VAL_DEC: {
	    uint8_t	len = *val++;
	    char	buf[256];
	    double	d;
		
	    memcpy(buf, val, (size_t)len);
	    buf[len] = '\0';
	    d = strtod(buf, NULL);
	    push_parse_value(err, &ctx, ojc_create_double(d));
	    val += len;
	    break;
	}
	case VAL_UUID: {
	    uint64_t	hi;
	    uint64_t	lo;
	    char	buf[40];
	    
	    val = read_uint64(val, &hi);
	    val = read_uint64(val, &lo);
	    sprintf(buf, "%08lx-%04lx-%04lx-%04lx-%012lx",
		    (unsigned long)(hi >> 32),
		    (unsigned long)((hi >> 16) & 0x000000000000FFFFUL),
		    (unsigned long)(hi & 0x000000000000FFFFUL),
		    (unsigned long)(lo >> 48),
		    (unsigned long)(lo & 0x0000FFFFFFFFFFFFUL));

	    push_parse_value(err, &ctx, ojc_create_str(buf, UUID_STR_LEN));
	    break;
	}
	case VAL_TIME: {
	    uint64_t	nsec;
	    
	    val = read_uint64(val, &nsec);
	    char	buf[64];
	    struct tm	tm;
	    time_t	t = (time_t)(nsec / 1000000000LL);
	    long	frac = nsec - (int64_t)t * 1000000000LL;

	    if (NULL == gmtime_r(&t, &tm)) {
		opo_err_set(err, OPO_ERR_PARSE, "invalid time");
		break;
	    }
	    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d.%09ldZ",
		    1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
		    tm.tm_hour, tm.tm_min, tm.tm_sec, frac);
	    push_parse_value(err, &ctx, ojc_create_str(buf, TIME_STR_LEN));
	    break;
	}
	case VAL_OBJ1:
	    if (ctx.end - 1 <= ctx.cur) {
		opo_err_set(err, OPO_ERR_PARSE, "too deeply nested");
	    } else {
		ojcVal	obj = ojc_create_object();
		uint8_t	size = *val++;

		if (OPO_ERR_OK == push_parse_value(err, &ctx, obj)) {
		    ctx.cur++;
		    ctx.cur->val = obj;
		    ctx.cur->end = val + size;
		}
	    }
	    break;
	case VAL_OBJ2:
	    if (ctx.end - 1 <= ctx.cur) {
		opo_err_set(err, OPO_ERR_PARSE, "too deeply nested");
	    } else {
		ojcVal		obj = ojc_create_object();
		uint16_t	size;

		val = read_uint16(val, &size);
		if (OPO_ERR_OK == push_parse_value(err, &ctx, obj)) {
		    ctx.cur++;
		    ctx.cur->val = obj;
		    ctx.cur->end = val + size;
		}
	    }
	    break;
	case VAL_OBJ4:
	    if (ctx.end - 1 <= ctx.cur) {
		opo_err_set(err, OPO_ERR_PARSE, "too deeply nested");
	    } else {
		ojcVal		obj = ojc_create_object();
		uint32_t	size;

		val = read_uint32(val, &size);
		if (OPO_ERR_OK == push_parse_value(err, &ctx, obj)) {
		    ctx.cur++;
		    ctx.cur->val = obj;
		    ctx.cur->end = val + size;
		}
	    }
	    break;
	case VAL_ARRAY1:
	    if (ctx.end - 1 <= ctx.cur) {
		opo_err_set(err, OPO_ERR_PARSE, "too deeply nested");
	    } else {
		ojcVal	array = ojc_create_array();
		uint8_t	size = *val++;

		if (OPO_ERR_OK == push_parse_value(err, &ctx, array)) {
		    ctx.cur++;
		    ctx.cur->val = array;
		    ctx.cur->end = val + size;
		}
	    }
	    break;
	case VAL_ARRAY2:
	    if (ctx.end - 1 <= ctx.cur) {
		opo_err_set(err, OPO_ERR_PARSE, "too deeply nested");
	    } else {
		ojcVal	array = ojc_create_array();
		uint16_t	size;

		val = read_uint16(val, &size);
		if (OPO_ERR_OK == push_parse_value(err, &ctx, array)) {
		    ctx.cur++;
		    ctx.cur->val = array;
		    ctx.cur->end = val + size;
		}
	    }
	    break;
	case VAL_ARRAY4:
	    if (ctx.end - 1 <= ctx.cur) {
		opo_err_set(err, OPO_ERR_PARSE, "too deeply nested");
	    } else {
		ojcVal	array = ojc_create_array();
		uint32_t	size;

		val = read_uint32(val, &size);
		if (OPO_ERR_OK == push_parse_value(err, &ctx, array)) {
		    ctx.cur++;
		    ctx.cur->val = array;
		    ctx.cur->end = val + size;
		}
	    }
	    break;
	case VAL_DICS: {
	    if (NULL == dict) {
		opo_err_set(err, OPO_ERR_PARSE, "dictionary required");
		break;
	    }
	    opoDictEntry	de = &dict->entries[*val];

	    if (NULL == de->str) {
		opo_err_set(err, OPO_ERR_PARSE, "dictionary entry missing");
		break;
	    }
	    push_parse_value(err, &ctx, ojc_create_str((const char*)de->str, (int)de->len));
	    val++;
	    break;
	}
	case VAL_DICK: {
	    if (NULL == dict) {
		opo_err_set(err, OPO_ERR_PARSE, "dictionary required");
		break;
	    }
	    opoDictEntry	de = &dict->entries[*val];

	    if (NULL == de->str) {
		opo_err_set(err, OPO_ERR_PARSE, "dictionary entry missing");
		break;
	    }
	    ctx.key = (const char*)de->str;
	    ctx.klen = de->len;
	    val++;
	    break;
	}
	default:
	    err->code = OJC_PARSE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "corrupt message format");
	    break;
	}
	if (OJC_OK != err->code) {
	    break;
	}
    }
    return ctx.top;
}
