#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

#define CHECK(expr) \
	do { \
		if (!(expr)) { \
			fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
			return 1; \
		} \
	} while (0)

int
main(void)
{
	uint32_t addr = 0;
	uint32_t size = 0;
	const char *name = NULL;

	CHECK(scan_addr_size("0x0:0x1000", &addr, &size) && addr == 0 && size == 4096);
	CHECK(scan_addr_size("0X10:4096", &addr, &size) && addr == 16 && size == 4096);
	CHECK(scan_addr_size("0:0", &addr, &size) && addr == 0 && size == 0);
	CHECK(!scan_addr_size(NULL, &addr, &size));
	CHECK(!scan_addr_size("0x0", &addr, &size));
	CHECK(!scan_addr_size("0x0:", &addr, &size));
	CHECK(!scan_addr_size(":0x1000", &addr, &size));
	CHECK(!scan_addr_size("0x0:0x1000:extra", &addr, &size));
	CHECK(!scan_addr_size("0x100000000:1", &addr, &size));
	CHECK(!scan_addr_size("1:2junk", &addr, &size));

	CHECK(scan_addr_size_name("0x0:64:a.bin", &addr, &size, &name) && addr == 0 && size == 64 &&
			strcmp(name, "a.bin") == 0);
	CHECK(scan_addr_size_name("0X10:64:C:\\tmp\\a.bin", &addr, &size, &name) && addr == 16 &&
			size == 64 && strcmp(name, "C:\\tmp\\a.bin") == 0);
	CHECK(!scan_addr_size_name("0x0:64:", &addr, &size, &name));
	CHECK(!scan_addr_size_name("0x0::a.bin", &addr, &size, &name));
	CHECK(!scan_addr_size_name(":64:a.bin", &addr, &size, &name));
	CHECK(!scan_addr_size_name("0x0:64", &addr, &size, &name));

	return 0;
}
