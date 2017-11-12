// Copyright 2017 by Peter Ohler, All Rights Reserved

#include "client.h"

struct _opoClient {
    int	sock;
};

opoClient
opo_client_connect(opoErr err, const char *host, int port) {
    // TBD
    return 0;
}

void
opo_client_close(opoClient client) {
    // TBD

}

opoRef
opo_client_query(opoErr err, opoClient client, opoVal query, opoQueryCallback cb, void *ctx) {
    // TBD
    return 0;
}

opoVal
opo_client_query_wait(opoErr err, opoClient client, opoVal query) {
    // TBD
    return 0;
}

