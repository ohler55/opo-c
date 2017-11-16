// Copyright 2015, 2016, 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPO_ERR_H__
#define __OPO_ERR_H__

#include <errno.h>

#ifdef Linux
#define ELAST 160
#endif

#define OPO_ERR_INIT	{ 0, { 0 } }

typedef enum {
    OPO_ERR_OK		= 0,
    OPO_ERR_MEMORY	= ENOMEM,
    OPO_ERR_DENIED	= EACCES,
    OPO_ERR_IMPL	= ENOSYS,
    OPO_ERR_PARSE	= ELAST + 1,
    OPO_ERR_READ,
    OPO_ERR_WRITE,
    OPO_ERR_OVERFLOW,
    OPO_ERR_ARG,
    OPO_ERR_NOT_FOUND,
    OPO_ERR_THREAD,
    OPO_ERR_NETWORK,
    OPO_ERR_LOCK,
    OPO_ERR_FREE,
    OPO_ERR_IN_USE,
    OPO_ERR_TOO_MANY,
    OPO_ERR_TYPE,
    OPO_ERR_LAST
} opoErrCode;

typedef struct _opoErr {
    int		code;
    char	msg[256];
} *opoErr;

extern opoErrCode	opo_err_set(opoErr err, opoErrCode code, const char *fmt, ...);
extern opoErrCode	opo_err_no(opoErr err, const char *fmt, ...);
extern const char*	opo_err_str(opoErrCode code);
extern void		opo_err_clear(opoErr err);

#endif /* __OPO_ERR_H__ */
