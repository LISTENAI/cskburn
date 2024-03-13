#ifndef __LIB_CSKBURN_LOG__
#define __LIB_CSKBURN_LOG__

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define LOGLEVEL_ERROR 3
#define LOGLEVEL_INFO 2
#define LOGLEVEL_DEBUG 1
#define LOGLEVEL_TRACE 0

extern int csk_log_level;

void set_log_level(int level);

const char *errid(int errnum);

#define LOGE(format, ...)                                \
	do {                                                 \
		if (csk_log_level <= LOGLEVEL_ERROR) {           \
			fprintf(stderr, format "\n", ##__VA_ARGS__); \
			fflush(stderr);                              \
		}                                                \
	} while (0);

#define LOGI(format, ...)                                \
	do {                                                 \
		if (csk_log_level <= LOGLEVEL_INFO) {            \
			fprintf(stdout, format "\n", ##__VA_ARGS__); \
			fflush(stdout);                              \
		}                                                \
	} while (0);

#define LOGD(format, ...)                                \
	do {                                                 \
		if (csk_log_level <= LOGLEVEL_DEBUG) {           \
			fprintf(stdout, format "\n", ##__VA_ARGS__); \
			fflush(stdout);                              \
		}                                                \
	} while (0);

#define LOGE_RET(ret, format, ...)                                   \
	if (ret < 0) {                                                   \
		LOGE(format ": %s", ##__VA_ARGS__, errid(-ret));             \
	} else {                                                         \
		LOGE(format ": %02X", ##__VA_ARGS__, (uint8_t)(ret & 0xFF)); \
	}

#define LOGI_RET(ret, format, ...)                                   \
	if (ret < 0) {                                                   \
		LOGI(format ": %s", ##__VA_ARGS__, errid(-ret));             \
	} else {                                                         \
		LOGI(format ": %02X", ##__VA_ARGS__, (uint8_t)(ret & 0xFF)); \
	}

#define LOGD_RET(ret, format, ...)                                   \
	if (ret < 0) {                                                   \
		LOGD(format ": %s", ##__VA_ARGS__, errid(-ret));             \
	} else {                                                         \
		LOGD(format ": %02X", ##__VA_ARGS__, (uint8_t)(ret & 0xFF)); \
	}

#define LOG_TRACE(format, ...)                                                     \
	do {                                                                           \
		if (csk_log_level <= LOGLEVEL_TRACE) {                                     \
			fprintf(stdout, "\033[0;36m[TRACE] " format "\033[0m\n", __VA_ARGS__); \
			fflush(stdout);                                                        \
		}                                                                          \
	} while (0);

#define LOG_DUMP(data, len)                                         \
	do {                                                            \
		if (csk_log_level <= LOGLEVEL_TRACE)                        \
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
