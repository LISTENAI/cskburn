#ifndef __LIB_CSKBURN_SERIAL_CMD__
#define __LIB_CSKBURN_SERIAL_CMD__

#include <cskburn_serial.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint8_t direction;
	uint8_t command;
	uint16_t size;
	uint32_t checksum;
} csk_command_t;

typedef struct {
	uint8_t direction;
	uint8_t command;
	uint16_t size;
	uint32_t value;
} csk_response_t;

#define RAM_BLOCK_SIZE 0x800
#define FLASH_BLOCK_SIZE 0x1000

#define STATUS_BYTES_LEN 2

#define MAX_REQ_COMMAND_LEN (sizeof(csk_command_t) + sizeof(uint32_t) * 4)
#define MAX_REQ_PAYLOAD_LEN (FLASH_BLOCK_SIZE)
#define MAX_REQ_RAW_LEN (MAX_REQ_COMMAND_LEN + MAX_REQ_PAYLOAD_LEN)
#define MAX_REQ_SLIP_LEN (MAX_REQ_RAW_LEN * 2)

#define MAX_RES_READ_LEN (512)
#define MAX_RES_SLIP_LEN (MAX_RES_READ_LEN * 2)

#define MD5_LEN 16

#define BLOCKS(size, block_size) ((size + block_size - 1) / block_size)

bool cmd_sync(cskburn_serial_device_t *dev, uint16_t timeout);

bool cmd_read_flash_id(cskburn_serial_device_t *dev, uint32_t *id);
bool cmd_read_chip_id(cskburn_serial_device_t *dev, uint8_t *id);

bool cmd_nand_init(cskburn_serial_device_t *dev, uint64_t *size);

bool cmd_mem_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks,
		uint32_t block_size, uint32_t offset);
bool cmd_mem_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq);
bool cmd_mem_finish(cskburn_serial_device_t *dev);

bool cmd_flash_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks,
		uint32_t block_size, uint32_t offset);
bool cmd_flash_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq);
bool cmd_flash_finish(cskburn_serial_device_t *dev);

bool cmd_flash_erase_chip(cskburn_serial_device_t *dev);
bool cmd_flash_erase_region(cskburn_serial_device_t *dev, uint32_t address, uint32_t size);

bool cmd_flash_md5sum(cskburn_serial_device_t *dev, uint32_t address, uint32_t size, uint8_t *md5);

bool cmd_change_baud(cskburn_serial_device_t *dev, uint32_t baud, uint32_t old_baud);

#endif  // __LIB_CSKBURN_SERIAL_CMD__
