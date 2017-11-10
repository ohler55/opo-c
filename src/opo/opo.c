// Copyright 2017 by Peter Ohler, All Rights Reserved

#include "opo.h"

#define NO_TIME		0xffffffffffffffffULL
#define TIME_STR_LEN	30

#if 0

typedef struct _ParseCtx {
    ojcVal	top;
    ojcVal	stack[WIRE_STACK_SIZE];
    ojcVal	*cur;
    ojcVal	*end;
    const char	*key;
    int		klen;
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

static uint64_t
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

    return (uint64_t)secs * 1000000000ULL + (uint64_t)nsecs;
}

static int
wire_size(ojcVal val) {
    int64_t	size = 1;
    int64_t	len;

    switch (val->type) {
    case OJC_NULL:
    case OJC_TRUE:
    case OJC_FALSE:
	break;
    case OJC_STRING:
	if (UUID_STR_LEN == val->str_len && detect_uuid(val->str.bstr->ca, UUID_STR_LEN)) {
	    size += 16;
	} else if (TIME_STR_LEN == val->str_len && NO_TIME != time_parse(val->str.bstr->ca, TIME_STR_LEN)) {
	    size += 8;
	} else {
	    size += (int64_t)val->str_len + size_bytes((int64_t)val->str_len);
	}
	break;
    case OJC_NUMBER:
	size += (int64_t)val->str_len + size_bytes((int64_t)val->str_len);
	break;
    case OJC_FIXNUM:
	size += size_bytes(val->fixnum);
	break;
    case OJC_DECIMAL: {
	char	str[64];

	size++;
	if (val->dub == (double)(int64_t)val->dub) {
	    size += snprintf(str, sizeof(str) - 1, "%.1f", val->dub);
	} else {
	    size += snprintf(str, sizeof(str) - 1, "%0.15g", val->dub);
	}
	break;
    }
    case OJC_ARRAY:
	size++;
	for (ojcVal m = val->members.head; NULL != m; m = m->next) {
	    size += wire_size(m);
	}
	break;
    case OJC_OBJECT:
	size++;
	for (ojcVal m = val->members.head; NULL != m; m = m->next) {
	    if (KEY_BIG == (len = m->key_len)) {
		len = strlen(m->key.str);
	    } else if (KEY_NONE == len) {
		len = 0;
	    }
	    size += len + size_bytes(len) + 1;
	    size += wire_size(m);
	}
	break;
    case OJC_WORD:
	len = strlen(val->str.ca);
	size += len + size_bytes(len);
	break;
    default:
	break;
    }
    return (int)size;
}

