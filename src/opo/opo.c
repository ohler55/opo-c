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

static int
isize_bytes(int64_t n) {
    int	cnt = 0;
    
    if (-128 <= n && n <= 127) {
	cnt = 1;
    } else if (-32768 <= n && n <= 32767) {
	cnt = 2;
    } else if (-2147483648 <= n && n <= 2147483647) {
	cnt = 4;
    } else {
	cnt = 8;
    }
    return cnt;
}

static int
size_bytes(uint64_t n) {
    int	cnt = 0;
    
    if (n <= 0x00000000000000ffULL) {
	cnt = 1;
    } else if (n <= 0x000000000000ffffULL) {
	cnt = 2;
    } else if (n <= 0x00000000ffffffffULL) {
	cnt = 4;
    } else {
	cnt = 8;
    }
    return cnt;
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

static uint8_t*
fill_uint16(uint8_t *b, uint16_t n) {
    *b++ = (uint8_t)(n >> 8);
    *b++ = (uint8_t)(0x00ff & n);
    return b;
}

static uint8_t*
fill_uint32(uint8_t *b, uint32_t n) {
    *b++ = (uint8_t)(n >> 24);
    *b++ = (uint8_t)(0x000000ff & (n >> 16));
    *b++ = (uint8_t)(0x000000ff & (n >> 8));
    *b++ = (uint8_t)(0x000000ff & n);
    return b;
}

static uint8_t*
fill_uint64(uint8_t *b, uint64_t n) {
    *b++ = (uint8_t)(n >> 56);
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 48));
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 40));
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 32));
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 24));
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 16));
    *b++ = (uint8_t)(0x00000000000000ff & (n >> 8));
    *b++ = (uint8_t)(0x00000000000000ff & n);
    return b;
}

static uint8_t*
fill_str(uint8_t *w, const char *str, size_t len) {
    if (len <= (size_t)0x000000ff) {
	*w++ = VAL_STR1;
	*w++ = (uint8_t)len;
    } else if (len <= (size_t)0x0000ffff) {
	*w++ = VAL_STR2;
	w = fill_uint16(w, (uint16_t)len);
    } else {
	*w++ = VAL_STR4;
	w = fill_uint32(w, (uint32_t)len);
    }
    memcpy(w, str, len);
    w += len;
    *w++ = '\0';

    return w;
}

static uint8_t*
fill_key(uint8_t *w, const char *str, size_t len) {
    if (len <= (size_t)0x000000ff) {
	*w++ = VAL_KEY1;
	*w++ = (uint8_t)len;
    } else {
	*w++ = VAL_KEY2;
	w = fill_uint16(w, (uint16_t)len);
    }
    memcpy(w, str, len);
    w += len;
    *w++ = '\0';

    return w;
}

static uint8_t*
fill_uuid(uint8_t *w, const char *str) {
    const char	*s = str;
    const char	*end = str + UUID_STR_LEN;
    int		digits = 0;
    uint8_t	b = 0;
	    
    *w++ = VAL_UUID;
    for (; s < end; s++) {
	if ('0' <= *s && *s <= '9') {
	    b = (b << 4) | (*s - '0');
	} else if ('a' <= *s && *s <= 'f') {
	    b = (b << 4) | (*s - 'a' + 10);
	} else {
	    continue;
	}
	if (0 == digits) {
	    digits++;
	} else {
	    digits = 0;
	    *w++ = b;
	    b = 0;
	}
    }
    return w;
}

static size_t
wire_size(ojcVal val) {
    int64_t	size = 1;
    int64_t	len;

    switch (ojc_type(val)) {
    case OJC_NULL:
    case OJC_TRUE:
    case OJC_FALSE:
	break;
    case OJC_STRING: {
	const char	*str = ojc_str(NULL, val);
	
	len = ojc_str_len(NULL, val);
	if (UUID_STR_LEN == len && detect_uuid(str, UUID_STR_LEN)) {
	    size += 16;
	} else if (TIME_STR_LEN == len && NO_TIME != time_parse(str, TIME_STR_LEN)) {
	    size += 8;
	} else {
	    size += (int64_t)len + size_bytes((int64_t)len) + 1;
	}
	break;
    }
    case OJC_NUMBER:
	len = ojc_number_len(NULL, val);
	
	size += (int64_t)len + size_bytes((int64_t)len) + 1;
	break;
    case OJC_FIXNUM:
	size += isize_bytes(ojc_int(NULL, val));
	break;
    case OJC_DECIMAL: {
	char	str[64];
	double	d = ojc_double(NULL, val);
	
	size++;
	if (d == (double)(int64_t)d) {
	    size += snprintf(str, sizeof(str) - 1, "%.1f", d);
	} else {
	    size += snprintf(str, sizeof(str) - 1, "%0.15g", d);
	}
	break;
    }
    case OJC_ARRAY:
	size += 4;
	for (ojcVal m = ojc_members(NULL, val); NULL != m; m = ojc_next(m)) {
	    size += wire_size(m);
	}
	break;
    case OJC_OBJECT:
	size += 4;
	for (ojcVal m = ojc_members(NULL, val); NULL != m; m = ojc_next(m)) {
	    len = ojc_key_len(m);
	    size += len + size_bytes(len) + 2;
	    size += wire_size(m);
	}
	break;
    case OJC_WORD: {
	const char	*str = ojc_word(NULL, val);
	
	len = strlen(str);
	size += len + size_bytes(len) + 1;
	break;
    }
    default:
	break;
    }
    return (size_t)size;
}

