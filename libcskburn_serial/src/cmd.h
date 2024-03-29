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

typedef enum {
	OPTION_REBOOT = 0,
	OPTION_JUMP = 1,
	OPTION_RUN = 2,
} cmd_finish_action_t;

#define RAM_BLOCK_SIZE (2 * 1024)
#define FLASH_BLOCK_SIZE (4 * 1024)
#define FLASH_READ_SIZE (64)

#define STATUS_BYTES_LEN 2

#define MAX_REQ_COMMAND_LEN (sizeof(csk_command_t) + sizeof(uint32_t) * 4)
#define MAX_REQ_PAYLOAD_LEN (FLASH_BLOCK_SIZE)
#define MAX_REQ_RAW_LEN (MAX_REQ_COMMAND_LEN + MAX_REQ_PAYLOAD_LEN)
#define MAX_REQ_SLIP_LEN (MAX_REQ_RAW_LEN * 2)

#define MAX_RES_COMMAND_LEN (sizeof(csk_response_t) + STATUS_BYTES_LEN)
#define MAX_RES_PAYLOAD_LEN (FLASH_READ_SIZE)
#define MAX_RES_RAW_LEN (MAX_RES_COMMAND_LEN + MAX_RES_PAYLOAD_LEN)
#define MAX_RES_SLIP_LEN (MAX_RES_RAW_LEN * 2)

#define MD5_LEN 16

#define BLOCKS(size, block_size) ((size + block_size - 1) / block_size)

int cmd_sync(cskburn_serial_device_t *dev, uint16_t timeout);

int cmd_read_flash_id(cskburn_serial_device_t *dev, uint32_t *id);
int cmd_read_chip_id(cskburn_serial_device_t *dev, uint8_t *id);

int cmd_nand_init(cskburn_serial_device_t *dev, nand_config_t *config, uint64_t *size);

int cmd_nand_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks,
		uint32_t block_size, uint32_t offset);
int cmd_nand_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq);
int cmd_nand_finish(cskburn_serial_device_t *dev);

int cmd_nand_md5(cskburn_serial_device_t *dev, uint32_t address, uint32_t size, uint8_t *md5);

int cmd_mem_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks, uint32_t block_size,
		uint32_t offset);
int cmd_mem_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq);
int cmd_mem_finish(cskburn_serial_device_t *dev, cmd_finish_action_t action, uint32_t address);

int cmd_flash_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks,
		uint32_t block_size, uint32_t offset);
int cmd_flash_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq);
int cmd_flash_finish(cskburn_serial_device_t *dev);

int cmd_flash_erase_chip(cskburn_serial_device_t *dev);
int cmd_flash_erase_region(cskburn_serial_device_t *dev, uint32_t address, uint32_t size);

int cmd_flash_md5sum(cskburn_serial_device_t *dev, uint32_t address, uint32_t size, uint8_t *md5);

int cmd_read_flash(cskburn_serial_device_t *dev, uint32_t address, uint32_t size, uint8_t *data,
		uint32_t *data_len);

int cmd_change_baud(cskburn_serial_device_t *dev, uint32_t baud, uint32_t old_baud);

#endif  // __LIB_CSKBURN_SERIAL_CMD__
