#ifndef __CSKBURN_UTILS__
#define __CSKBURN_UTILS__

#include <stdbool.h>
#include <stdint.h>

#define MD5_SIZE 16
#define FLASH_ALIGN (4 * 1024)
#define NAND_ALIGN 512

uint32_t read_file(const char *path, uint8_t *buf, uint32_t limit);

bool scan_int(const char *str, uint32_t *out);

bool scan_addr_size(const char *str, uint32_t *addr, uint32_t *size);

bool scan_addr_size_name(const char *str, uint32_t *addr, uint32_t *size, const char **name);

void md5_to_str(char *buf, uint8_t *md5);

bool has_extname(char *path, const char *extname);

uint32_t align_up(uint32_t addr, uint32_t align);

uint32_t align_down(uint32_t addr, uint32_t align);

bool is_aligned(uint32_t addr, uint32_t align);

#endif  // __CSKBURN_UTILS__
