#include <stdio.h>

#include "cskburn_serial.h"

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
	uint32_t arcs = cskburn_serial_get_capabilities(CHIP_ARCS);
	uint32_t dual = cskburn_serial_get_capabilities(CHIP_ARCS_DUAL);
	uint32_t venusa = cskburn_serial_get_capabilities(CHIP_VENUSA);

	CHECK((arcs & CSKBURN_CAP_EMMC) != 0);
	CHECK((arcs & CSKBURN_CAP_FLASH_PROTECTION) != 0);
	CHECK((arcs & CSKBURN_CAP_FLASH_INDEX) == 0);
	CHECK(cskburn_serial_get_flash_count(CHIP_ARCS) == 1);

	CHECK((dual & CSKBURN_CAP_EMMC) != 0);
	CHECK((dual & CSKBURN_CAP_FLASH_PROTECTION) != 0);
	CHECK((dual & CSKBURN_CAP_FLASH_INDEX) != 0);
	CHECK(cskburn_serial_get_flash_count(CHIP_ARCS_DUAL) == 2);

	CHECK((venusa & CSKBURN_CAP_READ_FLASH_STREAM) != 0);
	CHECK((venusa & CSKBURN_CAP_VENUSA_LOADER_PACING) != 0);
	CHECK(cskburn_serial_get_flash_count(CHIP_VENUSA) == 1);

	return 0;
}