static uint8_t*
wire_fill(ojcVal val, uint8_t *w) {
    switch (val->type) {
    case OJC_NULL:
	*w++ = WIRE_NULL;
	break;
    case OJC_TRUE:
	*w++ = WIRE_TRUE;
	break;
    case OJC_FALSE:
	*w++ = WIRE_FALSE;
	break;
    case OJC_STRING: {
	const char	*str;
	uint64_t	t;

	if ((int)sizeof(union _Bstr) <= val->str_len) {
	    str = val->str.str;
	} else if ((int)sizeof(val->str.ca) <= val->str_len) {
	    str = val->str.bstr->ca;
	} else {
	    str = val->str.ca;
	}
	if (UUID_STR_LEN == val->str_len && detect_uuid(val->str.bstr->ca, UUID_STR_LEN)) {
	    w = fill_uuid(w, str);
	} else if (TIME_STR_LEN == val->str_len && NO_TIME != (t = time_parse(val->str.bstr->ca, TIME_STR_LEN))) {
	    *w++ = WIRE_TIME;
	    w = fill_uint64(w, t);
	} else {
	    w = fill_str(w, str, val->str_len);
	}
	break;
    }
    case OJC_NUMBER:
	if ((int)sizeof(union _Bstr) <= val->str_len) {
	    w = fill_num(w, val->str.str, val->str_len);
	} else if ((int)sizeof(val->str.ca) <= val->str_len) {
	    w = fill_num(w, val->str.bstr->ca, val->str_len);
	} else {
	    w = fill_num(w, val->str.ca, val->str_len);
	}
	break;
    case OJC_FIXNUM:
	if (-128 <= val->fixnum && val->fixnum <= 127) {
	    *w++ = WIRE_INT1;
	    *w++ = (uint8_t)(int8_t)val->fixnum;
	} else if (-32768 <= val->fixnum && val->fixnum <= 32767) {
	    *w++ = WIRE_INT2;
	    w = fill_uint16(w, (uint16_t)(int16_t)val->fixnum);
	} else if (-2147483648 <= val->fixnum && val->fixnum <= 2147483647) {
	    *w++ = WIRE_INT4;
	    w = fill_uint32(w, (uint32_t)(int32_t)val->fixnum);
	} else {
	    *w++ = WIRE_INT8;
	    w = fill_uint64(w, (uint64_t)val->fixnum);
	}
	break;
    case OJC_DECIMAL: {
	char	str[64];
	int	cnt;

	*w++ = WIRE_DEC;
	if (val->dub == (double)(int64_t)val->dub) {
	    cnt = snprintf(str, sizeof(str) - 1, "%.1f", val->dub);
	} else {
	    cnt = snprintf(str, sizeof(str) - 1, "%0.15g", val->dub);
	}
	*w++ = (uint8_t)cnt;
	memcpy(w, str, cnt);
	w += cnt;
	break;
    }
    case OJC_ARRAY:
	*w++ = WIRE_ABEG;
	for (ojcVal m = val->members.head; NULL != m; m = m->next) {
	    w = wire_fill(m, w);
	}
	*w++ = WIRE_AEND;
	break;
    case OJC_OBJECT:
	*w++ = WIRE_OBEG;
	for (ojcVal m = val->members.head; NULL != m; m = m->next) {
	    if ((int)sizeof(union _Bstr) <= m->key_len) {
		w = fill_key(w, m->key.str, m->key_len);
	    } else if ((int)sizeof(m->key.ca) <= m->key_len) {
		w = fill_key(w, m->key.bstr->ca, m->key_len);
	    } else {
		w = fill_key(w, m->key.ca, m->key_len);
	    }
	    w = wire_fill(m, w);
	}
	*w++ = WIRE_OEND;
	break;
    case OJC_WORD:
	break;
    default:
	break;
    }
    return w;
}

int
ojc_wire_size(ojcVal val) {
    if (NULL == val) {
	return 0;
    }
    return wire_size(val) + 4;
}

uint8_t*
ojc_wire_write_mem(ojcErr err, ojcVal val) {
    if (NULL == val) {
	err->code = OJC_ARG_ERR;
	snprintf(err->msg, sizeof(err->msg), "NULL value");
	return NULL;
    }
    size_t	size = (size_t)wire_size(val);
    uint8_t	*wire = (uint8_t*)malloc(size + 4);

    if (NULL == wire) {
	err->code = OJC_MEMORY_ERR;
	snprintf(err->msg, sizeof(err->msg), "memory allocation failed for size %lu", (unsigned long)size );
	return NULL;
    }
    if ((int64_t)0xffffffffU < size) {
	err->code = OJC_ARG_ERR;
	snprintf(err->msg, sizeof(err->msg), "too large (%lu bytes), limit is %d bytes", (unsigned long)size, (int)0xffffffffU);
	return NULL;
    }
    ojc_wire_fill(val, wire, size + 4);

    return wire;
}

size_t
ojc_wire_fill(ojcVal val, uint8_t *wire, size_t size) {
    if (0 == size) {
	size = wire_size(val);
	if (0xffffffffU < size) {
	    return 0;
	}
    } else if (NULL == val) {
	return 0;
    } else {
	size -= 4;
    }
    wire_fill(val, fill_uint32(wire, (uint32_t)size));

    return size + 4;
}

int
ojc_wire_write_file(ojcErr err, ojcVal val, FILE *file) {
    return ojc_wire_write_fd(err, val, fileno(file));
}

