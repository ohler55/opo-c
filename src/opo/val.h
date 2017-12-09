// Copyright 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPOC_VAL_H__
#define __OPOC_VAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "dict.h"
#include "err.h"

    typedef const uint8_t	*opoVal;
    typedef const uint8_t	*opoMsg;

    typedef enum {
	OPO_VAL_NONE	= (uint8_t)0,
	OPO_VAL_NULL	= (uint8_t)'n',
	OPO_VAL_BOOL	= (uint8_t)'b',
	OPO_VAL_INT	= (uint8_t)'i',
	OPO_VAL_STR	= (uint8_t)'s',
	OPO_VAL_KEY	= (uint8_t)'k',
	OPO_VAL_DEC	= (uint8_t)'d',
	OPO_VAL_UUID	= (uint8_t)'u',
	OPO_VAL_TIME	= (uint8_t)'t',
	OPO_VAL_OBJ	= (uint8_t)'o',
	OPO_VAL_ARRAY	= (uint8_t)'a',
    } opoValType;

    typedef struct _opoValCallbacks {
	bool	(*begin_object)(opoErr err, void *ctx);
	bool	(*end_object)(opoErr err, void *ctx);
	bool	(*key)(opoErr err, const char *key, int len, void *ctx);
	bool	(*begin_array)(opoErr err, void *ctx);
	bool	(*end_array)(opoErr err, void *ctx);
	bool	(*null)(opoErr err, void *ctx);
	bool	(*boolean)(opoErr err, bool b, void *ctx);
	bool	(*fixnum)(opoErr err, int64_t num, void *ctx);
	bool	(*decimal)(opoErr err, double num, void *ctx);
	bool	(*string)(opoErr err, const char *str, int len, void *ctx);
	bool	(*uuid)(opoErr err , uint64_t hi, uint64_t lo, void *ctx);
	bool	(*uuid_str)(opoErr err, const char *str, void *ctx);
	bool	(*time)(opoErr err, int64_t t, void *ctx);
	bool	(*time_str)(opoErr err, const char *str, void *ctx);
    } *opoValCallbacks;

    extern size_t	opo_val_bsize(opoVal val);
    extern size_t	opo_val_size(opoVal val);
    extern opoValType	opo_val_type(opoVal val);

    extern opoErrCode	opo_val_iterate(opoErr err, opoVal val, opoValCallbacks callbacks, void *ctx, opoDict dict);

    extern opoVal	opo_val_get(opoVal val, const char *path, opoDict dict);

    extern bool		opo_val_bool(opoErr err, opoVal val);
    extern int64_t	opo_val_int(opoErr err, opoVal val);
    extern double	opo_val_double(opoErr err, opoVal val);
    extern const char*	opo_val_string(opoErr err, opoVal val, int *lenp, opoDict dict);
    extern const char*	opo_val_key(opoErr err, opoVal key, int *lenp, opoDict dict);
    extern void		opo_val_uuid_str(opoErr err, opoVal val, char *str);
    extern void		opo_val_uuid(opoErr err, opoVal val, uint64_t *hip, uint64_t *lop);
    extern int64_t	opo_val_time(opoErr err, opoVal val);
    extern opoVal	opo_val_members(opoErr err, opoVal val);
    extern opoVal	opo_val_next(opoVal val);
    extern int		opo_val_member_count(opoErr err, opoVal val);

    extern uint64_t	opo_msg_id(opoMsg msg);
    extern void		opo_msg_set_id(uint8_t *msg, uint64_t id);
    extern size_t	opo_msg_bsize(opoMsg msg);
    extern opoMsg	opo_msg_dup(opoMsg msg);
    extern bool		opo_msg_size_ok(opoMsg msg, size_t len);

    static inline opoVal opo_msg_val(opoMsg msg) {
	return msg + 8;
    }

#ifdef __cplusplus
}
#endif
#endif /* __OPOC_VAL_H__ */
