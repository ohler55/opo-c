// Copyright 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPOC_OPO_H__
#define __OPOC_OPO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include <ojc/ojc.h>

#include "client.h"
#include "dict.h"
#include "builder.h"
#include "val.h"

    extern opoMsg	opo_ojc_to_msg(opoErr err, ojcVal val, opoDict dict);
    extern ojcVal	opo_msg_to_ojc(opoErr err, opoMsg msg, opoDict dict);
    extern ojcVal	opo_val_to_ojc(opoErr err, opoVal msg, opoDict dict);
    extern opoErrCode	opo_ojc_fill_msg(opoErr err, ojcVal val, uint8_t *buf, size_t size, opoDict dict);

#ifdef __cplusplus
}
#endif
#endif /* __OPOC_OPO_H__ */
