#ifndef __LIB_CSKBURN_SERIAL__
#define __LIB_CSKBURN_SERIAL__

#include <stdbool.h>
#include <stdint.h>

#include "io.h"

#define FLAG_INVERT_RTS (1 << 0)

#define CHIP_ID_LEN 8

void cskburn_serial_init(int flags);

struct _cskburn_serial_device_t;
typedef struct _cskburn_serial_device_t cskburn_serial_device_t;

#pragma pack(1)
typedef struct {
	uint8_t set : 1;
	uint8_t pad : 1;
	uint8_t pin : 6;
} nand_pinmux_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	uint8_t mode_4bit;
	nand_pinmux_t sd_cmd;
	nand_pinmux_t sd_clk;
	nand_pinmux_t sd_dat0;
	nand_pinmux_t sd_dat1;
	nand_pinmux_t sd_dat2;
	nand_pinmux_t sd_dat3;
	uint8_t _reserved;
} nand_config_t;
#pragma pack()

typedef struct {
	bool enable;
	nand_config_t config;
} cskburn_serial_nand_t;

cskburn_serial_device_t *cskburn_serial_open(
		const char *pat, uint32_t chip, cskburn_serial_nand_t *nand);
void cskburn_serial_close(cskburn_serial_device_t **dev);

bool cskburn_serial_connect(
		cskburn_serial_device_t *dev, uint32_t reset_delay, uint32_t probe_timeout);

bool cskburn_serial_enter(
		cskburn_serial_device_t *dev, uint32_t baud_rate, uint8_t *burner, uint32_t len);

bool cskburn_serial_write(cskburn_serial_device_t *dev, uint32_t addr, reader_t *reader,
		void (*on_progress)(int32_t wrote_bytes, uint32_t total_bytes));

bool cskburn_serial_erase_all(cskburn_serial_device_t *dev);
bool cskburn_serial_erase(cskburn_serial_device_t *dev, uint32_t addr, uint32_t size);

bool cskburn_serial_verify(
		cskburn_serial_device_t *dev, uint32_t addr, uint32_t size, uint8_t *md5);

bool cskburn_serial_read_chip_id(cskburn_serial_device_t *dev, uint8_t *chip_id);

bool cskburn_serial_get_flash_info(
		cskburn_serial_device_t *dev, uint32_t *flash_id, uint64_t *flash_size);

bool cskburn_serial_reset(cskburn_serial_device_t *dev, uint32_t reset_delay);

#endif  // __LIB_CSKBURN_SERIAL__
