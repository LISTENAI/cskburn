#include "core.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"
#include "cskburn_serial.h"
#include "log.h"
#include "msleep.h"
#include "serial.h"
#include "slip.h"
#include "time_monotonic.h"

#define BAUD_RATE_INIT 115200

#define FLASH_BLOCK_TRIES 3

extern uint8_t burner_serial_4[];
extern uint32_t burner_serial_4_len;

extern uint8_t burner_serial_6[];
extern uint32_t burner_serial_6_len;

static int init_flags = 0;
static bool rts_active = SERIAL_LOW;

void
cskburn_serial_init(int flags)
{
	init_flags = flags;
	rts_active = (flags & FLAG_INVERT_RTS) ? SERIAL_HIGH : SERIAL_LOW;
}

int
cskburn_serial_open(cskburn_serial_device_t **dev, const char *path, uint32_t chip)
{
	int ret;

	serial_dev_t *serial = NULL;
	ret = serial_open(path, &serial);
	if (ret != 0) {
		goto err_serial;
	}

	slip_dev_t *slip = slip_init(serial, MAX_REQ_SLIP_LEN, MAX_RES_SLIP_LEN);
	if (slip == NULL) {
		goto err_slip;
	}

	(*dev) = (cskburn_serial_device_t *)calloc(1, sizeof(cskburn_serial_device_t));
	(*dev)->serial = serial;
	(*dev)->slip = slip;
	(*dev)->req_buf = (uint8_t *)calloc(1, MAX_REQ_RAW_LEN);
	(*dev)->res_buf = (uint8_t *)calloc(1, MAX_RES_RAW_LEN);
	(*dev)->req_hdr = (*dev)->req_buf;
	(*dev)->req_cmd = (*dev)->req_buf + sizeof(csk_command_t);
	(*dev)->chip = chip;

	return 0;

err_slip:
	slip_deinit(&slip);
err_serial:
	return ret;
}

void
cskburn_serial_close(cskburn_serial_device_t **dev)
{
	if (dev != NULL) {
		if ((*dev)->slip != NULL) {
			slip_deinit(&(*dev)->slip);
		}
		if ((*dev)->serial != NULL) {
			serial_close(&(*dev)->serial);
		}
		if ((*dev)->req_buf != NULL) {
			free((*dev)->req_buf);
		}
		if ((*dev)->res_buf != NULL) {
			free((*dev)->res_buf);
		}
		free(*dev);
		*dev = NULL;
	}
}

static int
try_sync(cskburn_serial_device_t *dev, int timeout)
{
	int ret;
	uint64_t start = time_monotonic();
	do {
		ret = cmd_sync(dev, 100);
		if (ret == 0) {
			return 0;
		} else if (ret != -ETIMEDOUT) {
			return ret;
		}
	} while (TIME_SINCE_MS(start) < timeout);
	return -ETIMEDOUT;
}

int
cskburn_serial_connect(cskburn_serial_device_t *dev, uint32_t reset_delay, uint32_t probe_timeout)
{
	if (reset_delay > 0) {
		serial_set_rts(dev->serial, !rts_active);  // UPDATE=HIGH
		serial_set_dtr(dev->serial, SERIAL_HIGH);  // RESET=HIGH

		msleep(10);

		serial_set_rts(dev->serial, rts_active);  // UPDATE=LOW
		serial_set_dtr(dev->serial, SERIAL_LOW);  // RESET=LOW

		msleep(reset_delay);

		serial_set_dtr(dev->serial, SERIAL_HIGH);  // RESET=HIGH
	}

	return try_sync(dev, probe_timeout);
}

