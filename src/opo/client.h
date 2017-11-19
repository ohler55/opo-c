// Copyright 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPOC_CLIENT_H__
#define __OPOC_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include "val.h"

    typedef struct _opoClientOptions {
	double	timeout;
    } *opoClientOptions;
    
    typedef struct _opoClient	*opoClient;
    typedef uint64_t		opoRef;
    typedef void		(*opoQueryCallback)(opoRef ref, opoVal response, void *ctx);

    extern opoClient	opo_client_connect(opoErr err, const char *host, int port, opoClientOptions options);
    extern void		opo_client_close(opoClient client);
    extern opoRef	opo_client_query(opoErr err, opoClient client, opoVal query, opoQueryCallback cb, void *ctx);
    extern bool		opo_client_cancel(opoClient client, opoRef ref, bool silent);
    extern int		opo_client_process(opoClient client, int max, double wait);

#ifdef __cplusplus
}
#endif
#endif /* __OPOC_CLIENT_H__ */
