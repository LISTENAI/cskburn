#ifndef __CSKBURN_READ_PARTS__
#define __CSKBURN_READ_PARTS__

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	char *path;
	uint8_t *image;
	uint32_t addr;
	uint32_t size;
} cskburn_partition_t;

bool read_parts_bin(char **argv, int argc, cskburn_partition_t *parts, int *parts_cnt,
		uint32_t part_size_limit, int parts_cnt_limit);

bool read_parts_hex(char **argv, int argc, cskburn_partition_t *parts, int *parts_cnt,
		uint32_t part_size_limit, int parts_cnt_limit, uint32_t base_addr);

#endif  // __CSKBURN_READ_PARTS__
