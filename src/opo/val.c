// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "internal.h"
#include "val.h"

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

size_t
opo_val_bsize(opoVal val) {
    size_t	size = 0;

    if (NULL != val) {
	switch (*val) {
	case VAL_NULL:	size = 1;	break;
	case VAL_TRUE:	size = 1;	break;
	case VAL_FALSE:	size = 1;	break;
	case VAL_INT1:	size = 2;	break;
	case VAL_INT2:	size = 3;	break;
	case VAL_INT4:	size = 5;	break;
	case VAL_INT8:	size = 9;	break;
	case VAL_STR1:
	    size = 3 + (size_t)*(val + 1);
	    break;
	case VAL_STR2: {
	    uint16_t	num;

	    read_uint16(val + 1, &num);
	    size = 4 + (size_t)num;
	    break;
	}
	case VAL_STR4: {
	    uint32_t	num;

	    read_uint32(val + 1, &num);
	    size = 6 + (size_t)num;
	    break;
	}
	case VAL_KEY1:
	    size = 3 + (size_t)*(val + 1);
	    break;
	case VAL_KEY2: {
	    uint16_t	num;

	    read_uint16(val + 1, &num);
	    size = 4 + (size_t)num;
	    break;
	}
	case VAL_DEC:
	    size = 2 + (size_t)*(val + 1);
	    break;
	case VAL_UUID:	size = 17;	break;
	case VAL_TIME:	size = 9;	break;
	case VAL_OBJ:
	case VAL_ARRAY: {
	    uint32_t	num;

	    read_uint32(val + 1, &num);
	    size = (size_t)num + 5;
	    break;
	}
	default:
	    break;
	}
    }
    return size;
}

size_t
opo_val_size(opoVal val) {
    size_t	size = 0;

    if (NULL != val) {
	switch (*val) {
	case VAL_INT1:	size = 1;	break;
	case VAL_INT2:	size = 2;	break;
	case VAL_INT4:	size = 4;	break;
	case VAL_INT8:	size = 8;	break;
	case VAL_STR1:
	    size = (size_t)*(val + 1);
	    break;
	case VAL_STR2: {
	    uint16_t	num;

	    read_uint16(val + 1, &num);
	    size = (size_t)num;
	    break;
	}
	case VAL_STR4: {
	    uint32_t	num;

	    read_uint32(val + 1, &num);
	    size = (size_t)num;
	    break;
	}
	case VAL_KEY1:
	    size = (size_t)*(val + 1);
	    break;
	case VAL_KEY2: {
	    uint16_t	num;

	    read_uint16(val + 1, &num);
	    size = (size_t)num;
	    break;
	}
	case VAL_DEC:
	    size = (size_t)*(val + 1);
	    break;
	case VAL_UUID:	size = 16;	break;
	case VAL_TIME:	size = 8;	break;
	default:
	    break;
	}
    }
    return size;
}

opoValType
opo_val_type(opoVal val) {
    opoValType	type = OPO_VAL_NONE;

    if (NULL != val) {
	switch (*val) {
	case VAL_NULL:	type = OPO_VAL_NULL;	break;
	case VAL_TRUE:	type = OPO_VAL_BOOL;	break;
	case VAL_FALSE:	type = OPO_VAL_BOOL;	break;
	case VAL_INT1:	type = OPO_VAL_INT;	break;
	case VAL_INT2:	type = OPO_VAL_INT;	break;
	case VAL_INT4:	type = OPO_VAL_INT;	break;
	case VAL_INT8:	type = OPO_VAL_INT;	break;
	case VAL_STR1:	type = OPO_VAL_STR;	break;
	case VAL_STR2:	type = OPO_VAL_STR;	break;
	case VAL_STR4:	type = OPO_VAL_STR;	break;
	case VAL_DEC:	type = OPO_VAL_DEC;	break;
	case VAL_UUID:	type = OPO_VAL_UUID;	break;
	case VAL_TIME:	type = OPO_VAL_TIME;	break;
	case VAL_OBJ:	type = OPO_VAL_OBJ;	break;
	case VAL_ARRAY:	type = OPO_VAL_ARRAY;	break;
	default:				break;
	}
    }
    return type;
}

