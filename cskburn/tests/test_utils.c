#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

#define CHECK(expr)                                                                             \
	do {                                                                                        \
		if (!(expr)) {                                                                          \
			fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr);          \
			return false;                                                                       \
		}                                                                                       \
	} while (0)

static bool
test_scan_int(void)
{
	uint32_t value = 0;

	CHECK(scan_int("0", &value) && value == 0);
	CHECK(scan_int("4294967295", &value) && value == UINT32_MAX);
	CHECK(scan_int("0xabcdef", &value) && value == 0xABCDEF);
	CHECK(scan_int("0X100000", &value) && value == 0x100000);
	CHECK(scan_int("010", &value) && value == 10);
	CHECK(scan_int(" 123", &value) && value == 123);

	CHECK(!scan_int(NULL, &value));
	CHECK(!scan_int("1", NULL));
	CHECK(!scan_int("", &value));
	CHECK(!scan_int("-1", &value));
	CHECK(!scan_int(" -1", &value));
	CHECK(!scan_int("4294967296", &value));
	CHECK(!scan_int("0x", &value));
	CHECK(!scan_int("123junk", &value));
	CHECK(!scan_int("123:456", &value));

	return true;
}

static bool
test_scan_addr_size(void)
{
	uint32_t addr = 0;
	uint32_t size = 0;

	CHECK(scan_addr_size("0x0:4194304", &addr, &size));
	CHECK(addr == 0 && size == 4194304);
	CHECK(scan_addr_size("0X100000:0x100000", &addr, &size));
	CHECK(addr == 0x100000 && size == 0x100000);
	CHECK(scan_addr_size("4096:8192", &addr, &size));
	CHECK(addr == 4096 && size == 8192);
	CHECK(scan_addr_size("000010:000020", &addr, &size));
	CHECK(addr == 10 && size == 20);

	CHECK(!scan_addr_size(NULL, &addr, &size));
	CHECK(!scan_addr_size("1:2", NULL, &size));
	CHECK(!scan_addr_size("1:2", &addr, NULL));
	CHECK(!scan_addr_size("1", &addr, &size));
	CHECK(!scan_addr_size(":1", &addr, &size));
	CHECK(!scan_addr_size("1:", &addr, &size));
	CHECK(!scan_addr_size("1:2:3", &addr, &size));
	CHECK(!scan_addr_size("1:-2", &addr, &size));
	CHECK(!scan_addr_size("4294967296:1", &addr, &size));
	CHECK(!scan_addr_size("1:4294967296", &addr, &size));

	addr = 11;
	size = 22;
	CHECK(!scan_addr_size("1:invalid", &addr, &size));
	CHECK(addr == 11 && size == 22);

	return true;
}

static bool
test_scan_addr_size_name(void)
{
	uint32_t addr = 0;
	uint32_t size = 0;
	const char *name = NULL;

	CHECK(scan_addr_size_name("0x0:4194304:factory.bin", &addr, &size, &name));
	CHECK(addr == 0 && size == 4194304 && strcmp(name, "factory.bin") == 0);
	CHECK(scan_addr_size_name("0X100000:0x6EC00:C:\\firmware\\dsp.bin", &addr, &size,
			&name));
	CHECK(addr == 0x100000 && size == 0x6EC00);
	CHECK(strcmp(name, "C:\\firmware\\dsp.bin") == 0);
	CHECK(scan_addr_size_name("1:2:output:part.bin", &addr, &size, &name));
	CHECK(strcmp(name, "output:part.bin") == 0);

	char long_arg[700] = "0:1:";
	memset(long_arg + 4, 'a', sizeof(long_arg) - 5);
	long_arg[sizeof(long_arg) - 1] = '\0';
	CHECK(scan_addr_size_name(long_arg, &addr, &size, &name));
	CHECK(name == long_arg + 4);
	CHECK(strlen(name) == sizeof(long_arg) - 5);

	CHECK(!scan_addr_size_name(NULL, &addr, &size, &name));
	CHECK(!scan_addr_size_name("1:2:file", NULL, &size, &name));
	CHECK(!scan_addr_size_name("1:2:file", &addr, NULL, &name));
	CHECK(!scan_addr_size_name("1:2:file", &addr, &size, NULL));
	CHECK(!scan_addr_size_name("1:2", &addr, &size, &name));
	CHECK(!scan_addr_size_name(":2:file", &addr, &size, &name));
	CHECK(!scan_addr_size_name("1::file", &addr, &size, &name));
	CHECK(!scan_addr_size_name("1:2:", &addr, &size, &name));
	CHECK(!scan_addr_size_name("1:invalid:file", &addr, &size, &name));
	CHECK(!scan_addr_size_name("4294967296:1:file", &addr, &size, &name));

	addr = 11;
	size = 22;
	name = "unchanged";
	CHECK(!scan_addr_size_name("1:invalid:file", &addr, &size, &name));
	CHECK(addr == 11 && size == 22 && strcmp(name, "unchanged") == 0);

	return true;
}

int
main(void)
{
	if (!test_scan_int() || !test_scan_addr_size() || !test_scan_addr_size_name()) {
		return 1;
	}
	puts("utils parser tests passed");
	return 0;
}
