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

#ifndef TRACE_DATA
#define TRACE_DATA 0
#endif

#define DIR_REQ 0x00
#define DIR_RES 0x01

#define CMD_FLASH_BEGIN 0x02
#define CMD_FLASH_DATA 0x03
#define CMD_FLASH_END 0x04
#define CMD_MEM_BEGIN 0x05
#define CMD_MEM_END 0x06
#define CMD_MEM_DATA 0x07
#define CMD_SYNC 0x08
#define CMD_READ_FLASH 0x0e
#define CMD_CHANGE_BAUDRATE 0x0f
#define CMD_SPI_FLASH_MD5 0x13
#define CMD_NAND_INIT 0x20
#define CMD_NAND_BEGIN 0x21
#define CMD_NAND_DATA 0x22
#define CMD_NAND_END 0x23
#define CMD_NAND_MD5 0x24
#define CMD_FLASH_ERASE_CHIP 0xD0
#define CMD_FLASH_ERASE_REGION 0xD1
#define CMD_READ_FLASH_ID 0xF3
#define CMD_READ_CHIP_ID 0xF4

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

// Flash 结束写入指令超时时间
// 该操作包含等待写入队列的时间，实测可到 750ms，取保险值
#define TIMEOUT_FLASH_END 2000

// Flash 擦除指令超时时间 (每 MB)
// 实测约 222KB/s，即每 MB 约 4612
// 取保守值
#define TIMEOUT_FLASH_ERASE_PER_MB 10000

// MD5 计算指令超时时间 (每 MB)
// 实测约 1882KB/s，即每 MB 约 540ms
// 取保守值
#define TIMEOUT_FLASH_MD5SUM_PER_MB 1000

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
} cmd_flash_erase_t;

typedef struct {
	uint32_t address;
	uint32_t size;
	uint32_t rev1;
	uint32_t rev2;
} cmd_flash_md5_t;

typedef struct {
	uint32_t address;
	uint32_t size;
} cmd_read_flash_t;

static ssize_t
command_send(cskburn_serial_device_t *dev, uint8_t op, uint8_t *req_buf, uint32_t req_len,
		uint32_t timeout)
{
	return slip_write(dev->slip, req_buf, req_len, timeout);
}

static ssize_t
command_recv(cskburn_serial_device_t *dev, uint8_t op, uint8_t **res_buf, uint32_t timeout)
{
	if (dev->timeout > 0 && op != CMD_SYNC) {
		timeout = dev->timeout;
	}

	uint64_t start = time_monotonic();
	do {
		ssize_t r = slip_read(dev->slip, dev->res_buf, MAX_RES_SLIP_LEN, timeout);
		if (r == 0) {
			continue;
		} else if (r == -ETIMEDOUT) {
			break;
		} else if (r < 0) {
			return r;
		}

		csk_response_t *res = (csk_response_t *)dev->res_buf;
		if (res->direction == DIR_RES && res->command == op) {
			*res_buf = dev->res_buf;
			return r;
		}
	} while (TIME_SINCE_MS(start) < timeout || (dev->timeout == -1 && op != CMD_SYNC));

	return -ETIMEDOUT;
}

static int
command(cskburn_serial_device_t *dev, uint8_t op, uint16_t in_len, uint32_t in_chk,
		uint32_t *out_val, void *out_buf, uint16_t *out_len, uint16_t out_limit, uint32_t timeout)
{
	int ret;

	serial_discard_input(dev->serial);
	serial_discard_output(dev->serial);

	LOG_TRACE("> req op=%02X len=%d", op, in_len);
#if TRACE_DATA
	LOG_DUMP(dev->req_cmd, in_len);
#endif

	csk_command_t *req = (csk_command_t *)dev->req_hdr;
	req->direction = DIR_REQ;
	req->command = op;
	req->size = in_len;
	req->checksum = in_chk;

	uint32_t req_len = sizeof(csk_command_t) + in_len;
	if ((ret = command_send(dev, op, dev->req_buf, req_len, timeout)) < 0) {
		if (ret != -ETIMEDOUT) {
			LOGD_RET(ret, "DEBUG: Failed to write command %02X", op);
		}
		goto exit;
	}

	uint8_t *res_ptr;
	if ((ret = command_recv(dev, op, &res_ptr, timeout)) < 0) {
		if (ret != -ETIMEDOUT) {
			LOGD_RET(ret, "DEBUG: Failed to read command %02X", op);
		}
		goto exit;
	}

	csk_response_t *res = (csk_response_t *)res_ptr;
	uint8_t *res_data = res_ptr + sizeof(csk_response_t);

	LOG_TRACE("< res op=%02X len=%d val=%d", res->command, res->size, res->value);
#if TRACE_DATA
	LOG_DUMP(res_data, res->size);
#endif

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

	ret = 0;

exit:
	serial_discard_input(dev->serial);
	return ret;
}

