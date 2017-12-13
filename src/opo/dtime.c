// Copyright 2015, 2016, 2017 by Peter Ohler, All Rights Reserved

#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "dtime.h"

#define MIN_SLEEP	(1.0 / (double)CLOCKS_PER_SEC)

double
dtime() {
    struct timeval	tv;

    gettimeofday(&tv, NULL);

    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

double
dsleep(double t) {
    struct timespec	req, rem;

    if (MIN_SLEEP > t) {
	t = MIN_SLEEP;
    }
    req.tv_sec = (time_t)t;
    req.tv_nsec = (long)(1000000000.0 * (t - (double)req.tv_sec));
    if (nanosleep(&req, &rem) == -1 && EINTR == errno) {
	return (double)rem.tv_sec + (double)rem.tv_nsec / 1000000000.0;
    }
    return 0.0;
}
