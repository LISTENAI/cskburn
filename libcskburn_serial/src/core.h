#ifndef __LIB_CSKBURN_SERIAL_CORE__
#define __LIB_CSKBURN_SERIAL_CORE__

#include <cskburn_serial.h>

struct _cskburn_serial_device_t {
	void *handle;
	void *req_hdr;
	void *req_cmd;
	uint8_t *req_raw_buf;
	uint8_t *req_slip_buf;
	uint8_t *res_slip_buf;
	uint32_t chip;
	bool nand;
	nand_config_t nand_cfg;
};

#endif  // __LIB_CSKBURN_SERIAL_CORE__
