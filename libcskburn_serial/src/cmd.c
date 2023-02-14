#include "cmd.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "log.h"
#include "msleep.h"
#include "serial.h"
#include "slip.h"
#include "time_monotonic.h"

#define TRACE_DATA 0
#define TRACE_SLIP 0

#define DIR_REQ 0x00
#define DIR_RES 0x01

#define CMD_FLASH_BEGIN 0x02
#define CMD_FLASH_DATA 0x03
#define CMD_FLASH_END 0x04
#define CMD_MEM_BEGIN 0x05
#define CMD_MEM_END 0x06
#define CMD_MEM_DATA 0x07
#define CMD_SYNC 0x08
#define CMD_READ_REG 0x0a
#define CMD_CHANGE_BAUDRATE 0x0f
#define CMD_SPI_FLASH_MD5 0x13
#define CMD_NAND_INIT 0x20
#define CMD_NAND_BEGIN 0x21
#define CMD_NAND_DATA 0x22
#define CMD_NAND_END 0x23
#define CMD_NAND_MD5 0x24
#define CMD_READ_FLASH_ID 0xF3

#define CHECKSUM_MAGIC 0xef
#define CHECKSUM_NONE 0

// 默认指令超时时间
#define TIMEOUT_DEFAULT 200

// Mem 写入指令超时时间
#define TIMEOUT_MEM_DATA 500

// Flash 写入指令超时时间
// 实测极限烧录速度为 8M/48s，约 170KB/s
// 换算可得每 4K-block 的写入耗时为 23.4375ms
// 因此写入超时取 100ms
// 当 erase 阻塞导致队列满时，burner 会延迟 100ms 返回 0x0A，故取 500ms 保险值
// 取值过小会导致正常返回和重试返回叠加，seq 错位
#define TIMEOUT_FLASH_DATA 1000

// MD5 计算指令超时时间
#define TIMEOUT_FLASH_MD5SUM 30000

typedef struct {
	uint32_t size;
	uint32_t blocks;
	uint32_t block_size;
	uint32_t offset;
} cmd_mem_begin_t;

typedef struct {
	uint32_t size;
	uint32_t seq;
	uint32_t rev1;
	uint32_t rev2;
} cmd_mem_block_t;

typedef struct {
	uint32_t option;  // cmd_finish_action_t
	uint32_t address;
} cmd_mem_finish_t;

typedef enum {
	OPTION_REBOOT = 0,
	OPTION_JUMP = 1,
	OPTION_RUN = 2,
} cmd_finish_action_t;

typedef cmd_mem_begin_t cmd_flash_begin_t;
typedef cmd_mem_block_t cmd_flash_block_t;
typedef cmd_mem_finish_t cmd_flash_finish_t;

typedef struct {
	uint32_t baud;
	uint32_t old_baud;
} cmd_change_baud_t;

typedef struct {
	uint32_t address;
	uint32_t size;
	uint32_t rev1;
	uint32_t rev2;
} cmd_flash_md5_t;

