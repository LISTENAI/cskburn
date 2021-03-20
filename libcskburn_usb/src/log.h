#ifndef __LIB_CSKBURN_USB_LOG__
#define __LIB_CSKBURN_USB_LOG__

#include <stdio.h>
#include <stdbool.h>

bool log_enabled;

#define LOGD(format, ...)                                    \
	do {                                                     \
		if (log_enabled) printf(format "\n", ##__VA_ARGS__); \
	} while (0)

#endif  // __LIB_CSKBURN_USB_LOG__