static bool
val_iterate(opoErr err, opoVal val, opoValCallbacks callbacks, void *ctx, opoVal end) {
    bool	cont = true;

    while (val < end) {
	switch (*val++) {
	case VAL_NULL:
	    if (NULL != callbacks->null) {
		cont = callbacks->null(err, ctx);
	    }
	    break;
	case VAL_TRUE:
	    if (NULL != callbacks->boolean) {
		cont = callbacks->boolean(err, true, ctx);
	    }
	    break;
	case VAL_FALSE:
	    if (NULL != callbacks->boolean) {
		cont = callbacks->boolean(err, false, ctx);
	    }
	    break;
	case VAL_INT1:
	    if (NULL != callbacks->fixnum) {
		cont = callbacks->fixnum(err, (int64_t)(int8_t)*val, ctx);
	    }
	    val++;
	    break;
	case VAL_INT2:
	    if (NULL != callbacks->fixnum) {
		uint16_t	num;;

		val = read_uint16(val, &num);
		cont = callbacks->fixnum(err, (int64_t)(int16_t)num, ctx);
	    } else {
		val += 2;
	    }
	    break;
	case VAL_INT4:
	    if (NULL != callbacks->fixnum) {
		uint32_t	num;;

		val = read_uint32(val, &num);
		cont = callbacks->fixnum(err, (int64_t)(int32_t)num, ctx);
	    } else {
		val += 4;
	    }
	    break;
	case VAL_INT8:
	    if (NULL != callbacks->fixnum) {
		uint64_t	num;

		val = read_uint64(val, &num);
		cont = callbacks->fixnum(err, (int64_t)num, ctx);
	    } else {
		val += 8;
	    }
	    break;
	case VAL_STR1: {
	    uint8_t	len = *val++;

	    if (NULL != callbacks->string) {
		cont = callbacks->string(err, (const char*)val, (int)len, ctx);
	    }
	    val += len + 1;
	    break;
	}
	case VAL_STR2: {
	    uint16_t	len;

	    val = read_uint16(val, &len);
	    if (NULL != callbacks->string) {
		cont = callbacks->string(err, (const char*)val, (int)len, ctx);
	    }
	    val += len + 1;
	    break;
	}
	case VAL_STR4: {
	    uint32_t	len;

	    val = read_uint32(val, &len);
	    if (NULL != callbacks->string) {
		cont = callbacks->string(err, (const char*)val, (int)len, ctx);
	    }
	    val += len + 1;
	    break;
	}
	case VAL_KEY1: {
	    uint8_t	len = *val++;

	    if (NULL != callbacks->key) {
		cont = callbacks->key(err, (const char*)val, (int)len, ctx);
	    }
	    val += len + 1;
	    break;
	}
	case VAL_KEY2: {
	    uint16_t	len;

	    val = read_uint16(val, &len);
	    if (NULL != callbacks->key) {
		cont = callbacks->key(err, (const char*)val, (int)len, ctx);
	    }
	    val += len + 1;
	    break;
	}
	case VAL_DEC: {
	    uint8_t	len = *val++;

	    if (NULL != callbacks->decimal) {
		char	buf[256];
		double	d;

		memcpy(buf, val, (size_t)len);
		buf[len] = '\0';
		d = strtod(buf, NULL);
		cont = callbacks->decimal(err, d, ctx);
	    }
	    val += len;
	    break;
	}
	case VAL_UUID:
	    if (NULL != callbacks->uuid || NULL != callbacks->uuid_str) {
		uint64_t	hi;
		uint64_t	lo;

		val = read_uint64(val, &hi);
		val = read_uint64(val, &lo);
		if (NULL != callbacks->uuid) {
		    callbacks->uuid(err, hi, lo, ctx);
		} else {
		    char	buf[40];

		    sprintf(buf, "%08lx-%04lx-%04lx-%04lx-%012lx",
			    (unsigned long)(hi >> 32),
			    (unsigned long)((hi >> 16) & 0x000000000000FFFFUL),
			    (unsigned long)(hi & 0x000000000000FFFFUL),
			    (unsigned long)(lo >> 48),
			    (unsigned long)(lo & 0x0000FFFFFFFFFFFFUL));
		    callbacks->uuid_str(err, buf, ctx);
		}
	    } else {
		val += 16;
	    }
	    break;
	case VAL_TIME:
	    if (NULL != callbacks->time || NULL != callbacks->time_str) {
		uint64_t	nsec;

		val = read_uint64(val, &nsec);
		if (NULL != callbacks->time) {
		    callbacks->time(err, (int64_t)nsec, ctx);
		} else {
		    int64_t	ns = (int64_t)nsec;
		    char	buf[64];
		    struct tm	tm;
		    time_t	t = (time_t)(ns / 1000000000LL);
		    long	frac = ns - (int64_t)t * 1000000000LL;

		    if (frac < 0) {
			frac = -frac;
		    }
		    if (NULL == gmtime_r(&t, &tm)) {
			opo_err_set(err, OPO_ERR_PARSE, "invalid time");
			break;
		    }
		    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d.%09ldZ",
			    1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
			    tm.tm_hour, tm.tm_min, tm.tm_sec, frac);
		    callbacks->time_str(err, buf, ctx);
		}
	    } else {
		val += 8;
	    }
	    break;
	case VAL_OBJ:
	    if (NULL != callbacks->begin_object) {
		cont = callbacks->begin_object(err, ctx);
	    }
	    if (cont) {
		uint32_t	size;

		val = read_uint32(val, &size);
		cont = val_iterate(err, val, callbacks, ctx, val + size);
		val += size;
	    }
	    if (NULL != callbacks->end_object && cont) {
		cont = callbacks->end_object(err, ctx);
	    }
	    break;
	case VAL_ARRAY:
	    if (NULL != callbacks->begin_array) {
		cont = callbacks->begin_array(err, ctx);
	    }
	    if (cont) {
		uint32_t	size;

		val = read_uint32(val, &size);
		cont = val_iterate(err, val, callbacks, ctx, val + size);
		val += size;
	    }
	    if (NULL != callbacks->end_array && cont) {
		cont = callbacks->end_array(err, ctx);
	    }
	    break;
	default:
	    opo_err_set(err, OPO_ERR_PARSE, "corrupt message format");
	    break;
	}
	if (!cont || OPO_ERR_OK != err->code) {
	    break;
	}
    }

    return cont;
}

