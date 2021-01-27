#ifndef __LIB_PORTABLE_EXISTS__
#define __LIB_PORTABLE_EXISTS__

#if defined(_WIN32) || defined(_WIN64)

#include <io.h>
#define exists(path) (_access(path, 0) == 0)

#else  // POSIX

#include <unistd.h>
#define exists(path) (access(path, F_OK) == 0)

#endif  // POSIX

#endif  // __LIB_PORTABLE_EXISTS__