int
cskburn_serial_enter(
		cskburn_serial_device_t *dev, uint32_t baud_rate, uint8_t *burner, uint32_t len)
{
	int ret;

	if (burner == NULL || len == 0) {
		if (dev->chip == 3 || dev->chip == 4) {
			burner = burner_serial_4;
			len = burner_serial_4_len;
		} else if (dev->chip == 6) {
			burner = burner_serial_6;
			len = burner_serial_6_len;
		}
	}

	if (burner != NULL && len > 0) {
		// For CSK6, CMD_CHANGE_BAUD is supported by the ROM, so take advantage
		// of it to speed up the process.
		if (dev->chip == 6 && baud_rate != BAUD_RATE_INIT) {
			if ((ret = cmd_change_baud(dev, baud_rate, BAUD_RATE_INIT)) != 0) {
				LOGE("ERROR: Failed changing baud rate: %d (%s)", ret, strerror(-ret));
				return ret;
			}

			if ((ret = try_sync(dev, 2000)) != 0) {
				LOGE("ERROR: Device not synced after baud rate change: %d (%s)", ret,
						strerror(-ret));
				return ret;
			}
		}

		uint32_t offset, length;
		uint32_t blocks = BLOCKS(len, RAM_BLOCK_SIZE);

		if ((ret = cmd_mem_begin(dev, len, blocks, RAM_BLOCK_SIZE, 0)) != 0) {
			return ret;
		}

		for (uint32_t i = 0; i < blocks; i++) {
			offset = RAM_BLOCK_SIZE * i;
			length = RAM_BLOCK_SIZE;

			if (offset + length > len) {
				length = len - offset;
			}

			if ((ret = cmd_mem_block(dev, burner + offset, length, i)) != 0) {
				return ret;
			}
		}

		if ((ret = cmd_mem_finish(dev, OPTION_REBOOT, 0)) != 0) {
			return ret;
		}

		// RAM proxy is up with default baud rate
		if (dev->chip == 6 && baud_rate != BAUD_RATE_INIT) {
			serial_set_speed(dev->serial, BAUD_RATE_INIT);
		}

		msleep(500);
	}

	if ((ret = try_sync(dev, 2000)) != 0) {
		LOGE("ERROR: Device not recognized: %d (%s)", ret, strerror(-ret));
		return ret;
	}

	if ((ret = cmd_change_baud(dev, baud_rate, BAUD_RATE_INIT)) != 0) {
		LOGE("ERROR: Failed changing baud rate: %d (%s)", ret, strerror(-ret));
		return ret;
	}

	msleep(200);

	if ((ret = try_sync(dev, 2000)) != 0) {
		LOGE("ERROR: Device not synced after baud rate change: %d (%s)", ret, strerror(-ret));
		return ret;
	}

	return 0;
}

static void
print_time_spent(const char *usage, uint64_t t1, uint64_t t2)
{
	float spent = (float)(t2 - t1) / 1000.0f;
	LOGD("%s took %.2fs", usage, spent);
}

static void
print_time_spent_with_speed(const char *usage, uint64_t t1, uint64_t t2, uint32_t size)
{
	float spent = (float)(t2 - t1) / 1000.0f;
	float speed = (float)size / 1024.0f / spent;
	LOGD("%s took %.2fs, speed %.2f KB/s", usage, spent, speed);
}

static int
try_flash_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq)
{
	int ret;
	for (int i = 0; i < FLASH_BLOCK_TRIES; i++) {
		if (i > 0) {
			LOGD("DEBUG: Attempts %d writing block %d", i, seq);
		}
		ret = cmd_flash_block(dev, data, data_len, seq);
		if (ret == 0) {
			return 0;
		} else if (ret == -ETIMEDOUT) {
			continue;
		} else if (ret < 0) {  // In case of hardware error
			return ret;
		}
	}
	return ret;
}

static int
try_nand_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq)
{
	int ret;
	for (int i = 0; i < FLASH_BLOCK_TRIES; i++) {
		if (i > 0) {
			LOGD("DEBUG: Attempts %d writing block %d", i, seq);
		}
		ret = cmd_nand_block(dev, data, data_len, seq);
		if (ret == 0) {
			return 0;
		} else if (ret < 0) {  // In case of hardware error
			return ret;
		}
	}
	return ret;
}

