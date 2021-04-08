#include <stdio.h>
#include <stdlib.h>

#include <msleep.h>
#include <serial.h>
#include <cskburn_serial.h>

#include "log.h"
#include "cmd.h"

extern uint8_t burner_serial[];
extern uint32_t burner_serial_len;

void
cskburn_serial_init(bool verbose)
{
	log_enabled = verbose;
}

cskburn_serial_device_t *
cskburn_serial_open(const char *path)
{
	cskburn_serial_device_t *dev =
			(cskburn_serial_device_t *)malloc(sizeof(cskburn_serial_device_t));

	serial_dev_t *device = serial_open(path);
	if (device == NULL) {
		goto err_open;
	}

	dev->handle = device;
	return dev;

err_open:
	free(dev);
	return NULL;
}

void
cskburn_serial_close(cskburn_serial_device_t **dev)
{
	if (dev != NULL) {
		if ((*dev)->handle != NULL) {
			serial_close((serial_dev_t **)&(*dev)->handle);
		}
		free(*dev);
		*dev = NULL;
	}
}

static bool
try_sync(cskburn_serial_device_t *dev, int timeout, int tries)
{
	for (int i = 0; i < tries; i++) {
		if (cmd_sync(dev, timeout)) {
			return true;
		}
	}
	return false;
}

bool
cskburn_serial_connect(cskburn_serial_device_t *dev)
{
	serial_set_dtr(dev->handle, true);  // RESET=LOW
	serial_set_rts(dev->handle, true);  // UPDATE=LOW

	msleep(500);

	serial_set_dtr(dev->handle, false);  // RESET=HIGH

	msleep(500);

	serial_set_rts(dev->handle, false);  // UPDATE=HIGH

	msleep(500);

	return try_sync(dev, 100, 3);
}

bool
cskburn_serial_enter(cskburn_serial_device_t *dev, uint32_t baud_rate)
{
	uint32_t offset, length;
	uint32_t blocks = BLOCKS(burner_serial_len, RAM_BLOCK_SIZE);

	if (!cmd_mem_begin(dev, burner_serial_len, blocks, RAM_BLOCK_SIZE, 0)) {
		return false;
	}

	for (uint32_t i = 0; i < blocks; i++) {
		offset = RAM_BLOCK_SIZE * i;
		length = RAM_BLOCK_SIZE;

		if (offset + length > burner_serial_len) {
			length = burner_serial_len - offset;
		}

		if (!cmd_mem_block(dev, burner_serial + offset, length, i)) {
			return false;
		}
	}

	if (!cmd_mem_finish(dev)) {
		return false;
	}

	msleep(500);

	if (!try_sync(dev, 500, 5)) {
		LOGD("错误: 无法识别设备");
		return false;
	}

	if (!cmd_change_baud(dev, baud_rate)) {
		LOGD("错误: 无法设置串口速率");
		return false;
	}

	if (!try_sync(dev, 500, 5)) {
		LOGD("错误: 无法连接设备");
		return false;
	}

	return true;
}

static bool
try_flash_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq)
{
	for (int i = 0; i < 5; i++) {
		if (cmd_flash_block(dev, data, data_len, seq)) {
			return true;
		}
	}
	return false;
}

bool
cskburn_serial_write(cskburn_serial_device_t *dev, uint32_t addr, uint8_t *image, uint32_t len,
		void (*on_progress)(int32_t wrote_bytes, uint32_t total_bytes))
{
	uint32_t offset, length;
	uint32_t blocks = BLOCKS(len, FLASH_BLOCK_SIZE);
	int32_t wrote = 0;

	if (!cmd_flash_begin(dev, len, blocks, FLASH_BLOCK_SIZE, addr)) {
		return false;
	}

	for (uint32_t i = 0; i < blocks; i++) {
		offset = FLASH_BLOCK_SIZE * i;
		length = FLASH_BLOCK_SIZE;

		if (offset + length > len) {
			length = len - offset;
		}

		if (!try_flash_block(dev, image + offset, length, i)) {
			return false;
		}

		wrote += length;
		if (on_progress != NULL) {
			on_progress(wrote, len);
		}
	}

	msleep(500);

	return true;
}

bool
cskburn_serial_reset(cskburn_serial_device_t *dev)
{
	serial_set_dtr(dev->handle, true);  // RESET=LOW
	msleep(500);
	serial_set_dtr(dev->handle, false);  // RESET=HIGH
	return true;
}