static int
check_command(cskburn_serial_device_t *dev, uint8_t op, uint16_t in_len, uint32_t in_chk,
		uint32_t *out_val, uint32_t timeout)
{
	struct {
		uint8_t error;
		uint8_t code;
	} out = {0};
	uint16_t out_len = 0;

	int ret = command(dev, op, in_len, in_chk, out_val, &out, &out_len, sizeof(out), timeout);
	if (ret < 0) {
		return ret;
	}

	if (out_len < sizeof(out)) {
		LOGD("DEBUG: Interrupted serial read of command %02X", op);
		return -EIO;
	}

	if (out.error) {
		LOGD("DEBUG: Unexpected device response of command %02X: 0x%02X", op, out.code);
		return out.code;
	} else {
		return 0;
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

static uint32_t
calc_timeout(uint32_t size, uint32_t per_mb)
{
	uint32_t mb = size / (1024 * 1024);
	return per_mb * (mb == 0 ? 1 : mb);
}

int
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

int
cmd_read_flash_id(cskburn_serial_device_t *dev, uint32_t *id)
{
	return check_command(dev, CMD_READ_FLASH_ID, 0, CHECKSUM_NONE, id, TIMEOUT_DEFAULT);
}

int
cmd_read_chip_id(cskburn_serial_device_t *dev, uint8_t *id)
{
	uint8_t ret_buf[STATUS_BYTES_LEN + CHIP_ID_LEN];
	uint16_t ret_len = 0;

	int ret = command(dev, CMD_READ_CHIP_ID, 0, CHECKSUM_NONE, NULL, ret_buf, &ret_len,
			sizeof(ret_buf), TIMEOUT_DEFAULT);

	if (ret != 0) {
		return ret;
	}

	if (ret_len < STATUS_BYTES_LEN) {
		LOGD("DEBUG: Interrupted serial read");
		return -EIO;
	}

	if (ret_buf[0] != 0) {
		LOGD("DEBUG: Unexpected device response: 0x%02X", ret_buf[1]);
		return ret_buf[1];
	}

	memcpy(id, ret_buf + STATUS_BYTES_LEN, CHIP_ID_LEN);

	return 0;
}

int
cmd_nand_init(cskburn_serial_device_t *dev, nand_config_t *config, uint64_t *size)
{
#pragma pack(1)
	struct {
		uint8_t error;
		uint8_t code;
		uint32_t blk_len;
		uint32_t blk_num;
	} out;
#pragma pack()
	uint16_t out_len = 0;

	nand_config_t *cmd = (nand_config_t *)dev->req_cmd;
	memcpy(cmd, config, sizeof(nand_config_t));

	int ret = command(dev, CMD_NAND_INIT, sizeof(nand_config_t), CHECKSUM_NONE, NULL, &out,
			&out_len, sizeof(out), TIMEOUT_FLASH_DATA);
	if (ret < 0) {
		return ret;
	}

	if (out_len < STATUS_BYTES_LEN) {
		LOGD("DEBUG: Interrupted serial read");
		return -EIO;
	}

	if (out.error) {
		LOGD("DEBUG: Unexpected device response: 0x%02X", out.code);
		return out.code;
	}

	LOGD("Inited NAND flash with block size: %u bytes, count: %u", out.blk_len, out.blk_num);

	*size = out.blk_len * out.blk_num;

	return 0;
}

int
cmd_nand_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks, uint32_t block_size,
		uint32_t offset)
{
	cmd_flash_begin_t *cmd = (cmd_flash_begin_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_begin_t));
	cmd->size = size;
	cmd->blocks = blocks;
	cmd->block_size = block_size;
	cmd->offset = offset;

	return check_command(
			dev, CMD_NAND_BEGIN, sizeof(cmd_flash_begin_t), CHECKSUM_NONE, NULL, TIMEOUT_DEFAULT);
}

