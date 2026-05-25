#ifndef __CSKBURN_READ_PARTS__
#define __CSKBURN_READ_PARTS__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "io.h"

typedef struct {
	char *path;
	uint32_t addr;
	reader_t *reader;
} cskburn_partition_t;

typedef struct {
	uint32_t base;
	uint32_t size;
} cskburn_chip_mem_region_t;

int read_parts_bin(
		char **argv, int argc, cskburn_partition_t *parts, int *parts_cnt, int parts_cnt_limit);

int read_parts_hex(char **argv, int argc, cskburn_partition_t *parts, int *parts_cnt,
		uint32_t part_size_limit, int parts_cnt_limit, const cskburn_chip_mem_region_t *regions,
		size_t region_count);

#endif  // __CSKBURN_READ_PARTS__
