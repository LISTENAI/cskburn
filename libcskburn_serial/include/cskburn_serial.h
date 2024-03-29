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

typedef enum {
	TARGET_FLASH = 0,
	TARGET_NAND = 1,
	TARGET_RAM = 2,
} cskburn_serial_target_t;

/**
 * @brief Open CSK device
 *
 * @param dev Pointer to device handle
 * @param path Path to serial device
 * @param chip Chip series
 *
 * @retval 0 if successful
 * @retval -errno on other errors from serial device
 */
int cskburn_serial_open(
		cskburn_serial_device_t **dev, const char *path, uint32_t chip, int32_t timeout);

/**
 * @brief Close CSK device
 *
 * @param dev Pointer to device handle
 */
void cskburn_serial_close(cskburn_serial_device_t **dev);

/**
 * @brief Connect to CSK device
 *
 * @param dev Device handle
 * @param reset_delay Delay in milliseconds that the reset line is held low
 * @param probe_timeout Timeout in milliseconds for probing the device
 *
 * @retval 0 if successful
 * @retval -ETIMEDOUT if timeout
 * @retval -errno on other errors from serial device
 */
int cskburn_serial_connect(
		cskburn_serial_device_t *dev, uint32_t reset_delay, uint32_t probe_timeout);

/**
 * @brief Enter CSK burn mode
 *
 * @param dev Device handle
 * @param baud_rate Baud rate to use
 * @param burner Custom burner.img to use, NULL to use default
 * @param len Length of burner.img, 0 to use default
 *
 * @retval 0 if successful
 * @retval -ETIMEDOUT if timeout
 * @retval -errno on other errors from serial device
 */
int cskburn_serial_enter(
		cskburn_serial_device_t *dev, uint32_t baud_rate, uint8_t *burner, uint32_t len);

int cskburn_serial_write(cskburn_serial_device_t *dev, cskburn_serial_target_t target,
		uint32_t addr, reader_t *reader, uint32_t jump,
		void (*on_progress)(int32_t wrote_bytes, uint32_t total_bytes));

int cskburn_serial_read(cskburn_serial_device_t *dev, cskburn_serial_target_t target, uint32_t addr,
		uint32_t size, writer_t *writer,
		void (*on_progress)(int32_t read_bytes, uint32_t total_bytes));

int cskburn_serial_erase_all(cskburn_serial_device_t *dev, cskburn_serial_target_t target);

int cskburn_serial_erase(
		cskburn_serial_device_t *dev, cskburn_serial_target_t target, uint32_t addr, uint32_t size);

int cskburn_serial_verify(cskburn_serial_device_t *dev, cskburn_serial_target_t target,
		uint32_t addr, uint32_t size, uint8_t *md5);

int cskburn_serial_read_chip_id(cskburn_serial_device_t *dev, uint8_t *chip_id);

int cskburn_serial_get_flash_info(
		cskburn_serial_device_t *dev, uint32_t *flash_id, uint64_t *flash_size);

int cskburn_serial_init_nand(
		cskburn_serial_device_t *dev, nand_config_t *config, uint64_t *nand_size);

int cskburn_serial_reset(cskburn_serial_device_t *dev, uint32_t reset_delay);

void cskburn_serial_read_logs(cskburn_serial_device_t *dev, uint32_t baud);

#endif  // __LIB_CSKBURN_SERIAL__
