// Copyright 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPOC_CLIENT_H__
#define __OPOC_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include "builder.h"
#include "dict.h"
#include "val.h"

    typedef struct _opoClient	*opoClient;
    typedef uint64_t		opoRef;
    typedef void		(*opoQueryCallback)(opoClient client, opoRef ref, opoMsg response, void *ctx);
    typedef void		(*opoStatusCallback)(opoClient client, bool connected, opoErrCode code, const char *msg);

    typedef struct _opoClientOptions {
	double			timeout;
	int			pending_max;
	opoStatusCallback	status_callback;
	const char		**words;
    } *opoClientOptions;

    extern opoClient	opo_client_connect(opoErr err, const char *host, int port, opoClientOptions options);
    extern void		opo_client_close(opoClient client);
    extern opoRef	opo_client_query(opoErr err, opoClient client, opoMsg query, opoQueryCallback cb, void *ctx);
    extern int		opo_client_process(opoClient client, int max, double wait);

    extern int		opo_client_pending_count(opoClient client);
    extern int		opo_client_ready_count(opoClient client);

    extern opoDict	opo_client_dictionary(opoClient client);
    extern void		opo_client_set_dictionary(opoClient client, opoBuilder builder);

#ifdef __cplusplus
}
#endif
#endif /* __OPOC_CLIENT_H__ */