static uint8_t*
wire_fill(ojcVal val, uint8_t *w) {
    switch (ojc_type(val)) {
    case OJC_NULL:
	*w++ = VAL_NULL;
	break;
    case OJC_TRUE:
	*w++ = VAL_TRUE;
	break;
    case OJC_FALSE:
	*w++ = VAL_FALSE;
	break;
    case OJC_STRING: {
	const char	*str = ojc_str(NULL, val);
	int64_t		t;
	int		len = ojc_str_len(NULL, val);
	
	if (UUID_STR_LEN == len && detect_uuid(str, UUID_STR_LEN)) {
	    w = fill_uuid(w, str);
	} else if (TIME_STR_LEN == len && NO_TIME != (t = time_parse(str, TIME_STR_LEN))) {
	    *w++ = VAL_TIME;
	    w = fill_uint64(w, (uint64_t)t);
	} else {
	    w = fill_str(w, str, len);
	}
	break;
    }
    case OJC_NUMBER: {
	const char	*str = ojc_number(NULL, val);
	int		len = ojc_number_len(NULL, val);

	w = fill_str(w, str, len);
	break;
    }
    case OJC_FIXNUM: {
	int64_t	i = ojc_int(NULL, val);
	
	if (-128 <= i && i <= 127) {
	    *w++ = VAL_INT1;
	    *w++ = (uint8_t)(int8_t)i;
	} else if (-32768 <= i && i <= 32767) {
	    *w++ = VAL_INT2;
	    w = fill_uint16(w, (uint16_t)(int16_t)i);
	} else if (-2147483648 <= i && i <= 2147483647) {
	    *w++ = VAL_INT4;
	    w = fill_uint32(w, (uint32_t)(int32_t)i);
	} else {
	    *w++ = VAL_INT8;
	    w = fill_uint64(w, (uint64_t)i);
	}
	break;
    }
    case OJC_DECIMAL: {
	char	str[64];
	int	cnt;
	double	d = ojc_double(NULL, val);
	
	*w++ = VAL_DEC;
	if (d == (double)(int64_t)d) {
	    cnt = snprintf(str, sizeof(str) - 1, "%.1f", d);
	} else {
	    cnt = snprintf(str, sizeof(str) - 1, "%0.15g", d);
	}
	*w++ = (uint8_t)cnt;
	memcpy(w, str, cnt);
	w += cnt;
	break;
    }
    case OJC_ARRAY: {
	*w++ = VAL_ARRAY;
	uint8_t	*start = w;

	w += 4;
	for (ojcVal m = ojc_members(NULL, val); NULL != m; m = ojc_next(m)) {
	    w = wire_fill(m, w);
	}
	fill_uint32(start, (uint32_t)(w - start - 4));
	break;
    }
    case OJC_OBJECT: {
	*w++ = VAL_OBJ;
	uint8_t		*start = w;
	const char	*key;
	int		len;
	
	w += 4;
	for (ojcVal m = ojc_members(NULL, val); NULL != m; m = ojc_next(m)) {
	    key = ojc_key(m);
	    len = ojc_key_len(m);
	    w = fill_key(w, key, len);
	    w = wire_fill(m, w);
	}
	fill_uint32(start, (uint32_t)(w - start - 4));
	break;
    }
    case OJC_WORD: {
	const char	*str = ojc_word(NULL, val);
	
	w = fill_str(w, str, strlen(str));
	break;
    }
    default:
	break;
    }
    return w;
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

size_t
opo_ojc_msg_size(ojcVal msg) {
    if (NULL == msg) {
	return 0;
    }
    return wire_size(msg) + 8;
}

void
opo_ojc_fill_msg(ojcVal val, uint8_t *buf) {
    memset(buf, 0, 8);
    wire_fill(val, buf + 8);
}

opoVal
opo_ojc_to_msg(opoErr err, ojcVal val) {
    if (NULL == val) {
	return NULL;
    }
    size_t	size = wire_size(val);
    uint8_t	*w = (uint8_t*)malloc(size + 8);

    if (NULL == w) {
	opo_err_set(err, OPO_ERR_MEMORY, "failed to allocate memory for a message %ld bytes long", size);
    } else {
	memset(w, 0, 8);
	wire_fill(val, w + 8);
    }
    return (opoMsg)w;
}

// Did not use the callbacks as that adds 10% to 20% over this more direct
// approach.
ojcVal
opo_msg_to_ojc(opoErr err, opoVal msg) {
    const uint8_t	*end = msg + opo_msg_bsize(msg);
    struct _ParseCtx	ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.top = NULL;
    ctx.cur = ctx.stack - 1;
    ctx.end = ctx.stack + sizeof(ctx.stack) / sizeof(*ctx.stack);

    msg += 8; // move past msg ID
    while (msg < end) {
	if (ctx.stack <= ctx.cur && ctx.cur->end <= msg) {
	    ctx.cur--;
	}
	switch (*msg++) {
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
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int8_t)*msg));
	    msg++;
	    break;
	case VAL_INT2: {
	    uint16_t	num;;

	    msg = read_uint16(msg, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int16_t)num));
	    break;
	}
	case VAL_INT4: {
	    uint32_t	num;;

	    msg = read_uint32(msg, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int32_t)num));
	    break;
	}
	case VAL_INT8: {
	    uint64_t	num;

	    msg = read_uint64(msg, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)num));
	    break;
	}
	case VAL_STR1: {
	    uint8_t	len = *msg++;

	    push_parse_value(err, &ctx, ojc_create_str((const char*)msg, (int)len));
	    msg += len + 1;
	    break;
	}
	case VAL_STR2: {
	    uint16_t	len;
	    
	    msg = read_uint16(msg, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)msg, (int)len));
	    msg += len + 1;
	    break;
	}
	case VAL_STR4: {
	    uint32_t	len;
	    
	    msg = read_uint32(msg, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)msg, (int)len));
	    msg += len + 1;
	    break;
	}
	case VAL_KEY1: {
	    uint8_t	len = *msg++;

	    ctx.key = (const char*)msg;
	    ctx.klen = len;
	    msg += len + 1;
	    break;
	}
	case VAL_KEY2: {
	    uint16_t	len;
	    
	    msg = read_uint16(msg, &len);
	    ctx.key = (const char*)msg;
	    ctx.klen = len;
	    msg += len + 1;
	    break;
	}
	case VAL_DEC: {
	    uint8_t	len = *msg++;
	    char	buf[256];
	    double	d;
		
	    memcpy(buf, msg, (size_t)len);
	    buf[len] = '\0';
	    d = strtod(buf, NULL);
	    push_parse_value(err, &ctx, ojc_create_double(d));
	    msg += len;
	    break;
	}
	case VAL_UUID: {
	    uint64_t	hi;
	    uint64_t	lo;
	    char	buf[40];
	    
	    msg = read_uint64(msg, &hi);
	    msg = read_uint64(msg, &lo);
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
	    
	    msg = read_uint64(msg, &nsec);
	    char	buf[64];
	    struct tm	tm;
	    time_t	t = (time_t)(nsec / 1000000000LL);
	    long	frac = nsec - (int64_t)t * 1000000000LL;

	    if (NULL == gmtime_r(&t, &tm)) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "invalid time");
		break;
	    }
	    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d.%09ldZ",
		    1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
		    tm.tm_hour, tm.tm_min, tm.tm_sec, frac);
	    push_parse_value(err, &ctx, ojc_create_str(buf, TIME_STR_LEN));
	    break;
	}
	case VAL_OBJ:
	    if (ctx.end - 1 <= ctx.cur) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "too deeply nested");
	    } else {
		ojcVal		obj = ojc_create_object();
		uint32_t	size;

		msg = read_uint32(msg, &size);
		if (OPO_ERR_OK == push_parse_value(err, &ctx, obj)) {
		    ctx.cur++;
		    ctx.cur->val = obj;
		    ctx.cur->end = msg + size;
		}
	    }
	    break;
	case VAL_ARRAY:
	    if (ctx.end - 1 <= ctx.cur) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "too deeply nested");
	    } else {
		ojcVal	array = ojc_create_array();
		uint32_t	size;

		msg = read_uint32(msg, &size);
		if (OPO_ERR_OK == push_parse_value(err, &ctx, array)) {
		    ctx.cur++;
		    ctx.cur->val = array;
		    ctx.cur->end = msg + size;
		}
	    }
	    break;
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
