#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "fsio.h"
#include "log.h"
#include "read_parts.h"
#include "utils.h"

int
read_parts_bin(
		char **argv, int argc, cskburn_partition_t *parts, int *parts_cnt, int parts_cnt_limit)
{
	int i = 0, cnt = 0;
	while (i < argc - 1 && cnt <= parts_cnt_limit) {
		if (!scan_int(argv[i], &parts[cnt].addr)) {
			i++;
			continue;
		}

		parts[cnt].path = argv[i + 1];
		if (has_extname(parts[cnt].path, ".hex")) {
			i++;
			continue;
		}

		reader_t *reader = filereader_open(parts[cnt].path);
		if (reader == NULL) {
			LOGE_RET(-errno, "ERROR: Failed reading %s", parts[cnt].path);
			return -errno;
		}

		parts[cnt].reader = reader;

		i += 2;
		cnt++;
	}
	*parts_cnt += cnt;
	return 0;
}
