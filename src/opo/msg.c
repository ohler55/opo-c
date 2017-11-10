// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "internal.h"
#include "msg.h"

#define MSG_INC	4096
#define MAX_STACK_BUF	4096
#define MIN_MSG_BUF	1024
#define UUID_STR_LEN	36

#if 0
static int
size_bytes(int64_t n) {
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
#endif

size_t
opo_msg_size(opoMsg msg) {
    size_t	size = 0;

    for (int i = 4; 0 < i; i--, msg++) {
	size = (size << 8) + (size_t)*msg;
    }
    return size + 4;
}

opoVal
opo_msg_top(opoMsg msg) {
    return msg + 4;
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

opoErrCode
opo_msg_iterate(opoErr err, opoMsg msg, opoMsgCallbacks callbacks, void *ctx) {
    const uint8_t	*end = msg + opo_msg_size(msg);
    
    msg += 4;
    while (msg < end) {
	switch (*msg++) {
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
		callbacks->fixnum(err, (int64_t)(int8_t)*msg, ctx);
	    }
	    msg++;
	    break;
	case VAL_INT2:
	    if (NULL != callbacks->fixnum) {
		uint16_t	num;;

		msg = read_uint16(msg, &num);
		callbacks->fixnum(err, (int64_t)(int16_t)num, ctx);
	    } else {
		msg += 2;
	    }
	    break;
	case VAL_INT4:
	    if (NULL != callbacks->fixnum) {
		uint32_t	num;;

		msg = read_uint32(msg, &num);
		callbacks->fixnum(err, (int64_t)(int32_t)num, ctx);
	    } else {
		msg += 4;
	    }
	    break;
	case VAL_INT8:
	    if (NULL != callbacks->fixnum) {
		uint64_t	num;

		msg = read_uint64(msg, &num);
		callbacks->fixnum(err, (int64_t)num, ctx);
	    } else {
		msg += 8;
	    }
	    break;
	case VAL_STR1: {
	    uint8_t	len = *msg++;

	    if (NULL != callbacks->string) {
		callbacks->string(err, (const char*)msg, (int)len, ctx);
	    }
	    msg += len + 1;
	    break;
	}
	case VAL_STR2: {
	    uint16_t	len;
	    
	    msg = read_uint16(msg, &len);
	    if (NULL != callbacks->string) {
		callbacks->string(err, (const char*)msg, (int)len, ctx);
	    }
	    msg += len + 1;
	    break;
	}
	case VAL_STR4: {
	    uint32_t	len;
	    
	    msg = read_uint32(msg, &len);
	    if (NULL != callbacks->string) {
		callbacks->string(err, (const char*)msg, (int)len, ctx);
	    }
	    msg += len + 1;
	    break;
	}
	case VAL_KEY1: {
	    uint8_t	len = *msg++;

	    if (NULL != callbacks->key) {
		callbacks->key(err, (const char*)msg, (int)len, ctx);
	    }
	    msg += len + 1;
	    break;
	}
	case VAL_KEY2: {
	    uint16_t	len;
	    
	    msg = read_uint16(msg, &len);
	    if (NULL != callbacks->key) {
		callbacks->key(err, (const char*)msg, (int)len, ctx);
	    }
	    msg += len + 1;
	    break;
	}
	case VAL_DEC: {
	    uint8_t	len = *msg++;

	    if (NULL != callbacks->decimal) {
		char	buf[256];
		double	d;
		
		memcpy(buf, msg, (size_t)len);
		buf[len] = '\0';
		d = strtod(buf, NULL);
		callbacks->decimal(err, d, ctx);
	    }
	    msg += len;
	    break;
	}
	case VAL_UUID:
	    if (NULL != callbacks->uuid || NULL != callbacks->uuid_str) {
		uint64_t	hi;
		uint64_t	lo;
	    
		msg = read_uint64(msg, &hi);
		msg = read_uint64(msg, &lo);
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
		msg += 16;
	    }
	    break;
	case VAL_TIME:
	    if (NULL != callbacks->time || NULL != callbacks->time_str) {
		uint64_t	nsec;
	    
		msg = read_uint64(msg, &nsec);
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
		msg += 8;
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
	    opo_err_set(err, OPO_ERR_PARSE, "corrupt msg format");
	    break;
	}
	if (OPO_ERR_OK != err->code) {
	    break;
	}
    }
    return err->code;
}
