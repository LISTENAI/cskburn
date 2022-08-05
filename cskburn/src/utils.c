#include "utils.h"

#include <stdio.h>

uint32_t
read_file(const char *path, uint8_t *buf, uint32_t limit)
{
	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		return 0;
	}

	size_t len = fread(buf, 1, limit, f);
	fclose(f);
	return (uint32_t)len;
}

bool
scan_int(char *str, uint32_t *out)
{
	if (sscanf(str, "0x%x", out) == 1) {
		return true;
	}

	if (sscanf(str, "%d", out) == 1) {
		return true;
	}

	return false;
}

void
md5_to_str(char *buf, uint8_t *md5)
{
	sprintf(buf, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", md5[0], md5[1],
			md5[2], md5[3], md5[4], md5[5], md5[6], md5[7], md5[8], md5[9], md5[10], md5[11],
			md5[12], md5[13], md5[14], md5[15]);
}
