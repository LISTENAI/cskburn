#ifndef __LIB_CSKBURN_SERIAL_CORE__
#define __LIB_CSKBURN_SERIAL_CORE__

#include <cskburn_serial.h>

#include "serial.h"
#include "slip.h"

struct _cskburn_serial_device_t {
	serial_dev_t *serial;
	slip_dev_t *slip;
	void *req_hdr;
	void *req_cmd;
	uint8_t *req_buf;
	uint8_t *res_buf;
	uint32_t chip;
};

#endif  // __LIB_CSKBURN_SERIAL_CORE__
