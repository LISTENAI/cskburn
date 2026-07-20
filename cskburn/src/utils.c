#include "utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
	if (str == NULL || str[0] == '\0' || str[0] == '-') {
		return false;
	}

	// base 0 自动识别 0x/0X 十六进制前缀，其余按十进制解析
	char *end = NULL;
	errno = 0;
	unsigned long val = strtoul(str, &end, 0);
	if (errno != 0 || end == str || *end != '\0' || val > UINT32_MAX) {
		return false;
	}

	*out = (uint32_t)val;
	return true;
}

static bool
scan_int_field(const char *str, size_t len, uint32_t *out)
{
	char field[32];

	if (len == 0 || len >= sizeof(field)) {
		return false;
	}
	memcpy(field, str, len);
	field[len] = '\0';
	return scan_int(field, out);
}

bool
scan_addr_size(const char *str, uint32_t *addr, uint32_t *size)
{
	if (str == NULL || addr == NULL || size == NULL) {
		return false;
	}

	const char *split = strchr(str, ':');
	if (split == NULL || strchr(split + 1, ':') != NULL) {
		return false;
	}

	const char *str_size = split + 1;
	return scan_int_field(str, (size_t)(split - str), addr) && scan_int(str_size, size);
}

bool
scan_addr_size_name(const char *str, uint32_t *addr, uint32_t *size, const char **name)
{
	if (str == NULL || addr == NULL || size == NULL || name == NULL) {
		return false;
	}

	const char *addr_end = strchr(str, ':');
	if (addr_end == NULL) {
		return false;
	}

	const char *str_size = addr_end + 1;
	const char *size_end = strchr(str_size, ':');
	if (size_end == NULL || size_end[1] == '\0') {
		return false;
	}

	if (!scan_int_field(str, (size_t)(addr_end - str), addr) ||
			!scan_int_field(str_size, (size_t)(size_end - str_size), size)) {
		return false;
	}

	*name = size_end + 1;
	return true;
}

void
md5_to_str(char *buf, uint8_t *md5)
{
	snprintf(buf, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", md5[0],
			md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7], md5[8], md5[9], md5[10],
			md5[11], md5[12], md5[13], md5[14], md5[15]);
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

uint32_t
align_down(uint32_t addr, uint32_t align)
{
	if (align == 0) return addr;
	return addr & ~(align - 1);
}

bool
is_aligned(uint32_t addr, uint32_t align)
{
	return addr % align == 0;
}
