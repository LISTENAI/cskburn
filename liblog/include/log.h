#ifndef __LIB_CSKBURN_LOG__
#define __LIB_CSKBURN_LOG__

#include <stdio.h>
#include <stdbool.h>

#define LOGLEVEL_ERROR 3
#define LOGLEVEL_INFO 2
#define LOGLEVEL_DEBUG 1
#define LOGLEVEL_TRACE 0

int log_level;

void set_log_level(int level);

#define LOGE(format, ...)                                \
	do {                                                 \
		if (log_level <= LOGLEVEL_ERROR) {               \
			fprintf(stderr, format "\n", ##__VA_ARGS__); \
			fflush(stderr);                              \
		}                                                \
	} while (0);

#define LOGI(format, ...)                                \
	do {                                                 \
		if (log_level <= LOGLEVEL_INFO) {                \
			fprintf(stdout, format "\n", ##__VA_ARGS__); \
			fflush(stdout);                              \
		}                                                \
	} while (0);

#define LOGD(format, ...)                                \
	do {                                                 \
		if (log_level <= LOGLEVEL_DEBUG) {               \
			fprintf(stdout, format "\n", ##__VA_ARGS__); \
			fflush(stdout);                              \
		}                                                \
	} while (0);

#define LOG_TRACE(format, ...)                                                     \
	do {                                                                           \
		if (log_level <= LOGLEVEL_TRACE) {                                         \
			fprintf(stdout, "\033[0;36m[TRACE] " format "\033[0m\n", __VA_ARGS__); \
			fflush(stdout);                                                        \
		}                                                                          \
	} while (0);

#define LOG_DUMP(data, len)                                         \
	do {                                                            \
		if (log_level <= LOGLEVEL_TRACE)                            \
			for (uint16_t i = 0; i < len; i += 32) {                \
				fprintf(stdout, "\033[0;35m[TRACE] 0x%04X:", i);    \
				for (uint16_t j = i; j < i + 32 && j < len; j++) {  \
					fprintf(stdout, " %02X", ((uint8_t *)data)[j]); \
				}                                                   \
				fprintf(stdout, "\033[0m\n");                       \
				fflush(stdout);                                     \
			}                                                       \
	} while (0);

#endif  // __LIB_CSKBURN_LOG__