opoErrCode
opo_val_iterate(opoErr err, opoVal val, opoValCallbacks callbacks, void *ctx) {
    val_iterate(err, val, callbacks, ctx, val + 1);

    return err->code;
}

static const char*
val_key(opoVal val, int *lenp) {
    const char	*key = NULL;

    switch (*val++) {
    case VAL_KEY1:
	*lenp = (int)*val++;;
	key = (const char*)val;
	break;
    case VAL_KEY2: {
	uint16_t	len;

	val = read_uint16(val, &len);
	*lenp = (int)len;
	key = (const char*)val;
	break;
    }
    default:
	*lenp = 0;
	break;
    }
    return key;
}

opoVal
opo_val_get(opoVal val, const char *path) {
    if (NULL == path || '\0' == *path) {
	return val;
    }
    const char	*dot = path;
    uint32_t	size;
    opoVal	end;

    for (; '.' != *dot && '\0' != *dot; dot++) {
    }
    switch (*val++) {
    case VAL_OBJ: {
	const char	*key;
	int		len = dot - path;
	int		klen;

	val = read_uint32(val, &size);
	end = val + size;
	while (val < end) {
	    key = val_key(val, &klen);
	    val += opo_val_bsize(val);
	    if (len == klen && 0 == strncmp(key, path, len)) {
		if ('\0' == *dot) {
		    return val;
		}
		return opo_val_get(val, dot + 1);
	    }
	    val += opo_val_bsize(val);
	}
	break;
    }
    case VAL_ARRAY: {
	char	*iend;
	long	i = strtol(path, &iend, 10);

	if (iend != dot) {
	    break;
	}
	val = read_uint32(val, &size);
	end = val + size;
	for (; val < end; i--) {
	    if (0 == i) {
		if ('\0' == *dot) {
		    return val;
		}
		return opo_val_get(val, dot + 1);
	    }
	    val += opo_val_bsize(val);
	}
	break;
    }
    default:
	break;
    }
    return NULL;
}

