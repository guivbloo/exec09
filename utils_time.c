#include <sys/time.h>
#include <stdlib.h>
#include "utils_time.h"

/*
Timestamp set at startup
*/
struct timeval time_started;

void init_time (void)
{
    gettimeofday (&time_started, NULL);
}


/**
 * Return elapsed real time in milliseconds.
 */
long time_diff (struct timeval *old, struct timeval *new)
{
	long ms = (new->tv_usec - old->tv_usec) / 1000;
	ms += (new->tv_sec - old->tv_sec) * 1000;
	return ms;
}


long get_elapsed_realtime (void)
{
	struct timeval now;
	gettimeofday (&now, NULL);
	return time_diff (&time_started, &now);
}