int
cmd_nand_block(cskburn_serial_device_t *dev, uint8_t *data, uint32_t data_len, uint32_t seq)
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

	int ret = check_command(
			dev, CMD_NAND_DATA, in_len, checksum(data, data_len), NULL, TIMEOUT_FLASH_DATA);

	if (ret != 0) {
		LOGD("DEBUG: Failed writing block %d", seq);
	}

	if (ret == 0x0A) {
		msleep(250);
	}

	return ret;
}

int
cmd_nand_finish(cskburn_serial_device_t *dev)
{
	cmd_flash_finish_t *cmd = (cmd_flash_finish_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_finish_t));

	return check_command(
			dev, CMD_NAND_END, sizeof(uint32_t), CHECKSUM_NONE, NULL, TIMEOUT_FLASH_END);
}

int
cmd_nand_md5(cskburn_serial_device_t *dev, uint32_t address, uint32_t size, uint8_t *md5)
{
	uint8_t ret_buf[STATUS_BYTES_LEN + 16];
	uint16_t ret_len = 0;

	cmd_flash_md5_t *cmd = (cmd_flash_md5_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_md5_t));
	cmd->address = address;
	cmd->size = size;

	int ret = command(dev, CMD_NAND_MD5, sizeof(cmd_flash_md5_t), CHECKSUM_NONE, NULL, ret_buf,
			&ret_len, sizeof(ret_buf), calc_timeout(size, TIMEOUT_FLASH_MD5SUM_PER_MB));
	if (ret != 0) {
		return ret;
	}

	if (ret_len < STATUS_BYTES_LEN) {
		LOGD("DEBUG: Interrupted serial read");
		return -EIO;
	}

	if (ret_buf[0] != 0) {
		LOGD("DEBUG: Unexpected device response: 0x%02X", ret_buf[1]);
		return ret_buf[1];
	}

	memcpy(md5, ret_buf + 2, 16);

	return 0;
}

int
cmd_mem_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks, uint32_t block_size,
		uint32_t offset)
{
	cmd_mem_begin_t *cmd = (cmd_mem_begin_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_mem_begin_t));
	cmd->size = size;
	cmd->blocks = blocks;
	cmd->block_size = block_size;
	cmd->offset = offset;

	return check_command(
			dev, CMD_MEM_BEGIN, sizeof(cmd_mem_begin_t), CHECKSUM_NONE, NULL, TIMEOUT_DEFAULT);
}

int
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

	return check_command(
			dev, CMD_MEM_DATA, in_len, checksum(data, data_len), NULL, TIMEOUT_MEM_DATA);
}

int
cmd_mem_finish(cskburn_serial_device_t *dev, cmd_finish_action_t action, uint32_t address)
{
	cmd_mem_finish_t *cmd = (cmd_mem_finish_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_mem_finish_t));
	cmd->option = action;
	cmd->address = address;

	return check_command(
			dev, CMD_MEM_END, sizeof(cmd_mem_finish_t), CHECKSUM_NONE, NULL, TIMEOUT_DEFAULT);
}

int
cmd_flash_begin(cskburn_serial_device_t *dev, uint32_t size, uint32_t blocks, uint32_t block_size,
		uint32_t offset)
{
	cmd_flash_begin_t *cmd = (cmd_flash_begin_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_begin_t));
	cmd->size = size;
	cmd->blocks = blocks;
	cmd->block_size = block_size;
	cmd->offset = offset;

	return check_command(
			dev, CMD_FLASH_BEGIN, sizeof(cmd_flash_begin_t), CHECKSUM_NONE, NULL, TIMEOUT_DEFAULT);
}

int
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

	int ret = check_command(
			dev, CMD_FLASH_DATA, in_len, checksum(data, data_len), NULL, TIMEOUT_FLASH_DATA);

	if (ret != 0) {
		LOGD("DEBUG: Failed writing block %d", seq);
	}

	if (ret == 0x0A) {
		msleep(250);
	}

	return ret;
}

