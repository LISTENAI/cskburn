#include <time_monotonic.h>

#if !defined(_WIN32) && !defined(_WIN64)

#include <time.h>

uint64_t
time_monotonic()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000 / 1000;
}

#endif
