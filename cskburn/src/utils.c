#include "utils.h"

#include <stdio.h>
#include <string.h>

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
scan_int(const char *str, uint32_t *out)
{
	if (sscanf(str, "0x%x", out) == 1) {
		return true;
	}

	if (sscanf(str, "%d", out) == 1) {
		return true;
	}

	return false;
}

bool
scan_addr_size(const char *str, uint32_t *addr, uint32_t *size)
{
	char *split = strstr(str, ":");
	if (split == NULL) {
		return false;
	}

	const char *str_addr = str;
	const char *str_size = split + 1;

	return scan_int(str_addr, addr) && scan_int(str_size, size);
}

bool
scan_addr_size_name(const char *str, uint32_t *addr, uint32_t *size, const char **name)
{
	char *split;

	if (name == NULL) {
		return false;
	}

	if ((split = strstr(str, ":")) == NULL) {
		return false;
	}

	const char *str_addr = str;
	const char *str_size = split + 1;

	if ((split = strstr(str_size, ":")) == NULL) {
		return false;
	}

	*name = split + 1;
	return scan_int(str_addr, addr) && scan_int(str_size, size);
}

void
md5_to_str(char *buf, uint8_t *md5)
{
	sprintf(buf, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", md5[0], md5[1],
			md5[2], md5[3], md5[4], md5[5], md5[6], md5[7], md5[8], md5[9], md5[10], md5[11],
			md5[12], md5[13], md5[14], md5[15]);
}

bool
has_extname(char *path, const char *extname)
{
	size_t path_len = strlen(path);
	size_t extn_len = strlen(extname);
	if (path_len <= extn_len) return false;
	return strncasecmp(path + path_len - extn_len, extname, extn_len) == 0;
}

uint32_t
align_up(uint32_t addr, uint32_t align)
{
	if (align == 0) return addr;
	return (addr + align - 1) & ~(align - 1);
}

bool
is_aligned(uint32_t addr, uint32_t align)
{
	return addr % align == 0;
}
