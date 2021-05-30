#ifndef __LIB_PORTABLE_TIME_MONOTONIC__
#define __LIB_PORTABLE_TIME_MONOTONIC__

#if defined(_WIN32) || defined(_WIN64)

#include <stdint.h>
#include <Sysinfoapi.h>
#define time_monotonic() (uint64_t) GetTickCount64()

#else  // POSIX

#include <stdint.h>
uint64_t time_monotonic();

#endif  // POSIX

#define TIME_SINCE_MS(start) (uint16_t)(time_monotonic() - start)

#endif  // __LIB_PORTABLE_TIME_MONOTONIC__