int
ojc_wire_write_fd(ojcErr err, ojcVal val, int fd) {
    int		size = wire_size(val) + 4;

    if (MAX_STACK_BUF <= size) {
	uint8_t	*buf = (uint8_t*)malloc(size);

	ojc_wire_fill(val, buf, size);
	if (size != write(fd, buf, size)) {
	    err->code = OJC_WRITE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "write failed: %s", strerror(errno));
	}
	free(buf);
    } else {
	uint8_t	buf[MAX_STACK_BUF];

	ojc_wire_fill(val, buf, size);
	if (size != write(fd, buf, size)) {
	    err->code = OJC_WRITE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "write failed: %s", strerror(errno));
	}
    }
    return err->code;
}

///// parse

static int
push_parse_value(ojcErr err, ParseCtx pc, ojcVal val) {
    if (pc->stack <= pc->cur) {
	ojcVal	parent = *pc->cur;
	
	if (OJC_OBJECT == parent->type) {
	    if (NULL == pc->key) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "missing key");
		return err->code;
	    }
	    ojc_object_nappend(err, parent, pc->key, pc->klen, val);
	    pc->key = NULL;
	    pc->klen = 0;
	} else { // array
	    ojc_array_append(err, parent, val);
	}
    } else {
	pc->top = val;
    }
    return OJC_OK;
}

