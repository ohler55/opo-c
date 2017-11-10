// Copyright 2015, 2016, 2017 by Peter Ohler, All Rights Reserved

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "err.h"

int
opo_err_set(opoErr err, int code, const char *fmt, ...) {
    va_list	ap;

    va_start(ap, fmt);
    vsnprintf(err->msg, sizeof(err->msg), fmt, ap);
    va_end(ap);
    err->code = code;

    return err->code;
}

int
opo_err_no(opoErr err, const char *fmt, ...) {
    int	cnt = 0;
    
    if (NULL != fmt) {
	va_list	ap;

	va_start(ap, fmt);
	cnt = vsnprintf(err->msg, sizeof(err->msg), fmt, ap);
	va_end(ap);
    }
    if (cnt < sizeof(err->msg) + 2 && 0 <= cnt) {
	err->msg[cnt] = ' ';
	cnt++;
	strncpy(err->msg + cnt, strerror(errno), sizeof(err->msg) - cnt);
	err->msg[sizeof(err->msg) - 1] = '\0';
    }
    err->code = errno;

    return err->code;
}

void
opo_err_clear(opoErr err) {
    err->code = OPO_ERR_OK;
    *err->msg = '\0';
}

const char*
opo_err_str(opoErrCode code) {
    const char	*str = NULL;
    
    if (code <= ELAST) {
	str = strerror(code);
    }
    if (NULL == str) {
	switch (code) {
	case OPO_ERR_PARSE:	str = "parse error";		break;
	case OPO_ERR_READ:	str = "read failed";		break;
	case OPO_ERR_WRITE:	str = "write failed";		break;
	case OPO_ERR_ARG:	str = "invalid argument";	break;
	case OPO_ERR_NOT_FOUND:	str = "not found";		break;
	case OPO_ERR_THREAD:	str = "thread error";		break;
	case OPO_ERR_NETWORK:	str = "network error";		break;
	case OPO_ERR_LOCK:	str = "lock error";		break;
	case OPO_ERR_FREE:	str = "already freed";		break;
	case OPO_ERR_IN_USE:	str = "in use";			break;
	case OPO_ERR_TOO_MANY:	str = "too many";		break;
	case OPO_ERR_TYPE:	str = "type error";		break;
	default:		str = "unknown error";		break;
	}
    }
    return str;
}