int
cmd_flash_finish(cskburn_serial_device_t *dev)
{
	cmd_flash_finish_t *cmd = (cmd_flash_finish_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_finish_t));
	cmd->option = 0xFF;  // no-op on CSK4, do nothing on CSK6
	cmd->address = 0;

	return check_command(
			dev, CMD_FLASH_END, sizeof(uint32_t), CHECKSUM_NONE, NULL, TIMEOUT_FLASH_END);
}

int
cmd_flash_erase_chip(cskburn_serial_device_t *dev)
{
	// TODO: 暂时假定 32MB flash
	return check_command(
			dev, CMD_FLASH_ERASE_CHIP, 0, CHECKSUM_NONE, NULL, TIMEOUT_FLASH_ERASE_PER_MB * 32);
}

int
cmd_flash_erase_region(cskburn_serial_device_t *dev, uint32_t address, uint32_t size)
{
	cmd_flash_erase_t *cmd = (cmd_flash_erase_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_erase_t));
	cmd->address = address;
	cmd->size = size;

	return check_command(dev, CMD_FLASH_ERASE_REGION, sizeof(cmd_flash_erase_t), CHECKSUM_NONE,
			NULL, calc_timeout(size, TIMEOUT_FLASH_ERASE_PER_MB));
}

int
cmd_flash_md5sum(cskburn_serial_device_t *dev, uint32_t address, uint32_t size, uint8_t *md5)
{
	uint8_t ret_buf[STATUS_BYTES_LEN + 16];
	uint16_t ret_len = 0;

	cmd_flash_md5_t *cmd = (cmd_flash_md5_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_flash_md5_t));
	cmd->address = address;
	cmd->size = size;

	int ret = command(dev, CMD_SPI_FLASH_MD5, sizeof(cmd_flash_md5_t), CHECKSUM_NONE, NULL, ret_buf,
			&ret_len, sizeof(ret_buf), calc_timeout(size, TIMEOUT_FLASH_MD5SUM_PER_MB));
	if (ret != 0) {
		return ret;
	}

	if (ret_len < STATUS_BYTES_LEN) {
		LOGD("DEBUG: Interrupted serial read");
		return -EIO;
	}

	if (ret_buf[0] != 0) {
		LOGD("DEBUG: Unexpected device response: 0x%02X", ret_buf[1]);
		return ret_buf[1];
	}

	memcpy(md5, ret_buf + 2, 16);

	return 0;
}

int
cmd_read_flash(cskburn_serial_device_t *dev, uint32_t address, uint32_t size, uint8_t *data,
		uint32_t *data_len)
{
	uint8_t ret_buf[STATUS_BYTES_LEN + 64];
	uint16_t ret_len = 0;

	cmd_read_flash_t *cmd = (cmd_read_flash_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_read_flash_t));
	cmd->address = address;
	cmd->size = size;

	int ret = command(dev, CMD_READ_FLASH, sizeof(cmd_read_flash_t), CHECKSUM_NONE, NULL, ret_buf,
			&ret_len, sizeof(ret_buf), TIMEOUT_FLASH_DATA);
	if (ret != 0) {
		return ret;
	}

	if (ret_len < STATUS_BYTES_LEN) {
		LOGD("DEBUG: Interrupted serial read");
		return -EIO;
	}

	if (ret_buf[0] != 0) {
		LOGD("DEBUG: Unexpected device response: 0x%02X", ret_buf[1]);
		return ret_buf[1];
	}

	*data_len = ret_len - STATUS_BYTES_LEN;
	memcpy(data, ret_buf + STATUS_BYTES_LEN, *data_len);

	return 0;
}

int
cmd_change_baud(cskburn_serial_device_t *dev, uint32_t baud, uint32_t old_baud)
{
	int ret;

	cmd_change_baud_t *cmd = (cmd_change_baud_t *)dev->req_cmd;
	memset(cmd, 0, sizeof(cmd_change_baud_t));
	cmd->baud = baud;
	cmd->old_baud = old_baud;

	ret = command(dev, CMD_CHANGE_BAUDRATE, sizeof(cmd_change_baud_t), CHECKSUM_NONE, NULL, NULL,
			NULL, 0, 1000);
	if (ret != 0) {
		return ret;
	}

	ret = serial_set_speed(dev->serial, baud);
	if (ret != 0) {
		LOGD_RET(ret, "DEBUG: Failed to set baudrate");
		return ret;
	}

	return 0;
}
