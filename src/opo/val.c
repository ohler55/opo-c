// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "internal.h"
#include "val.h"

#define MSG_INC	4096
#define MAX_STACK_BUF	4096
#define MIN_MSG_BUF	1024
#define UUID_STR_LEN	36

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

opoErrCode
opo_val_iterate(opoErr err, opoVal val, opoMsgCallbacks callbacks, void *ctx) {
    // TBD no end available, terminate on last pop or when top is set and stack is empty
    const uint8_t	*end = val + opo_val_size(val);
    
    while (val < end) {
	switch (*val++) {
	case VAL_NULL:
	    if (NULL != callbacks->null) {
		callbacks->null(err, ctx);
	    }
	    break;
	case VAL_TRUE:
	    if (NULL != callbacks->boolean) {
		callbacks->boolean(err, true, ctx);
	    }
	    break;
	case VAL_FALSE:
	    if (NULL != callbacks->boolean) {
		callbacks->boolean(err, false, ctx);
	    }
	    break;
	case VAL_INT1:
	    if (NULL != callbacks->fixnum) {
		callbacks->fixnum(err, (int64_t)(int8_t)*val, ctx);
	    }
	    val++;
	    break;
	case VAL_INT2:
	    if (NULL != callbacks->fixnum) {
		uint16_t	num;;

		val = read_uint16(val, &num);
		callbacks->fixnum(err, (int64_t)(int16_t)num, ctx);
	    } else {
		val += 2;
	    }
	    break;
	case VAL_INT4:
	    if (NULL != callbacks->fixnum) {
		uint32_t	num;;

		val = read_uint32(val, &num);
		callbacks->fixnum(err, (int64_t)(int32_t)num, ctx);
	    } else {
		val += 4;
	    }
	    break;
	case VAL_INT8:
	    if (NULL != callbacks->fixnum) {
		uint64_t	num;

		val = read_uint64(val, &num);
		callbacks->fixnum(err, (int64_t)num, ctx);
	    } else {
		val += 8;
	    }
	    break;
	case VAL_STR1: {
	    uint8_t	len = *val++;

	    if (NULL != callbacks->string) {
		callbacks->string(err, (const char*)val, (int)len, ctx);
	    }
	    val += len + 1;
	    break;
	}
	case VAL_STR2: {
	    uint16_t	len;
	    
	    val = read_uint16(val, &len);
	    if (NULL != callbacks->string) {
		callbacks->string(err, (const char*)val, (int)len, ctx);
	    }
	    val += len + 1;
	    break;
	}
	case VAL_STR4: {
	    uint32_t	len;
	    
	    val = read_uint32(val, &len);
	    if (NULL != callbacks->string) {
		callbacks->string(err, (const char*)val, (int)len, ctx);
	    }
	    val += len + 1;
	    break;
	}
	case VAL_KEY1: {
	    uint8_t	len = *val++;

	    if (NULL != callbacks->key) {
		callbacks->key(err, (const char*)val, (int)len, ctx);
	    }
	    val += len + 1;
	    break;
	}
	case VAL_KEY2: {
	    uint16_t	len;
	    
	    val = read_uint16(val, &len);
	    if (NULL != callbacks->key) {
		callbacks->key(err, (const char*)val, (int)len, ctx);
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
		callbacks->decimal(err, d, ctx);
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
		    callbacks->time(err, nsec, ctx);
		} else {
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
		    callbacks->time_str(err, buf, ctx);
		}
	    } else {
		val += 8;
	    }
	    break;
	case VAL_OBEG:
	    if (NULL != callbacks->begin_object) {
		callbacks->begin_object(err, ctx);
	    }
	    break;
	case VAL_OEND:
	    if (NULL != callbacks->end_object) {
		callbacks->end_object(err, ctx);
	    }
	    break;
	case VAL_ABEG:
	    if (NULL != callbacks->begin_array) {
		callbacks->begin_array(err, ctx);
	    }
	    break;
	case VAL_AEND:
	    if (NULL != callbacks->end_array) {
		callbacks->end_array(err, ctx);
	    }
	    break;
	default:
	    opo_err_set(err, OPO_ERR_PARSE, "corrupt message format");
	    break;
	}
	if (OPO_ERR_OK != err->code) {
	    break;
	}
    }
    return err->code;
}