int
cskburn_serial_write(cskburn_serial_device_t *dev, cskburn_serial_target_t target, uint32_t addr,
		reader_t *reader, uint32_t jump,
		void (*on_progress)(int32_t wrote_bytes, uint32_t total_bytes))
{
	int ret;
	uint32_t offset, length;
	uint32_t blocks = BLOCKS(reader->size, FLASH_BLOCK_SIZE);

	uint64_t t1 = time_monotonic();

	if (target == TARGET_FLASH) {
		if ((ret = cmd_flash_begin(dev, reader->size, blocks, FLASH_BLOCK_SIZE, addr)) != 0) {
			return ret;
		}
	} else if (target == TARGET_NAND) {
		if ((ret = cmd_nand_begin(dev, reader->size, blocks, FLASH_BLOCK_SIZE, addr)) != 0) {
			return ret;
		}
	} else if (target == TARGET_RAM) {
		if ((ret = cmd_mem_begin(dev, reader->size, blocks, FLASH_BLOCK_SIZE, addr) != 0)) {
			return ret;
		}
	} else {
		LOGE("ERROR: Unsupported target: %d", target);
		return -EINVAL;
	}

	uint8_t buffer[FLASH_BLOCK_SIZE];

	uint32_t i = 0;
	while (i < blocks) {
		offset = FLASH_BLOCK_SIZE * i;
		length = FLASH_BLOCK_SIZE;

		if (offset + length > reader->size) {
			length = reader->size - offset;
		}

		if (reader->read(reader, buffer, length) != length) {
			return -EIO;
		}

		if (target == TARGET_FLASH) {
			if ((ret = try_flash_block(dev, buffer, length, i)) != 0) {
				return ret;
			}
		} else if (target == TARGET_NAND) {
			if ((ret = try_nand_block(dev, buffer, length, i)) != 0) {
				return ret;
			}
		} else if (target == TARGET_RAM) {
			if ((ret = cmd_mem_block(dev, buffer, length, i)) != 0) {
				return ret;
			}
		}

		i++;

		if (on_progress != NULL) {
			on_progress(offset + length, reader->size);
		}
	}

	if (target == TARGET_FLASH) {
		cmd_flash_finish(dev);
	} else if (target == TARGET_NAND) {
		cmd_nand_finish(dev);
	} else if (target == TARGET_RAM) {
		cmd_mem_finish(dev, jump ? OPTION_JUMP : OPTION_RUN, jump);
	}

	uint64_t t2 = time_monotonic();
	print_time_spent_with_speed("Writing", t1, t2, reader->size);

	return 0;
}

int
cskburn_serial_read(cskburn_serial_device_t *dev, cskburn_serial_target_t target, uint32_t addr,
		uint32_t size, writer_t *writer,
		void (*on_progress)(int32_t read_bytes, uint32_t total_bytes))
{
	int ret;

	uint64_t t1 = time_monotonic();

	uint8_t buffer[FLASH_READ_SIZE];
	uint32_t offset, read_size;
	while (offset < size) {
		if ((ret = cmd_read_flash(dev, addr + offset, FLASH_READ_SIZE, buffer, &read_size)) != 0) {
			return ret;
		}

		if (writer->write(writer, buffer, read_size) != read_size) {
			return -EIO;
		}

		offset += read_size;

		if (on_progress != NULL) {
			on_progress(offset, size);
		}
	}

	uint64_t t2 = time_monotonic();
	print_time_spent_with_speed("Reading", t1, t2, size);

	return 0;
}

int
cskburn_serial_erase_all(cskburn_serial_device_t *dev, cskburn_serial_target_t target)
{
	int ret;

	if (target == TARGET_FLASH) {
		uint64_t t1 = time_monotonic();

		if ((ret = cmd_flash_erase_chip(dev)) != 0) {
			return ret;
		}

		uint64_t t2 = time_monotonic();
		print_time_spent("Erasing", t1, t2);

		return 0;
	} else if (target == TARGET_NAND) {
		LOGE("ERROR: Erasing is not supported for NAND yet");
		return -ENOTSUP;
	}

	LOGE("ERROR: Unsupported target: %d", target);
	return -EINVAL;
}