bool
opo_val_bool(opoErr err, opoVal val) {
    if (NULL == val) {
	opo_err_set(err, OPO_ERR_TYPE, "NULL is not an boolean value");
	return 0;
    }
    bool	v = false;

    switch (*val++) {
    case VAL_TRUE:	v = true;	break;
    case VAL_FALSE:	v = false;	break;
    default:
	opo_err_set(err, OPO_ERR_TYPE, "not a boolean value");
	break;
    }
    return v;
}

int64_t
opo_val_int(opoErr err, opoVal val) {
    if (NULL == val) {
	opo_err_set(err, OPO_ERR_TYPE, "NULL is not an integer value");
	return 0;
    }
    int64_t	i = 0;

    switch (*val++) {
    case VAL_INT1:
	i = (int64_t)(int8_t)*val;
	break;
    case VAL_INT2: {
	uint16_t	num;;

	val = read_uint16(val, &num);
	i = (int64_t)(int16_t)num;
	break;
    }
    case VAL_INT4: {
	uint32_t	num;;

	val = read_uint32(val, &num);
	i = (int64_t)(int32_t)num;
	break;
    }
    case VAL_INT8: {
	uint64_t	num;

	val = read_uint64(val, &num);
	i = (int64_t)num;
	break;
    }
    default:
	opo_err_set(err, OPO_ERR_TYPE, "not an integer value");
	break;
    }
    return i;
}

double
opo_val_double(opoErr err, opoVal val) {
    if (NULL == val) {
	opo_err_set(err, OPO_ERR_TYPE, "NULL is not an double value");
	return 0.0;
    }
    if (VAL_DEC != *val) {
	opo_err_set(err, OPO_ERR_TYPE, "not a double value");
	return 0.0;
    }
    val++;
    uint8_t	len = *val++;
    char	buf[256];

    memcpy(buf, val, (size_t)len);
    buf[len] = '\0';

    return strtod(buf, NULL);
}

const char*
opo_val_string(opoErr err, opoVal val, int *lenp) {
    if (NULL == val) {
	opo_err_set(err, OPO_ERR_TYPE, "NULL is not an string value");
	return NULL;
    }
    const char	*str = NULL;

    switch (*val++) {
    case VAL_STR1: {
	uint8_t	len = *val++;

	if (NULL != lenp) {
	    *lenp = (int)len;
	}
	str = (const char*)val;
	break;
    }
    case VAL_STR2: {
	uint16_t	len;

	val = read_uint16(val, &len);
	if (NULL != lenp) {
	    *lenp = (int)len;
	}
	str = (const char*)val;
	break;
    }
    case VAL_STR4: {
	uint32_t	len;

	val = read_uint32(val, &len);
	if (NULL != lenp) {
	    *lenp = (int)len;
	}
	str = (const char*)val;
	break;
    }
    default:
	opo_err_set(err, OPO_ERR_TYPE, "not a string value");
	break;
    }
    return str;
}

const char*
opo_val_key(opoErr err, opoVal val, int *lenp) {
    if (NULL == val) {
	opo_err_set(err, OPO_ERR_TYPE, "NULL is not an string value");
	return NULL;
    }
    const char	*key = NULL;

    switch (*val++) {
    case VAL_KEY1:
	if (NULL != lenp) {
	    *lenp = (int)*val;
	}
	val++;
	key = (const char*)val;
	break;
    case VAL_STR2: {
	uint16_t	len;

	val = read_uint16(val, &len);
	if (NULL != lenp) {
	    *lenp = (int)len;
	}
	key = (const char*)val;
	break;
    }
    default:
	break;
    }
    return key;
}


