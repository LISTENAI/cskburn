#ifndef __CSKBURN_UTILS__
#define __CSKBURN_UTILS__

#include <stdbool.h>
#include <stdint.h>

#define MD5_SIZE 16

uint32_t read_file(const char *path, uint8_t *buf, uint32_t limit);

bool scan_int(char *str, uint32_t *out);

void md5_to_str(char *buf, uint8_t *md5);

#endif  // __CSKBURN_UTILS__