static bool
command(cskburn_serial_device_t *dev, uint8_t op, uint16_t in_len, uint32_t in_chk,
		uint32_t *out_val, void *out_buf, uint16_t *out_len, uint16_t out_limit, uint16_t timeout)
{
	bool ret = false;

	LOG_TRACE("> req op=%02X len=%d", op, in_len);
#if TRACE_DATA
	LOG_DUMP(dev->req_cmd, in_len);
#endif

	uint32_t req_raw_len = sizeof(csk_command_t) + in_len;

	csk_command_t *req = (csk_command_t *)dev->req_hdr;
	req->direction = DIR_REQ;
	req->command = op;
	req->size = in_len;
	req->checksum = in_chk;

	uint32_t req_slip_len = slip_encode(dev->req_raw_buf, dev->req_slip_buf, req_raw_len);

	int32_t r;
	uint64_t start;

	serial_discard_input(dev->handle);
	serial_discard_output(dev->handle);

	start = time_monotonic();
	uint32_t bytes_wrote = 0;
	do {
		r = serial_write(
				dev->handle, dev->req_slip_buf + bytes_wrote, req_slip_len - bytes_wrote, timeout);
#if !defined(_WIN32) && !defined(_WIN64)
		if (r == -1 && errno != EAGAIN) {
			LOGD("DEBUG: Failed writing command %02X: %d (%s)", op, errno, strerror(errno));
			goto exit;
		}
#endif  // !WIN32 && !WIN64
		if (r <= 0) {
			msleep(10);
			continue;
		}

		LOG_TRACE("Wrote %d bytes in %d ms", r, TIME_SINCE_MS(start));
#if TRACE_SLIP
		LOG_DUMP(dev->req_slip_buf + bytes_wrote, r);
#endif

		bytes_wrote += r;

		if (bytes_wrote >= req_slip_len) {
			break;
		}
	} while (TIME_SINCE_MS(start) < timeout);
	if (bytes_wrote < req_slip_len) {
		LOGD("DEBUG: Timeout sending command %02X", op);
		goto exit;
	}

	start = time_monotonic();
	uint32_t bytes_read = 0;
	uint32_t res_slip_offset = 0;
	do {
		r = serial_read(dev->handle, dev->res_slip_buf + bytes_read, MAX_RES_READ_LEN - bytes_read,
				timeout);
#if !defined(_WIN32) && !defined(_WIN64)
		if (r == -1 && errno != EAGAIN) {
			LOGD("DEBUG: Failed reading command %02X: %d (%s)", op, errno, strerror(errno));
			goto exit;
		}
#endif  // !WIN32 && !WIN64
		if (r <= 0) {
			msleep(10);
			continue;
		}

		LOG_TRACE("Read %d bytes in %d ms", r, TIME_SINCE_MS(start));
#if TRACE_SLIP
		LOG_DUMP(dev->res_slip_buf + bytes_read, r);
#endif

		bytes_read += r;

		while (res_slip_offset < bytes_read) {
			uint8_t *res_raw_ptr = dev->res_slip_buf + res_slip_offset;
			uint32_t res_slip_limit = bytes_read - res_slip_offset;

			uint32_t res_raw_len = 0;
			uint32_t res_slip_step = slip_decode(res_raw_ptr, &res_raw_len, res_slip_limit);
			if (res_slip_step == 0) {
				break;
			}

			res_slip_offset += res_slip_step;

			if (res_raw_len <= 0) {
				continue;
			}

			csk_response_t *res = (csk_response_t *)res_raw_ptr;
			if (res->direction != DIR_RES) {
				continue;
			}

			uint8_t *res_data = res_raw_ptr + sizeof(csk_response_t);

			LOG_TRACE("< res op=%02X len=%d val=%d", res->command, res->size, res->value);
#if TRACE_DATA
			LOG_DUMP(res_data, res->size);
#endif

			if (res->command != op) {
				continue;
			}

			if (out_val != NULL) {
				*out_val = res->value;
			}

			if (out_buf != NULL && out_len != NULL) {
				uint16_t res_size = res->size;
				if (res_size > out_limit) {
					res_size = out_limit;
				}
				memcpy(out_buf, res_data, res_size);
				*out_len = res_size;
			}

			ret = true;
			goto exit;
		}
	} while (TIME_SINCE_MS(start) < timeout);
	if (bytes_read == 0) {
		LOG_TRACE("Read timeout after %d ms", TIME_SINCE_MS(start));
	}

exit:
	serial_discard_input(dev->handle);
	return ret;
}

static uint8_t
check_command(cskburn_serial_device_t *dev, uint8_t op, uint16_t in_len, uint32_t in_chk,
		uint32_t *out_val, uint16_t timeout)
{
	struct {
		uint8_t error;
		uint8_t code;
	} ret = {0};
	uint16_t ret_len = 0;

	if (!command(dev, op, in_len, in_chk, out_val, &ret, &ret_len, sizeof(ret), timeout)) {
		LOGD("DEBUG: Failed to send command %02X", op);
		return 0xFE;
	}

	if (ret_len < sizeof(ret)) {
		LOGD("DEBUG: Interrupted serial read of command %02X", op);
		return 0xFF;
	}

	if (ret.error) {
		LOGD("DEBUG: Unexpected device response of command %02X: 0x%02X", op, ret.code);
		return ret.code;
	} else {
		return 0x00;
	}
}