void
opo_val_uuid_str(opoErr err, opoVal val, char *str) {
    if (NULL == val) {
	opo_err_set(err, OPO_ERR_TYPE, "NULL is not a UUID value");
    } else if (VAL_UUID != *val) {
	opo_err_set(err, OPO_ERR_TYPE, "not a UUID value");
    } else {
	uint64_t	hi;
	uint64_t	lo;

	val++;
	val = read_uint64(val, &hi);
	val = read_uint64(val, &lo);
	sprintf(str, "%08lx-%04lx-%04lx-%04lx-%012lx",
		(unsigned long)(hi >> 32),
		(unsigned long)((hi >> 16) & 0x000000000000FFFFUL),
		(unsigned long)(hi & 0x000000000000FFFFUL),
		(unsigned long)(lo >> 48),
		(unsigned long)(lo & 0x0000FFFFFFFFFFFFUL));
    }
}

void
opo_val_uuid(opoErr err, opoVal val, uint64_t *hip, uint64_t *lop) {
    if (NULL == val) {
	opo_err_set(err, OPO_ERR_TYPE, "NULL is not a UUID value");
	return;
    } else if (VAL_UUID != *val) {
	opo_err_set(err, OPO_ERR_TYPE, "not a UUID value");
    } else {
	val++;
	val = read_uint64(val, hip);
	val = read_uint64(val, lop);
    }
}

int64_t
opo_val_time(opoErr err, opoVal val) {
    uint64_t	t = 0;

    if (NULL == val) {
	opo_err_set(err, OPO_ERR_TYPE, "NULL is not a time value");
    } else if (VAL_TIME != *val) {
	opo_err_set(err, OPO_ERR_TYPE, "not a time value");
    } else {
	val++;
	read_uint64(val, &t);
    }
    return (int64_t)t;
}

opoVal
opo_val_members(opoErr err, opoVal val) {
    opoVal	members = NULL;

    if (NULL == val) {
	opo_err_set(err, OPO_ERR_TYPE, "NULL is not a object or array value");
    } else if (VAL_OBJ != *val && VAL_ARRAY != *val) {
	opo_err_set(err, OPO_ERR_TYPE, "not an object or array value");
    } else {
	members = val + 5;
    }
    return members;
}

opoVal
opo_val_next(opoVal val) {
    return val + opo_val_bsize(val);
}

int
opo_val_member_count(opoErr err, opoVal val) {
    if (NULL == val) {
	opo_err_set(err, OPO_ERR_TYPE, "NULL is not a object or array value");
	return 0;
    }
    int		cnt = 0;
    opoVal	end;
    uint32_t	size;

    switch (*val++) {
    case VAL_OBJ:
	val = read_uint32(val, &size);
	end = val + size;
	while (val < end) {
	    cnt++;
	    val += opo_val_bsize(val);
	    val += opo_val_bsize(val);
	}
	break;
    case VAL_ARRAY:
	val = read_uint32(val, &size);
	end = val + size;
	while (val < end) {
	    cnt++;
	    val += opo_val_bsize(val);
	}
	break;
    default:
	opo_err_set(err, OPO_ERR_TYPE, "not an object or array value");
	break;
    }
    return cnt;
}

size_t
opo_msg_bsize(opoMsg msg) {
    return opo_val_bsize(msg + 8) + 8;
}

uint64_t
opo_msg_id(opoMsg msg) {
    uint64_t	id;

    read_uint64(msg, &id);

    return id;
}

void
opo_msg_set_id(uint8_t *msg, uint64_t id) {
    *msg++ = (uint8_t)(id >> 56);
    *msg++ = (uint8_t)(0x00000000000000ff & (id >> 48));
    *msg++ = (uint8_t)(0x00000000000000ff & (id >> 40));
    *msg++ = (uint8_t)(0x00000000000000ff & (id >> 32));
    *msg++ = (uint8_t)(0x00000000000000ff & (id >> 24));
    *msg++ = (uint8_t)(0x00000000000000ff & (id >> 16));
    *msg++ = (uint8_t)(0x00000000000000ff & (id >> 8));
    *msg = (uint8_t)(0x00000000000000ff & id);
}
