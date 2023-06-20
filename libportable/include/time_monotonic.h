#ifndef __LIB_PORTABLE_TIME_MONOTONIC__
#define __LIB_PORTABLE_TIME_MONOTONIC__

#if defined(_WIN32) || defined(_WIN64)

#include <Sysinfoapi.h>
#include <stdint.h>
#define time_monotonic() (uint64_t) GetTickCount64()

#else  // POSIX

#include <stdint.h>
uint64_t time_monotonic();

#endif  // POSIX

#define TIME_SINCE_MS(start) (uint32_t)(time_monotonic() - start)

#endif  // __LIB_PORTABLE_TIME_MONOTONIC__
