// Copyright 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPO_CLIENT_H__
#define __OPO_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include "val.h"

    typedef struct _opoClient	*opoClient;
    typedef uint64_t		opoRef;
    typedef void		(*opoQueryCallback)(opoRef ref, opoVal response, void *ctx);

    extern opoClient	opo_client_connect(opoErr err, const char *host, int port);
    extern void		opo_client_close(opoClient client);
    extern opoRef	opo_client_query(opoErr err, opoClient client, opoVal query, opoQueryCallback cb, void *ctx);
    extern opoVal	opo_client_query_wait(opoErr err, opoClient client, opoVal query);

#ifdef __cplusplus
}
#endif
#endif /* __OPO_CLIENT_H__ */
