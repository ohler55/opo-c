// Copyright 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPO_OPO_H__
#define __OPO_OPO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include <ojc/ojc.h>

#include "builder.h"
#include "msg.h"
#include "val.h"

    extern opoMsg	opo_ojc_to_msg(opoErr err, ojcVal val);
    extern ojcVal	opo_msg_to_ojc(opoErr err, const opoMsg msg);

    extern size_t	opo_ojc_msg_size(ojcVal val);

#ifdef __cplusplus
}
#endif
#endif /* __OPO_OPO_H__ */