static uint32_t
checksum(void *buf, uint16_t len)
{
	uint32_t state = CHECKSUM_MAGIC;
	for (uint16_t i = 0; i < len; i++) {
		state ^= ((uint8_t *)buf)[i];
	}
	return state;
}

bool
cmd_sync(cskburn_serial_device_t *dev, uint16_t timeout)
{
	uint8_t *cmd = (uint8_t *)dev->req_cmd;
	cmd[0] = 0x07;
	cmd[1] = 0x07;
	cmd[2] = 0x12;
	cmd[3] = 0x20;
	memset(cmd + 4, 0x55, 32);
	return command(dev, CMD_SYNC, 4 + 32, CHECKSUM_NONE, NULL, NULL, NULL, 0, timeout);
}

bool
cmd_read_reg(cskburn_serial_device_t *dev, uint32_t reg, uint32_t *val)
{
	uint32_t *cmd = (uint32_t *)dev->req_cmd;
	*cmd = reg;

	return !check_command(dev, CMD_READ_REG, sizeof(reg), CHECKSUM_NONE, val, TIMEOUT_DEFAULT);
}

bool
cmd_read_flash_id(cskburn_serial_device_t *dev, uint32_t *id)
{
	return !check_command(dev, CMD_READ_FLASH_ID, 0, CHECKSUM_NONE, id, TIMEOUT_DEFAULT);
}

bool
cmd_nand_init(cskburn_serial_device_t *dev, uint64_t *size)
{
#pragma pack(1)
	struct {
		uint8_t status;
		uint8_t error;
		uint32_t blk_len;
		uint32_t blk_num;
	} ret;
#pragma pack()
	uint16_t ret_len = 0;

	nand_config_t *cmd = (nand_config_t *)dev->req_cmd;
	memcpy(cmd, &dev->nand_cfg, sizeof(nand_config_t));

	if (!command(dev, CMD_NAND_INIT, sizeof(nand_config_t), CHECKSUM_NONE, NULL, &ret, &ret_len,
				sizeof(ret), TIMEOUT_FLASH_DATA)) {
		return false;
	}

	if (ret_len < STATUS_BYTES_LEN) {
		LOGD("DEBUG: Interrupted serial read");
		return false;
	}

	if (ret.status != 0) {
		LOGD("DEBUG: Unexpected device response: %02X%02X", ret.status, ret.error);
		return false;
	}

	LOGD("Inited NAND flash with block size: %u bytes, count: %u", ret.blk_len, ret.blk_num);

	*size = ret.blk_len * ret.blk_num;

	return true;
}

bool
cmd_mem_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks, uint32_t block_size,
		uint32_t offset)
{
	cmd_mem_begin_t *cmd = (cmd_mem_begin_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_mem_begin_t));
	cmd->size = size;
	cmd->blocks = blocks;
	cmd->block_size = block_size;
	cmd->offset = offset;

	return !check_command(
			dev, CMD_MEM_BEGIN, sizeof(cmd_mem_begin_t), CHECKSUM_NONE, NULL, TIMEOUT_DEFAULT);
}

bool
cmd_mem_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq)
{
	cmd_mem_block_t *cmd = (cmd_mem_block_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_mem_block_t));
	cmd->size = data_len;
	cmd->seq = seq;
	cmd->rev1 = 0;
	cmd->rev2 = 0;

	uint8_t *req_data = (uint8_t *)dev->req_cmd + sizeof(cmd_mem_block_t);
	memcpy(req_data, data, data_len);

	uint32_t in_len = sizeof(cmd_mem_block_t) + data_len;

	return !check_command(
			dev, CMD_MEM_DATA, in_len, checksum(data, data_len), NULL, TIMEOUT_MEM_DATA);
}

