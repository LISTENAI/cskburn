#include "utils.h"

#include <ctype.h>
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

static bool
scan_int_prefix(const char *str, uint32_t *out, const char **end)
{
	if (str == NULL || out == NULL || end == NULL) {
		return false;
	}

	const char *first = str;
	while (isspace((unsigned char)*first)) {
		first++;
	}
	if (*first == '\0' || *first == '-') {
		return false;
	}

	const char *digits = *first == '+' ? first + 1 : first;
	int base = digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X') ? 16 : 10;

	// 仅 0x/0X 前缀按十六进制解析，其余按十进制解析
	char *parsed_end = NULL;
	errno = 0;
	unsigned long val = strtoul(str, &parsed_end, base);
	if (errno != 0 || parsed_end == str || val > UINT32_MAX) {
		return false;
	}

	*out = (uint32_t)val;
	*end = parsed_end;
	return true;
}

bool
scan_int(const char *str, uint32_t *out)
{
	if (out == NULL) {
		return false;
	}

	uint32_t value;
	const char *end;
	if (!scan_int_prefix(str, &value, &end) || *end != '\0') {
		return false;
	}

	*out = value;
	return true;
}

bool
scan_addr_size(const char *str, uint32_t *addr, uint32_t *size)
{
	if (str == NULL || addr == NULL || size == NULL) {
		return false;
	}

	uint32_t parsed_addr;
	uint32_t parsed_size;
	const char *end;

	if (!scan_int_prefix(str, &parsed_addr, &end) || *end != ':') {
		return false;
	}
	if (!scan_int(end + 1, &parsed_size)) {
		return false;
	}

	*addr = parsed_addr;
	*size = parsed_size;
	return true;
}

bool
scan_addr_size_name(const char *str,
					uint32_t *addr,
					uint32_t *size,
					const char **name)
{
	if (str == NULL || addr == NULL || size == NULL || name == NULL) {
		return false;
	}

	uint32_t parsed_addr;
	uint32_t parsed_size;
	const char *end;

	if (!scan_int_prefix(str, &parsed_addr, &end) || *end != ':') {
		return false;
	}
	if (!scan_int_prefix(end + 1, &parsed_size, &end) || *end != ':' || end[1] == '\0') {
		return false;
	}

	*addr = parsed_addr;
	*size = parsed_size;
	*name = end + 1;
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
