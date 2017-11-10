// Copyright 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPO_BUILDER_H__
#define __OPO_BUILDER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "err.h"
#include "msg.h"

#define OPO_MSG_MAX_DEPTH	512

    typedef struct _opoBuilder {
	uint8_t	*head;
	uint8_t	*end;
	uint8_t	*cur;
	bool	own;
	bool	*top;
	bool	stack[OPO_MSG_MAX_DEPTH]; // true if an object, false an array
    } *opoBuilder;

    extern opoErrCode	opo_builder_init(opoErr err, opoBuilder builder, uint8_t *buf, size_t size);
    extern void		opo_builder_cleanup(opoBuilder builder);
    extern opoErrCode	opo_builder_finish(opoErr err, opoBuilder builder);
    extern size_t	opo_builder_length(opoBuilder builder);
    extern opoMsg	opo_builder_take(opoBuilder builder);
    extern opoErrCode	opo_builder_push_object(opoErr err, opoBuilder builder);
    extern opoErrCode	opo_builder_push_array(opoErr err, opoBuilder builder);
    extern opoErrCode	opo_builder_pop(opoErr err, opoBuilder builder);
    extern opoErrCode	opo_builder_push_key(opoErr err, opoBuilder builder, const char *key, int len);
    extern opoErrCode	opo_builder_push_null(opoErr err, opoBuilder builder);
    extern opoErrCode	opo_builder_push_bool(opoErr err, opoBuilder builder, bool value);
    extern opoErrCode	opo_builder_push_int(opoErr err, opoBuilder builder, int64_t value);
    extern opoErrCode	opo_builder_push_double(opoErr err, opoBuilder builder, double value);
    extern opoErrCode	opo_builder_push_string(opoErr err, opoBuilder builder, const char *value, int len);
    extern opoErrCode	opo_builder_push_uuid(opoErr err, opoBuilder builder, uint64_t hi, uint64_t lo);
    extern opoErrCode	opo_builder_push_uuid_string(opoErr err, opoBuilder builder, const char *value);
    extern opoErrCode	opo_builder_push_time(opoErr err, opoBuilder builder, uint64_t value);

#ifdef __cplusplus
}
#endif
#endif /* __OPO_BUILDER_H__ */
