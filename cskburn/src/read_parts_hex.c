#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "intelhex/intelhex.h"
#include "log.h"
#include "memio.h"
#include "read_parts.h"
#include "utils.h"

#define MAX_HEX_SIZE (48 * 1024 * 1024)

static int append_part(cskburn_partition_t *parts, int *part_idx, uint32_t part_size_limit,
		int parts_cnt_limit, const char *path, uint8_t *buf, uint32_t addr, uint32_t size);

bool
read_parts_hex(char **argv, int argc, cskburn_partition_t *parts, int *parts_cnt,
		uint32_t part_size_limit, int parts_cnt_limit, uint32_t base_addr)
{
	bool ret = false;

	uint8_t *hex_buf = malloc(MAX_HEX_SIZE);
	uint8_t *hex_ptr;
	uint32_t hex_len = 0;
	uint32_t hex_parsed = 0;

	uint8_t *bin_buf = malloc(part_size_limit);
	uint32_t bin_addr = 0;
	uint32_t bin_size = 0;

	int cnt = 0, idx = 0;
	for (int i = 0; i < argc; i++) {
		if (has_extname(argv[i], ".hex")) {
			hex_ptr = hex_buf;
			hex_len = read_file(argv[i], hex_ptr, MAX_HEX_SIZE);
			hex_parsed = 0;
			if (hex_len == 0) {
				LOGE("ERROR: Failed reading %s: %s", argv[i], strerror(errno));
				goto exit;
			} else {
				LOGD("Parsing HEX file: %s, size: %d", argv[i], hex_len);
			}

			reset_hex_parser();
			bin_addr = 0;
			bin_size = 0;

			hexfile_parse_status_t status;
			while (1) {
				status = parse_hex_blob(hex_ptr, hex_len, &hex_parsed, bin_buf, part_size_limit,
						&bin_addr, &bin_size);
				if (bin_addr >= base_addr) {
					bin_addr -= base_addr;
				}
				if (status == HEX_PARSE_OK) {
					if (bin_size > 0) {
						LOG_TRACE("Parsed HEX, addr: 0x%08X, size: %d (OK)", bin_addr, bin_size);
						cnt += append_part(parts, &idx, part_size_limit, parts_cnt_limit, argv[i],
								bin_buf, bin_addr, bin_size);
						idx++;
					}
					break;
				} else if (status == HEX_PARSE_UNALIGNED) {
					if (bin_size > 0) {
						LOG_TRACE("Parsed HEX, addr: 0x%08X, size: %d (UNALIGNED)", bin_addr,
								bin_size);
						cnt += append_part(parts, &idx, part_size_limit, parts_cnt_limit, argv[i],
								bin_buf, bin_addr, bin_size);
					}
					hex_len -= hex_parsed;
					hex_ptr += hex_parsed;
				} else if (status == HEX_PARSE_EOF) {
					if (bin_size > 0) {
						LOG_TRACE("Parsed HEX, addr: 0x%08X, size: %d (EOF)", bin_addr, bin_size);
						cnt += append_part(parts, &idx, part_size_limit, parts_cnt_limit, argv[i],
								bin_buf, bin_addr, bin_size);
						idx++;
					}
					break;
				} else {
					LOGE("Failed parsing HEX file: %d", status);
					goto exit;
				}
			}
		}
	}
	ret = true;

exit:
	LOGD("Parsed %d parts from HEXs", cnt);
	*parts_cnt += cnt;
	free(bin_buf);
	free(hex_buf);
	return ret;
}

static int
append_part(cskburn_partition_t *parts, int *part_idx, uint32_t part_size_limit,
		int parts_cnt_limit, const char *path, uint8_t *buf, uint32_t addr, uint32_t size)
{
	int idx = *part_idx;
	cskburn_partition_t *part = &parts[idx];

	if (part->reader != NULL) {
		if (part->addr + part->reader->size == addr) {
			memreader_feed(part->reader, buf, size);
			LOG_TRACE("Part %d, addr: 0x%08X, size: %d (append)", idx, part->addr,
					part->reader->size);
			return 0;
		} else {
			idx += 1;
			LOG_TRACE("Address changed from 0x%08X to 0x%08X, moving to next partition %d",
					part->addr + part->reader->size, addr, idx);
			if (idx >= parts_cnt_limit) {
				LOG_TRACE("Index %d reached part count limit %d, ignored", idx, parts_cnt_limit);
				return 0;
			}
			part = &parts[idx];
			*part_idx = idx;
		}
	}

	part->path = malloc(260 + 11);
	sprintf(part->path, "%s@0x%08X", path, addr);
	part->reader = memreader_alloc(part_size_limit);
	memreader_feed(part->reader, buf, size);
	part->addr = addr;
	LOG_TRACE("Part %d, addr: 0x%08X, size: %d (new)", idx, part->addr, part->reader->size);
	return 1;
}
