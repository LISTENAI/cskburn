#ifndef __LIB_PORTABLE_MSLEEP__
#define __LIB_PORTABLE_MSLEEP__

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#define msleep(msec) Sleep(msec)

#else  // POSIX

#include <unistd.h>
#define msleep(ms) usleep(ms * 1000)

#endif  // POSIX

#endif  // __LIB_PORTABLE_MSLEEP__