bool
cmd_mem_finish(cskburn_serial_device_t *dev)
{
	cmd_mem_finish_t *cmd = (cmd_mem_finish_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_mem_finish_t));
	cmd->option = OPTION_REBOOT;
	cmd->address = 0;

	return !check_command(
			dev, CMD_MEM_END, sizeof(cmd_mem_finish_t), CHECKSUM_NONE, NULL, TIMEOUT_DEFAULT);
}

bool
cmd_flash_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks, uint32_t block_size,
		uint32_t offset)
{
	cmd_flash_begin_t *cmd = (cmd_flash_begin_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_begin_t));
	cmd->size = size;
	cmd->blocks = blocks;
	cmd->block_size = block_size;
	cmd->offset = offset;

	return !check_command(dev, dev->nand ? CMD_NAND_BEGIN : CMD_FLASH_BEGIN,
			sizeof(cmd_flash_begin_t), CHECKSUM_NONE, NULL, TIMEOUT_DEFAULT);
}

bool
cmd_flash_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq)
{
	cmd_flash_block_t *cmd = (cmd_flash_block_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_block_t));
	cmd->size = data_len;
	cmd->seq = seq;
	cmd->rev1 = 0;
	cmd->rev2 = 0;

	uint8_t *req_data = (uint8_t *)dev->req_cmd + sizeof(cmd_flash_block_t);
	memcpy(req_data, data, data_len);

	uint32_t in_len = sizeof(cmd_flash_block_t) + data_len;

	uint8_t ret = check_command(dev, dev->nand ? CMD_NAND_DATA : CMD_FLASH_DATA, in_len,
			checksum(data, data_len), NULL, TIMEOUT_FLASH_DATA);

	if (ret != 0x00) {
		LOGD("DEBUG: Failed writing block %d: %02X", seq, ret);
	}

	if (ret == 0x0A) {
		msleep(250);
	}

	return !ret;
}

bool
cmd_flash_finish(cskburn_serial_device_t *dev)
{
	cmd_flash_finish_t *cmd = (cmd_flash_finish_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_finish_t));
	cmd->option = 0xFF;  // no-op on CSK4, do nothing on CSK6
	cmd->address = 0;

	return !check_command(dev, dev->nand ? CMD_NAND_END : CMD_FLASH_END, sizeof(uint32_t),
			CHECKSUM_NONE, NULL, TIMEOUT_FLASH_DATA);
}

bool
cmd_flash_md5sum(cskburn_serial_device_t *dev, uint32_t address, uint32_t size, uint8_t *md5)
{
	uint8_t ret_buf[STATUS_BYTES_LEN + 16];
	uint16_t ret_len = 0;

	cmd_flash_md5_t *cmd = (cmd_flash_md5_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_md5_t));
	cmd->address = address;
	cmd->size = size;

	if (!command(dev, dev->nand ? CMD_NAND_MD5 : CMD_SPI_FLASH_MD5, sizeof(cmd_flash_md5_t),
				CHECKSUM_NONE, NULL, ret_buf, &ret_len, sizeof(ret_buf), TIMEOUT_FLASH_MD5SUM)) {
		return false;
	}

	if (ret_len < STATUS_BYTES_LEN) {
		LOGD("DEBUG: Interrupted serial read");
		return false;
	}

	if (ret_buf[0] != 0) {
		LOGD("DEBUG: Unexpected device response: %02X%02X", ret_buf[0], ret_buf[1]);
		return false;
	}

	memcpy(md5, ret_buf + 2, 16);

	return true;
}

bool
cmd_change_baud(cskburn_serial_device_t *dev, uint32_t baud, uint32_t old_baud)
{
	cmd_change_baud_t *cmd = (cmd_change_baud_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_change_baud_t));
	cmd->baud = baud;
	cmd->old_baud = old_baud;

	if (!command(dev, CMD_CHANGE_BAUDRATE, sizeof(cmd_change_baud_t), CHECKSUM_NONE, NULL, NULL,
				NULL, 0, 1000)) {
		return false;
	}

	return serial_set_speed(dev->handle, baud);
}
