#include "core.h"

#include <stdio.h>
#include <stdlib.h>

#include "cmd.h"
#include "cskburn_serial.h"
#include "log.h"
#include "msleep.h"
#include "serial.h"
#include "time_monotonic.h"

#ifdef FEATURE_MD5_CHALLENGE
#include "mbedtls_md5.h"
#endif  // FEATURE_MD5_CHALLENGE

#define EFUSE_BASE 0xF1800000

#define FLASH_BLOCK_TRIES 5

extern uint8_t burner_serial[];
extern uint32_t burner_serial_len;

static int init_flags = 0;
static bool rts_active = SERIAL_LOW;

void
cskburn_serial_init(int flags)
{
	init_flags = flags;
	rts_active = (flags & FLAG_INVERT_RTS) ? SERIAL_HIGH : SERIAL_LOW;
}

cskburn_serial_device_t *
cskburn_serial_open(const char *path, uint32_t chip)
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
	dev->chip = chip;
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
	uint64_t start = time_monotonic();
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
		serial_set_rts(dev->handle, !rts_active);  // UPDATE=HIGH
		serial_set_dtr(dev->handle, SERIAL_HIGH);  // RESET=HIGH

		msleep(10);

		serial_set_rts(dev->handle, rts_active);  // UPDATE=LOW
		serial_set_dtr(dev->handle, SERIAL_LOW);  // RESET=LOW

		msleep(reset_delay);

		serial_set_dtr(dev->handle, SERIAL_HIGH);  // RESET=HIGH
	}

	return try_sync(dev, probe_timeout);
}

bool
cskburn_serial_enter(
		cskburn_serial_device_t *dev, uint32_t baud_rate, uint8_t *burner, uint32_t len)
{
	uint32_t chip = dev->chip;
	if (chip == 3 || chip == 4) {
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
	}

	if (!try_sync(dev, 2000)) {
		LOGD("DEBUG: Device not recognized");
		return false;
	}

	if (!cmd_change_baud(dev, baud_rate, 115200)) {
		LOGD("DEBUG: Failed changing baud rate");
		return false;
	}

	if (!try_sync(dev, 2000)) {
		LOGD("DEBUG: Device not synced after baud rate change");
		return false;
	}

	return true;
}

static bool
try_flash_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks, uint32_t block_size,
		uint32_t offset, uint8_t *md5)
{
	for (int i = 0; i < FLASH_BLOCK_TRIES; i++) {
		if (cmd_flash_begin(dev, size, blocks, block_size, offset, md5)) {
			return true;
		}
	}
	return false;
}

static bool
try_flash_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq,
		uint32_t *next_seq)
{
	for (int i = 0; i < FLASH_BLOCK_TRIES; i++) {
		if (i > 0) {
			LOGD("DEBUG: Attempts %d writing block %d", i, seq);
		}
		if (cmd_flash_block(dev, data, data_len, seq, next_seq)) {
			return true;
		}
	}
	return false;
}

#ifdef FEATURE_MD5_CHALLENGE
static bool
try_flash_md5_challenge(cskburn_serial_device_t *dev)
{
	for (int i = 0; i < FLASH_BLOCK_TRIES; i++) {
		if (cmd_flash_md5_challenge(dev)) {
			return true;
		}
	}
	return false;
}
#endif  // FEATURE_MD5_CHALLENGE

bool
cskburn_serial_write(cskburn_serial_device_t *dev, uint32_t addr, uint8_t *image, uint32_t len,
		void (*on_progress)(int32_t wrote_bytes, uint32_t total_bytes))
{
	uint32_t chip = dev->chip;
	uint32_t offset, length;
	uint32_t blocks = BLOCKS(len, FLASH_BLOCK_SIZE);

	uint64_t t1 = time_monotonic();

	uint8_t md5[MD5_LEN];
#ifdef FEATURE_MD5_CHALLENGE
	if (mbedtls_md5_ret(image, len, md5) != 0) {
		return false;
	}
#endif  // FEATURE_MD5_CHALLENGE

	if (!try_flash_begin(dev, len, blocks, FLASH_BLOCK_SIZE, addr, md5)) {
		return false;
	}

	uint32_t i = 0;
	while (i < blocks) {
		offset = FLASH_BLOCK_SIZE * i;
		length = FLASH_BLOCK_SIZE;

		if (offset + length > len) {
			length = len - offset;
		}

		uint32_t next = 0;
		if (!try_flash_block(dev, image + offset, length, i, &next)) {
			return false;
		}
#ifdef FEATURE_SEQ_QUEST
		if (next != i + 1) {
			LOGD("DEBUG: Pointer jumped from %d to %d", i, next);
		}
		i = next;
#else  // FEATURE_SEQ_QUEST
		i++;
#endif  // FEATURE_SEQ_QUEST

		if (on_progress != NULL) {
			on_progress(offset + length, len);
		}
	}

#ifdef FEATURE_MD5_CHALLENGE
	if (!try_flash_md5_challenge(dev)) {
		LOGD("DEBUG: MD5 challenge failed");
		return false;
	}
#endif  // FEATURE_MD5_CHALLENGE

	if (chip == 6) {
		for (int i = 0; i < FLASH_BLOCK_TRIES; i++) {
			if (cmd_flash_finish(dev)) {
				return true;
			}
		}
		return false;
	}

	uint64_t t2 = time_monotonic();

	uint32_t spent = (uint32_t)((t2 - t1) / 1000);
	float speed = (float)(blocks * FLASH_BLOCK_SIZE) / 1024.0f / (float)spent;
	LOGD("Time used %d s, speed %.2f KB/s", spent, speed);

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
	serial_set_rts(dev->handle, !rts_active);  // UPDATE=HIGH
	serial_set_dtr(dev->handle, SERIAL_LOW);  // RESET=LOW

	msleep(delay);

	serial_set_dtr(dev->handle, SERIAL_HIGH);  // RESET=HIGH

	return true;
}
