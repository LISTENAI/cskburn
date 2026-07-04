#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "cskburn_errors.h"
#include "fsio.h"
#include "log.h"
#include "read_parts.h"
#include "utils.h"

int
read_parts_bin(
		char **argv, int argc, cskburn_partition_t *parts, int *parts_cnt, int parts_cnt_limit)
{
	int i = 0, cnt = 0, ret = 0;
	while (i < argc) {
		// .hex 文件由 read_parts_hex 处理（地址取自文件内容），此处跳过
		if (has_extname(argv[i], ".hex")) {
			i++;
			continue;
		}

		uint32_t addr;
		if (!scan_int(argv[i], &addr)) {
			// 既不是地址也不是 .hex 文件，无法解释的位置参数（如参数顺序写反）
			LOGE("ERROR [E%04d]: %s: %s", CSKBURN_ERR_ARG_INVALID,
					cskburn_strerror(-CSKBURN_ERR_ARG_INVALID), argv[i]);
			ret = -CSKBURN_ERR_ARG_INVALID;
			goto exit;
		}

		// 地址后必须跟随一个固件文件
		if (i + 1 >= argc) {
			LOGE("ERROR [E%04d]: %s: 地址 0x%08X 之后缺少固件文件", CSKBURN_ERR_ARG_INVALID,
					cskburn_strerror(-CSKBURN_ERR_ARG_INVALID), addr);
			ret = -CSKBURN_ERR_ARG_INVALID;
			goto exit;
		}

		char *path = argv[i + 1];
		if (has_extname(path, ".hex")) {
			// "addr file.hex"：地址取自 hex 文件内容，忽略此处地址，.hex 交给 hex 解析
			i++;
			continue;
		}

		if (cnt >= parts_cnt_limit) {
			LOGE("ERROR [E%04d]: %s（最多 %d 个）", CSKBURN_ERR_ARG_TOO_MANY_PARTS,
					cskburn_strerror(-CSKBURN_ERR_ARG_TOO_MANY_PARTS), parts_cnt_limit);
			ret = -CSKBURN_ERR_ARG_TOO_MANY_PARTS;
			goto exit;
		}

		reader_t *reader = filereader_open(path);
		if (reader == NULL) {
			LOGE("ERROR [E%04d]: %s: %s", CSKBURN_ERR_FILE_READ_FAILED,
					cskburn_strerror(-CSKBURN_ERR_FILE_READ_FAILED), path);
			ret = -CSKBURN_ERR_FILE_READ_FAILED;
			goto exit;
		}

		parts[cnt].addr = addr;
		parts[cnt].path = path;
		parts[cnt].reader = reader;

		i += 2;
		cnt++;
	}

exit:
	// 即便出错也累加已成功打开的分区数，让调用方清理逻辑正确关闭这些 reader
	*parts_cnt += cnt;
	return ret;
}
