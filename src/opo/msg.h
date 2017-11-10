// Copyright 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPO_MSG_H__
#define __OPO_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "err.h"
#include "val.h"

    typedef const uint8_t	*opoMsg;

    extern size_t	opo_msg_size(opoMsg msg);
    extern opoVal	opo_msg_top(opoMsg msg);
    extern opoErrCode	opo_msg_iterate(opoErr err, opoMsg msg, opoMsgCallbacks callbacks, void *ctx);

#ifdef __cplusplus
}
#endif
#endif /* __OPO_MSG_H__ */
