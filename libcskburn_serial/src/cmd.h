#ifndef __LIB_CSKBURN_SERIAL_CMD__
#define __LIB_CSKBURN_SERIAL_CMD__

#include <stdbool.h>
#include <stdint.h>
#include <cskburn_serial.h>

#define RAM_BLOCK_SIZE 0x800
#define FLASH_BLOCK_SIZE 0x1000

#define BLOCKS(size, block_size) ((size + block_size - 1) / block_size)

bool cmd_sync(cskburn_serial_device_t *dev, uint16_t timeout);

bool cmd_read_reg(cskburn_serial_device_t *dev, uint32_t reg, uint32_t *val);

bool cmd_mem_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks,
		uint32_t block_size, uint32_t offset);
bool cmd_mem_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq);
bool cmd_mem_finish(cskburn_serial_device_t *dev);

bool cmd_flash_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks,
		uint32_t block_size, uint32_t offset);
bool cmd_flash_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq);
bool cmd_flash_finish(cskburn_serial_device_t *dev);

bool cmd_change_baud(cskburn_serial_device_t *dev, uint32_t baud);

#endif  // __LIB_CSKBURN_SERIAL_CMD__
