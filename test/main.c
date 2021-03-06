// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <stdio.h>
#include <stdlib.h>

#include <ojc/ojc.h>

#include "ut.h"

extern void	append_builder_tests(utTest tests);
extern void	append_val_tests(utTest tests);
extern void	append_opo_tests(utTest tests);
extern void	append_client_tests(utTest tests);

int
main(int argc, char **argv) {
    struct _utTest	tests[1024] = { { NULL, NULL } };

    append_builder_tests(tests);
    append_val_tests(tests);
    append_opo_tests(tests);
    append_client_tests(tests);

    ut_init(argc, argv, "OpO", tests);

    ut_done();

    ojc_cleanup();
    
    return 0;
}
