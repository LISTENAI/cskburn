#ifndef __LIB_PORTABLE_TIME_MONOTONIC__
#define __LIB_PORTABLE_TIME_MONOTONIC__

#if defined(_WIN32) || defined(_WIN64)

#include <stdint.h>
#include <Sysinfoapi.h>
#define time_monotonic() (uint64_t)(GetTickCount64() / 1000)

#else  // POSIX

#include <stdint.h>
#include <time.h>
uint64_t
time_monotonic()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec;
}

#endif  // POSIX

#endif  // __LIB_PORTABLE_TIME_MONOTONIC__
