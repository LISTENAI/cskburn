#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <log.h>
#include <msleep.h>
#include <time_monotonic.h>
#include <serial.h>
#include <cskburn_serial.h>

#include "core.h"
#include "cmd.h"

#define EFUSE_BASE 0xF1800000

#define PIN_LO true
#define PIN_HI false

#define FLASH_BLOCK_TRIES 5

#define TIME_SINCE_MS(start) (uint16_t)((clock() - start) * 1000.0 * 1000.0 / CLOCKS_PER_SEC)

extern uint8_t burner_serial[];
extern uint32_t burner_serial_len;

static bool rts_inverted = false;

void
cskburn_serial_init(bool invert_rts)
{
	rts_inverted = invert_rts;
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
	dev->req_raw_buf = (uint8_t *)malloc(MAX_REQ_RAW_LEN);
	dev->req_slip_buf = (uint8_t *)malloc(MAX_REQ_SLIP_LEN);
	dev->res_slip_buf = (uint8_t *)malloc(MAX_RES_SLIP_LEN);
	dev->req_hdr = dev->req_raw_buf;
	dev->req_cmd = dev->req_raw_buf + sizeof(csk_command_t);
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
		if ((*dev)->req_raw_buf != NULL) {
			free((*dev)->req_raw_buf);
		}
		if ((*dev)->req_slip_buf != NULL) {
			free((*dev)->req_slip_buf);
		}
		if ((*dev)->res_slip_buf != NULL) {
			free((*dev)->res_slip_buf);
		}
		free(*dev);
		*dev = NULL;
	}
}

static bool
try_sync(cskburn_serial_device_t *dev, int timeout)
{
	clock_t start = clock();
	do {
		if (cmd_sync(dev, 100)) {
			return true;
		}
	} while (TIME_SINCE_MS(start) < timeout);
	return false;
}

bool
cskburn_serial_connect(cskburn_serial_device_t *dev, uint32_t reset_delay, uint32_t probe_timeout)
{
	if (reset_delay > 0) {
		serial_set_dtr(dev->handle, PIN_HI);  // RESET=HIGH
		serial_set_rts(dev->handle, rts_inverted ? PIN_LO : PIN_HI);  // UPDATE=HIGH

		msleep(10);

		serial_set_dtr(dev->handle, PIN_LO);  // RESET=LOW
		serial_set_rts(dev->handle, rts_inverted ? PIN_HI : PIN_LO);  // UPDATE=LOW

		msleep(reset_delay);

		serial_set_dtr(dev->handle, PIN_HI);  // RESET=HIGH
	}

	return try_sync(dev, probe_timeout);
}

bool
cskburn_serial_enter(
		cskburn_serial_device_t *dev, uint32_t baud_rate, uint8_t *burner, uint32_t len)
{
	if (len == 0) {
		burner = burner_serial;
		len = burner_serial_len;
	}

	uint32_t offset, length;
	uint32_t blocks = BLOCKS(len, RAM_BLOCK_SIZE);

	if (!cmd_mem_begin(dev, len, blocks, RAM_BLOCK_SIZE, 0)) {
		return false;
	}

	for (uint32_t i = 0; i < blocks; i++) {
		offset = RAM_BLOCK_SIZE * i;
		length = RAM_BLOCK_SIZE;

		if (offset + length > len) {
			length = len - offset;
		}

		if (!cmd_mem_block(dev, burner + offset, length, i)) {
			return false;
		}
	}

	if (!cmd_mem_finish(dev)) {
		return false;
	}

	msleep(500);

	if (!try_sync(dev, 2000)) {
		LOGD("错误: 无法识别设备");
		return false;
	}

	if (!cmd_change_baud(dev, baud_rate)) {
		LOGD("错误: 无法设置串口速率");
		return false;
	}

	if (!try_sync(dev, 2000)) {
		LOGD("错误: 无法连接设备");
		return false;
	}

	return true;
}

static bool
try_flash_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq)
{
	for (int i = 0; i < FLASH_BLOCK_TRIES; i++) {
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

	uint64_t t1 = time_monotonic();

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

	uint64_t t2 = time_monotonic();

	uint32_t spent = (uint32_t)(t2 - t1);
	float speed = (float)(blocks * FLASH_BLOCK_SIZE) / 1024.0f / (float)spent;
	LOGD("耗时 %d s, 速度 %.2f KB/s", spent, speed);

	msleep(500);

	return true;
}

bool
cskburn_serial_verify(cskburn_serial_device_t *dev, uint32_t addr, uint32_t size, uint8_t *md5)
{
	return cmd_flash_md5sum(dev, addr, size, md5);
}

bool
cskburn_serial_read_chip_id(cskburn_serial_device_t *dev, uint64_t *chip_id)
{
	uint32_t id0, id1;

	if (!cmd_read_reg(dev, EFUSE_BASE + 0x80 + 0x0A, &id1)) {
		return false;
	}

	if (!cmd_read_reg(dev, EFUSE_BASE + 0x80 + 0x0E, &id0)) {
		return false;
	}

	*chip_id = ((uint64_t)id0 << 32) | id1;
	return true;
}

bool
cskburn_serial_reset(cskburn_serial_device_t *dev, uint32_t delay, bool ok)
{
	if (ok) {
		serial_set_rts(dev->handle, PIN_LO);  // UPDATE=LOW, LED=GREEN
	} else {
		serial_set_rts(dev->handle, PIN_HI);  // UPDATE=HIGH, LED=RED
	}

	serial_set_dtr(dev->handle, PIN_LO);  // RESET=LOW

	msleep(delay);

	serial_set_dtr(dev->handle, PIN_HI);  // RESET=HIGH
	serial_set_rts(dev->handle, rts_inverted ? PIN_LO : PIN_HI);  // UPDATE=HIGH

	return true;
}
