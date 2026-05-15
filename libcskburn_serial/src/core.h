#ifndef __LIB_CSKBURN_SERIAL_CORE__
#define __LIB_CSKBURN_SERIAL_CORE__

#include <cskburn_serial.h>

#include "serial.h"
#include "slip.h"

struct cskburn_serial_burner_info {
	uint32_t load_addr;
};

struct _cskburn_serial_device_t {
	serial_dev_t *serial;
	slip_dev_t *slip;
	void *req_hdr;
	void *req_cmd;
	uint8_t *req_buf;
	uint8_t *res_buf;
	cskburn_serial_chip_t chip;
	const uint8_t *burner_img;
	uint32_t burner_len;
	const struct cskburn_serial_burner_info *burner_info;
	int32_t timeout;
};

#endif  // __LIB_CSKBURN_SERIAL_CORE__