// Did not use the callbacks as that adds 10% to 20% over this more direct
// approach.
ojcVal
ojc_wire_parse(ojcErr err, const uint8_t *wire) {
    const uint8_t	*end = wire + ojc_wire_buf_size(wire);
    struct _ParseCtx	ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.top = NULL;
    ctx.cur = ctx.stack - 1;
    ctx.end = ctx.stack + WIRE_STACK_SIZE;
    
    wire += 4;
    while (wire < end) {
	switch (*wire++) {
	case WIRE_NULL:
	    push_parse_value(err, &ctx, ojc_create_null());
	    break;
	case WIRE_TRUE:
	    push_parse_value(err, &ctx, ojc_create_bool(true));
	    break;
	case WIRE_FALSE:
	    push_parse_value(err, &ctx, ojc_create_bool(false));
	    break;
	case WIRE_INT1:
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int8_t)*wire));
	    wire++;
	    break;
	case WIRE_INT2: {
	    uint16_t	num;;

	    wire = read_uint16(wire, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int16_t)num));
	    break;
	}
	case WIRE_INT4: {
	    uint32_t	num;;

	    wire = read_uint32(wire, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)(int32_t)num));
	    break;
	}
	case WIRE_INT8: {
	    uint64_t	num;

	    wire = read_uint64(wire, &num);
	    push_parse_value(err, &ctx, ojc_create_int((int64_t)num));
	    break;
	}
	case WIRE_STR1: {
	    uint8_t	len = *wire++;

	    push_parse_value(err, &ctx, ojc_create_str((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_STR2: {
	    uint16_t	len;
	    
	    wire = read_uint16(wire, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_STR4: {
	    uint32_t	len;
	    
	    wire = read_uint32(wire, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_KEY1: {
	    uint8_t	len = *wire++;

	    ctx.key = (const char*)wire;
	    ctx.klen = len;
	    wire += len;
	    break;
	}
	case WIRE_KEY2: {
	    uint16_t	len;
	    
	    wire = read_uint16(wire, &len);
	    ctx.key = (const char*)wire;
	    ctx.klen = len;
	    wire += len;
	    break;
	}
	case WIRE_KEY4: {
	    uint32_t	len;
	    
	    wire = read_uint32(wire, &len);
	    ctx.key = (const char*)wire;
	    ctx.klen = len;
	    wire += len;
	    break;
	}
	case WIRE_DEC: {
	    uint8_t	len = *wire++;
	    char	buf[256];
	    double	d;
		
	    memcpy(buf, wire, (size_t)len);
	    buf[len] = '\0';
	    d = strtod(buf, NULL);
	    push_parse_value(err, &ctx, ojc_create_double(d));
	    wire += len;
	    break;
	}
	case WIRE_NUM1: {
	    uint8_t	len = *wire++;

	    push_parse_value(err, &ctx, ojc_create_number((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_NUM2: {
	    uint16_t	len;
	    
	    wire = read_uint16(wire, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_NUM4: {
	    uint32_t	len;
	    
	    wire = read_uint32(wire, &len);
	    push_parse_value(err, &ctx, ojc_create_str((const char*)wire, (int)len));
	    wire += len;
	    break;
	}
	case WIRE_UUID: {
	    uint64_t	hi;
	    uint64_t	lo;
	    char	buf[40];
	    
	    wire = read_uint64(wire, &hi);
	    wire = read_uint64(wire, &lo);
	    sprintf(buf, "%08lx-%04lx-%04lx-%04lx-%012lx",
		    (unsigned long)(hi >> 32),
		    (unsigned long)((hi >> 16) & 0x000000000000FFFFUL),
		    (unsigned long)(hi & 0x000000000000FFFFUL),
		    (unsigned long)(lo >> 48),
		    (unsigned long)(lo & 0x0000FFFFFFFFFFFFUL));

	    push_parse_value(err, &ctx, ojc_create_str(buf, UUID_STR_LEN));
	    break;
	}
	case WIRE_TIME: {
	    uint64_t	nsec;
	    
	    wire = read_uint64(wire, &nsec);
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
	case WIRE_OBEG:
	    if (ctx.end - 1 <= ctx.cur) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "too deeply nested");
	    } else {
		ojcVal	obj = _ojc_val_create(OJC_OBJECT);

		if (OJC_OK == push_parse_value(err, &ctx, obj)) {
		    ctx.cur++;
		    *ctx.cur = obj;
		}
	    }
	    break;
	case WIRE_ABEG:
	    if (ctx.end - 1 <= ctx.cur) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "too deeply nested");
	    } else {
		ojcVal	array = _ojc_val_create(OJC_ARRAY);

		if (OJC_OK == push_parse_value(err, &ctx, array)) {
		    ctx.cur++;
		    *ctx.cur = array;
		}
	    }
	    break;
	case WIRE_OEND:
	case WIRE_AEND:
	    if (ctx.cur < ctx.stack) {
		err->code = OJC_PARSE_ERR;
		snprintf(err->msg, sizeof(err->msg), "too many closes");
	    } else {
		ctx.cur--;
	    }
	    break;
	default:
	    err->code = OJC_PARSE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "corrupt wire format");
	    break;
	}
	if (OJC_OK != err->code) {
	    break;
	}
    }
    return ctx.top;
}

ojcVal
ojc_wire_parse_file(ojcErr err, FILE *file) {
    return ojc_wire_parse_fd(err, fileno(file));
}

ojcVal
ojc_wire_parse_fd(ojcErr err, int fd) {
    off_t	here = lseek(fd, 0, SEEK_CUR);
    off_t	size = lseek(fd, 0, SEEK_END) - here;
    ojcVal	val = NULL;
    
    lseek(fd, here, SEEK_SET);
    if (MAX_STACK_BUF <= size) {
	uint8_t	*buf = (uint8_t*)malloc(size);

	if (size != read(fd, buf, size)) {
	    err->code = OJC_PARSE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "read failed: %s", strerror(errno));
	}
	val = ojc_wire_parse(err, buf);
	free(buf);
    } else {
	uint8_t	buf[MAX_STACK_BUF];

	if (size != read(fd, buf, size)) {
	    err->code = OJC_PARSE_ERR;
	    snprintf(err->msg, sizeof(err->msg), "read failed: %s", strerror(errno));
	}
	val = ojc_wire_parse(err, buf);
    }
    return val;
}

#endif

opoMsg
opo_ojc_to_msg(opoErr err, ojcVal val) {
    // TBD
    return NULL;
}

ojcVal
opo_msg_to_ojc(opoErr err, const opoMsg msg) {
    // TBD
    return NULL;
}

size_t
opo_ojc_msg_size(ojcVal val) {
    // TBD
    return 0;
}

