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
opo_client_query(opoErr err, opoClient client, opoVal query, opoQueryCallback cb, void *ctx, double timeout) {
    // TBD
    return 0;
}

bool
opo_client_cancel(opoClient client, opoRef ref, bool silent) {
    // TBD
    return false;
}

int
opo_client_process(opoClient client, int max, double wait) {
    // TBD
    return 0;
}
