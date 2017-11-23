// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "internal.h"
#include "builder.h"

#define MSG_INC	4096
#define MAX_STACK_BUF	4096
#define MIN_MSG_BUF	1024
#define UUID_STR_LEN	36

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
    if (len <= 0xff) {
	*w++ = VAL_STR1;
	*w++ = (uint8_t)len;
    } else if (len <= 0xffff) {
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
    if (len <= 0xff) {
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

static opoErrCode
builder_assure(opoErr err, opoBuilder builder, size_t size) {
    if (builder->end <= builder->cur + size) {
	size_t	new_size = builder->end - builder->head + MSG_INC + size;
	int	off = builder->cur - builder->head;
	
	if (builder->own) {
	    builder->head = (uint8_t*)realloc(builder->head, new_size);
	} else {
	    builder->head = (uint8_t*)malloc(new_size);
	}
	if (NULL == builder->head) {
	    return opo_err_set(err, OPO_ERR_MEMORY, "memory allocation failed for size %lu", (unsigned long)new_size);
	}
	builder->end = builder->head + new_size;
	builder->cur = builder->head + off;
    }
    return OPO_ERR_OK;
}
    
static opoErrCode
builder_append_byte(opoErr err, opoBuilder builder, uint8_t b) {
    if (OPO_ERR_OK != builder_assure(err, builder, 1)) {
	return err->code;
    }
    *builder->cur++ = b;

    return OPO_ERR_OK;
}
    
static int
builder_push_str(opoErr err, opoBuilder builder, const char *str, size_t len) {
    if (OPO_ERR_OK != builder_assure(err, builder, len + 6)) {
	return err->code;
    }
    builder->cur = fill_str(builder->cur, str, len);

    return OPO_ERR_OK;
}
    
static int
builder_push_key(opoErr err, opoBuilder builder, const char *str, size_t len) {
    if (OPO_ERR_OK != builder_assure(err, builder, len + 4)) {
	return err->code;
    }
    builder->cur = fill_key(builder->cur, str, len);

    return OPO_ERR_OK;
}
    
opoErrCode
opo_builder_init(opoErr err, opoBuilder builder, uint8_t *buf, size_t size) {
    if (NULL == buf) {
	if (size < MIN_MSG_BUF) {
	    size = MIN_MSG_BUF;
	}
	if (NULL == (builder->head = (uint8_t*)malloc(size))) {
	    return opo_err_set(err, OPO_ERR_MEMORY, "memory allocation failed for size %lu", (unsigned long)size);
	}
	builder->own = true;
    } else {
	builder->head = buf;
	builder->own = false;
    }
    builder->end = builder->head + size;
    builder->cur = builder->head + 8; // past message ID
    memset(builder->head, 0, 8);      // message ID
    memset(builder->stack, 0, sizeof(builder->stack));
    builder->top = builder->stack - 1;
    
    return OPO_ERR_OK;
}

void
opo_builder_cleanup(opoBuilder builder) {
    if (builder->own) {
	free(builder->head);
    }
}

opoErrCode
opo_builder_finish(opoBuilder builder) {
    uint8_t	*start;

    for (; builder->stack <= builder->top; builder->top--) {
	start = builder->head + *builder->top;
	fill_uint32(start + 1, (uint32_t)(builder->cur - start - 5));
    }
    return 0;
}

size_t
opo_builder_length(opoBuilder builder) {
    return builder->cur - builder->head;
}

opoVal
opo_builder_take(opoBuilder builder) {
    uint8_t	*msg;

    opo_builder_finish(builder);
    if (builder->own) {
	msg = builder->head;
    } else {
	size_t	size = opo_msg_bsize(builder->head);

	if (NULL != (msg = (uint8_t*)malloc(size))) {
	    memcpy(msg, builder->head, size);
	}
    }
    builder->head = NULL;
    builder->cur = NULL;
    builder->end = NULL;
    
    return msg;
}

static opoErrCode
check_key(opoErr err, opoBuilder builder, const char *key, int klen) {
    if (builder->top < builder->stack) {
	if (builder->head + 8 < builder->cur) {
	    return opo_err_set(err, OPO_ERR_OVERFLOW, "only one element can be in a builder");
	}
	if (NULL != key) {
	    return opo_err_set(err, OPO_ERR_ARG, "only members of an object are keyed");
	}
    } else if (NULL != key) {
	// TBD check for obj
	if (VAL_OBJ != *(builder->head + *builder->top)) {
	    return opo_err_set(err, OPO_ERR_ARG, "only members of an object are keyed");
	}
	if (0 >= klen) {
	    klen = strlen(key);
	}
	builder_push_key(err, builder, key, klen);
    } else if (VAL_ARRAY != *(builder->head + *builder->top)) {
	return opo_err_set(err, OPO_ERR_ARG, "members of an object must be keyed");
    }
    return OPO_ERR_OK;
}

opoErrCode
opo_builder_push_object(opoErr err, opoBuilder builder, const char *key, int klen) {
    if (builder->stack + OPO_MSG_MAX_DEPTH <= builder->top) {
	return opo_err_set(err, OPO_ERR_OVERFLOW, "too deeply nested. Limit is %d", OPO_MSG_MAX_DEPTH);
    }
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    builder->top++;
    *builder->top = builder->cur - builder->head;

    if (OPO_ERR_OK != builder_assure(err, builder, 5)) {
	return err->code;
    }
    *builder->cur++ = VAL_OBJ;
    builder->cur += 4;

    return OPO_ERR_OK;
}

opoErrCode
opo_builder_push_array(opoErr err, opoBuilder builder, const char *key, int klen) {
    if (builder->stack + OPO_MSG_MAX_DEPTH <= builder->top) {
	return opo_err_set(err, OPO_ERR_OVERFLOW, "too deeply nested. Limit is %d", OPO_MSG_MAX_DEPTH);
    }
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    builder->top++;
    *builder->top = builder->cur - builder->head;
	
    *builder->cur++ = VAL_ARRAY;
    builder->cur += 4;

    return OPO_ERR_OK;
}

opoErrCode
opo_builder_pop(opoErr err, opoBuilder builder) {
    if (builder->stack > builder->top) {
	return opo_err_set(err, OPO_ERR_OVERFLOW, "nothing left to pop");
    }
    uint8_t	*start = builder->head + *builder->top;
    
    fill_uint32(start + 1, (uint32_t)(builder->cur - start - 5));
    builder->top--;

    return OPO_ERR_OK;
}

opoErrCode
opo_builder_push_null(opoErr err, opoBuilder builder, const char *key, int klen) {
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    return builder_append_byte(err, builder, VAL_NULL);
}

opoErrCode
opo_builder_push_bool(opoErr err, opoBuilder builder, bool value, const char *key, int klen) {
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    return builder_append_byte(err, builder, value ? VAL_TRUE : VAL_FALSE);
}

opoErrCode
opo_builder_push_int(opoErr err, opoBuilder builder, int64_t value, const char *key, int klen) {
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    if (-128 <= value && value <= 127) {
	if (OPO_ERR_OK != builder_assure(err, builder, 2)) {
	    return err->code;
	}
	*builder->cur++ = VAL_INT1;
	*builder->cur++ = (uint8_t)(int8_t)value;
    } else if (-32768 <= value && value <= 32767) {
	if (OPO_ERR_OK != builder_assure(err, builder, 3)) {
	    return err->code;
	}
	*builder->cur++ = VAL_INT2;
	builder->cur = fill_uint16(builder->cur, (uint16_t)(int16_t)value);
    } else if (-2147483648 <= value && value <= 2147483647) {
	if (OPO_ERR_OK != builder_assure(err, builder, 5)) {
	    return err->code;
	}
	*builder->cur++ = VAL_INT4;
	builder->cur = fill_uint32(builder->cur, (uint32_t)(int32_t)value);
    } else {
	if (OPO_ERR_OK != builder_assure(err, builder, 9)) {
	    return err->code;
	}
	*builder->cur++ = VAL_INT8;
	builder->cur = fill_uint64(builder->cur, (uint64_t)value);
    }
    return OPO_ERR_OK;
}

opoErrCode
opo_builder_push_double(opoErr err, opoBuilder builder, double value, const char *key, int klen) {
    char	str[64];
    int		cnt;
    
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    if (value == (double)(int64_t)value) {
	cnt = snprintf(str, sizeof(str) - 1, "%.1f", value);
    } else {
	cnt = snprintf(str, sizeof(str) - 1, "%0.15g", value);
    }
    if (OPO_ERR_OK != builder_assure(err, builder, cnt + 2)) {
	return err->code;
    }
    *builder->cur++ = VAL_DEC;
    *builder->cur++ = (uint8_t)cnt;
    memcpy(builder->cur, str, cnt);
    builder->cur += cnt;

    return OPO_ERR_OK;
}

opoErrCode
opo_builder_push_string(opoErr err, opoBuilder builder, const char *value, int len, const char *key, int klen) {
    if (0 >= len) {
	len = strlen(value);
    }
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    if (OPO_ERR_OK != builder_push_str(err, builder, value, len)) {
	return err->code;
    }
    return OPO_ERR_OK;
}

opoErrCode
opo_builder_push_uuid(opoErr err, opoBuilder builder, uint64_t hi, uint64_t lo, const char *key, int klen) {
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    if (OPO_ERR_OK != builder_assure(err, builder, 17)) {
	return err->code;
    }
    *builder->cur++ = VAL_UUID;
    builder->cur = fill_uint64(builder->cur, hi);
    builder->cur = fill_uint64(builder->cur, lo);

    return OPO_ERR_OK;
}

opoErrCode
opo_builder_push_uuid_string(opoErr err, opoBuilder builder, const char *value, const char *key, int klen) {
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    if (OPO_ERR_OK != builder_assure(err, builder, 17)) {
	return err->code;
    }
    builder->cur = fill_uuid(builder->cur, value);

    return OPO_ERR_OK;
}

opoErrCode
opo_builder_push_time(opoErr err, opoBuilder builder, int64_t value, const char *key, int klen) {
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    if (OPO_ERR_OK != builder_assure(err, builder, 9)) {
	return err->code;
    }
    *builder->cur++ = VAL_TIME;
    builder->cur = fill_uint64(builder->cur, (uint64_t)value);

    return OPO_ERR_OK;
}

opoErrCode
opo_builder_push_val(opoErr err, opoBuilder builder, opoVal value, const char *key, int klen) {
    if (OPO_ERR_OK != check_key(err, builder, key, klen)) {
	return err->code;
    }
    size_t	size = opo_val_bsize(value);

    if (OPO_ERR_OK != builder_assure(err, builder, size)) {
	return err->code;
    }
    memcpy(builder->cur, value, size);
    builder->cur += size;

    return OPO_ERR_OK;
}
