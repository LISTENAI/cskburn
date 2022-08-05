#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "read_parts.h"
#include "utils.h"

bool
read_parts_bin(char **argv, int argc, cskburn_partition_t *parts, int *parts_cnt,
		uint32_t part_size_limit, int parts_cnt_limit)
{
	int i = 0, cnt = 0;
	while (i < argc - 1 && cnt <= parts_cnt_limit) {
		if (!scan_int(argv[i], &parts[cnt].addr)) {
			i++;
			continue;
		}

		parts[cnt].path = argv[i + 1];
		parts[cnt].image = malloc(part_size_limit);
		parts[cnt].size = read_file(parts[cnt].path, parts[cnt].image, part_size_limit);
		if (parts[cnt].size == 0) {
			LOGE("ERROR: Failed reading %s: %s", parts[cnt].path, strerror(errno));
			return false;
		}

		i += 2;
		cnt++;
	}
	*parts_cnt += cnt;
	return true;
}
