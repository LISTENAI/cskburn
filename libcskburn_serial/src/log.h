#ifndef __LIB_CSKBURN_SERIAL_LOG__
#define __LIB_CSKBURN_SERIAL_LOG__

#include <stdio.h>
#include <stdbool.h>

bool log_enabled;

#define LOGD(format, ...)                                    \
	do {                                                     \
		if (log_enabled) printf(format "\n", ##__VA_ARGS__); \
	} while (0)

#define LOG_TRACE(format, ...)                                        \
	do {                                                              \
		printf("\033[0;36m[TRACE] " format "\033[0m\n", __VA_ARGS__); \
	} while (0);

#define LOG_DUMP(data, len)                                    \
	do {                                                       \
		for (uint16_t i = 0; i < len; i += 32) {               \
			printf("\033[0;35m[TRACE] 0x%04X:", i);            \
			for (uint16_t j = i; j < i + 32 && j < len; j++) { \
				printf(" %02X", ((uint8_t *)data)[j]);         \
			}                                                  \
			printf("\033[0m\n");                               \
		}                                                      \
	} while (0);

#endif  // __LIB_CSKBURN_SERIAL_LOG__