int
cskburn_serial_erase(
		cskburn_serial_device_t *dev, cskburn_serial_target_t target, uint32_t addr, uint32_t size)
{
	int ret;

	if (target == TARGET_FLASH) {
		uint64_t t1 = time_monotonic();

		if ((ret = cmd_flash_erase_region(dev, addr, size)) != 0) {
			return ret;
		}

		uint64_t t2 = time_monotonic();
		print_time_spent_with_speed("Erasing", t1, t2, size);

		return 0;
	} else if (target == TARGET_NAND) {
		LOGE("ERROR: Erasing is not supported for NAND yet");
		return -ENOTSUP;
	}

	LOGE("ERROR: Unsupported target: %d", target);
	return -EINVAL;
}

int
cskburn_serial_verify(cskburn_serial_device_t *dev, cskburn_serial_target_t target, uint32_t addr,
		uint32_t size, uint8_t *md5)
{
	int ret;

	if (target == TARGET_FLASH) {
		uint64_t t1 = time_monotonic();

		if ((ret = cmd_flash_md5sum(dev, addr, size, md5)) != 0) {
			return ret;
		}

		uint64_t t2 = time_monotonic();
		print_time_spent_with_speed("Verifying", t1, t2, size);

		return 0;
	} else if (target == TARGET_NAND) {
		uint64_t t1 = time_monotonic();

		if ((ret = cmd_nand_md5(dev, addr, size, md5)) != 0) {
			return ret;
		}

		uint64_t t2 = time_monotonic();
		print_time_spent_with_speed("Verifying", t1, t2, size);

		return 0;
	}

	LOGE("ERROR: Unsupported target: %d", target);
	return -EINVAL;
}

int
cskburn_serial_read_chip_id(cskburn_serial_device_t *dev, uint8_t *chip_id)
{
	return cmd_read_chip_id(dev, chip_id);
}

int
cskburn_serial_get_flash_info(
		cskburn_serial_device_t *dev, uint32_t *flash_id, uint64_t *flash_size)
{
	int ret;

	if ((ret = cmd_read_flash_id(dev, flash_id)) != 0) {
		return ret;
	}

	if ((*flash_id & 0xFFFFFF) == 0x000000 || (*flash_id & 0xFFFFFF) == 0xFFFFFF) {
		// In case flash is not present
		return -ENODEV;
	}

	*flash_size = 2 << (((*flash_id >> 16) & 0xFF) - 1);
	return 0;
}

int
cskburn_serial_init_nand(cskburn_serial_device_t *dev, nand_config_t *config, uint64_t *nand_size)
{
	return cmd_nand_init(dev, config, nand_size);
}

int
cskburn_serial_reset(cskburn_serial_device_t *dev, uint32_t reset_delay)
{
	serial_set_rts(dev->serial, !rts_active);  // UPDATE=HIGH
	serial_set_dtr(dev->serial, SERIAL_LOW);  // RESET=LOW

	msleep(reset_delay);

	serial_set_dtr(dev->serial, SERIAL_HIGH);  // RESET=HIGH

	return 0;
}

void
cskburn_serial_read_logs(cskburn_serial_device_t *dev, uint32_t baud)
{
	uint8_t buffer[1024];
	int32_t r;

	serial_discard_output(dev->serial);
	serial_set_speed(dev->serial, baud);

	while (true) {
		r = serial_read(dev->serial, buffer, sizeof(buffer), 100);
		if (r < 0) {
			LOGD("DEBUG: Failed reading logs: %d (%s)", r, strerror(-r));
			return;
		} else if (r == 0) {
			msleep(10);
			continue;
		}
		fwrite(buffer, 1, r, stdout);
		fflush(stdout);
	}
}
