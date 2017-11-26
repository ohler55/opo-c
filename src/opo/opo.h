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
#include "builder.h"
#include "val.h"

    extern opoVal	opo_ojc_to_msg(opoErr err, ojcVal val);
    extern ojcVal	opo_msg_to_ojc(opoErr err, const opoVal msg);
    extern void		opo_ojc_fill_msg(ojcVal val, uint8_t *buf);

    extern size_t	opo_ojc_msg_size(ojcVal val);

#ifdef __cplusplus
}
#endif
#endif /* __OPOC_OPO_H__ */
