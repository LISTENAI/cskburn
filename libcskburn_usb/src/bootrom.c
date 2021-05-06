#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libusb.h>

#include <log.h>
#include <msleep.h>

#include "core.h"
#include "bootrom.h"

#define DATA_BUF_SIZE 2048

typedef enum {
	BOOTROM_READ_VERSION = 0x01,
	BOOTROM_MEM_BEGIN = 0x05,
	BOOTROM_MEM_END = 0x06,
	BOOTROM_MEM_DATA = 0x07,
	BOOTROM_SYNC = 0x08,
} bootrom_command;

typedef enum {
	BOOTROM_OK = 0,

	BOOTROM_BAD_DATA_LEN = 0xC0,
	BOOTROM_BAD_DATA_CHECKSUM = 0xC1,
	BOOTROM_BAD_BLOCKSIZE = 0xC2,
	BOOTROM_INVALID_COMMAND = 0xC3,
	BOOTROM_FAILED_SPI_OP = 0xC4,
	BOOTROM_FAILED_SPI_UNLOCK = 0xC5,
	BOOTROM_NOT_IN_FLASH_MODE = 0xC6,
	BOOTROM_INFLATE_ERROR = 0xC7,
	BOOTROM_NOT_ENOUGH_DATA = 0xC8,
	BOOTROM_TOO_MUCH_DATA = 0xC9,
	BOOTROM_BAD_DATA_SEQ = 0xCA,

	BOOTROM_IMG_HDR_MARK_ERROR = 0xF0,
	BOOTROM_IMG_HDR_RSAKEY_OFFSET_ERROR = 0xF1,
	BOOTROM_IMG_HDR_IMGHASH_OFFSET_ERROR = 0xF2,
	BOOTROM_IMG_HDR_AESKEY_OFFSET_ERROR = 0xF3,
	BOOTROM_IMG_HDR_CMDSBLK_OFFSET_ERROR = 0xF4,
	BOOTROM_IMG_HDR_RSA_KEY_ERROR = 0xF5,
	BOOTROM_IMG_HDR_RSA_DECRYT_ERROR = 0xF6,
	BOOTROM_IMG_HDR_RSA_SIG_ERROR = 0xF7,
	BOOTROM_IMG_HASH_ERROR = 0xF8,
	BOOTROM_IMG_UNKNOWN_ERROR = 0xFE,

	BOOTROM_CMD_NOT_IMPLEMENTED = 0xFF,
} bootrom_command_error;

typedef struct {
	uint8_t zero;
	uint8_t op;  // bootrom_command
	uint16_t data_len;
	uint32_t checksum;
	// uint8_t data_buf[0];
} bootrom_command_req_t;

typedef struct {
	uint8_t error;  // bootrom_command_error
	uint8_t status;
} bootrom_command_data_status_t;

typedef struct {
	uint32_t total_size;
	uint32_t packets;
	uint32_t pkt_size;
	uint32_t moffset;
} bootrom_mem_begin_t;

typedef struct {
	uint32_t mdata_len;  // Mem data length, little endian 32bit word.
	uint32_t seq_no;  // Sequence number, little endian 32bit word, 0 based.
	uint32_t rsvd[2];  // Two words of 0, unused.
	// uint8_t mdata[0];  // Payload of mem data
} bootrom_mem_data_t;

typedef struct {
	uint32_t exec_flag;
	uint32_t entry_point;
} bootrom_mem_end_t;

static inline uint32_t
padded(uint32_t size)
{
	return (size + DATA_BUF_SIZE - 1) / DATA_BUF_SIZE;
}

static uint8_t
calc_data_checksum(uint8_t *payload, uint32_t payload_len)
{
	uint32_t i;
	uint8_t res = 0xEF;

	if (payload == NULL || payload_len == 0) return 0;

	for (i = 0; i < payload_len; i++) {
		res ^= payload[i];
	}

	return res;
}

static bool
send_mem_command(void *handle, uint8_t *buf, uint32_t data_len)
{
	int ret, xferred = 0;

	uint8_t resp[sizeof(bootrom_command_req_t) + sizeof(bootrom_command_data_status_t)] = {0};
	bootrom_command_data_status_t *data_status =
			(bootrom_command_data_status_t *)(resp + sizeof(bootrom_command_req_t));

	ret = libusb_bulk_transfer(
			handle, EP_ADDR_DATA_OUT, (unsigned char *)buf, data_len, &xferred, 0);

	if (ret != 0) {
		LOGD("错误: USB 数据写入失败: %d", ret);
		return false;
	}

	if ((uint32_t)xferred < data_len) {
		LOGD("错误: USB 数据写入中断");
		return false;
	}

	ret = libusb_bulk_transfer(
			handle, EP_ADDR_CMD_RESP_IN, (unsigned char *)&resp, sizeof(resp), &xferred, 0);

	if (ret != 0) {
		LOGD("错误: USB 数据读取失败: %d", ret);
		return false;
	}

	if (xferred < sizeof(resp)) {
		LOGD("错误: USB 数据读取中断");
		return false;
	}

	if (data_status->error >= 0xF0) {
		LOGD("错误: 设备返回致命错误: %02X", data_status->error);
		return false;
	}

	if (data_status->error != 0) {
		LOGD("错误: 设备返回错误: %02X", data_status->error);
		return false;
	}

	return true;
}

static bool
bootrom_mem_begin(void *handle, uint8_t *buf, uint32_t buf_len, uint32_t total_len)
{
	uint32_t header_len = sizeof(bootrom_command_req_t) + sizeof(bootrom_mem_begin_t);
	bootrom_command_req_t *cmd = (bootrom_command_req_t *)buf;
	bootrom_mem_begin_t *mem = (bootrom_mem_begin_t *)(buf + sizeof(bootrom_command_req_t));

	assert(buf != NULL && total_len > 0);
	assert(buf_len >= header_len);

	memset(buf, 0, header_len);

	cmd->op = BOOTROM_MEM_BEGIN;
	cmd->data_len = sizeof(bootrom_mem_begin_t);
	cmd->checksum = 0;

	mem->total_size = total_len;
	mem->packets = padded(total_len);
	mem->pkt_size = DATA_BUF_SIZE;
	mem->moffset = 0;

	return send_mem_command(handle, buf, header_len);
}

static bool
bootrom_mem_data(void *handle, uint8_t *buf, uint32_t buf_len, uint8_t *payload,
		uint32_t payload_len, uint32_t seq_no)
{
	uint32_t header_len = sizeof(bootrom_command_req_t) + sizeof(bootrom_mem_data_t);
	bootrom_command_req_t *cmd = (bootrom_command_req_t *)buf;
	bootrom_mem_data_t *mem = (bootrom_mem_data_t *)(buf + sizeof(bootrom_command_req_t));

	assert(buf != NULL && payload != NULL);
	assert(buf_len >= header_len + payload_len);

	memset(buf, 0, header_len);

	cmd->op = BOOTROM_MEM_DATA;
	cmd->data_len = (uint16_t)sizeof(bootrom_mem_data_t) + payload_len;
	cmd->checksum = calc_data_checksum(payload, payload_len);

	mem->mdata_len = payload_len;
	mem->seq_no = seq_no;

	memcpy(buf + header_len, payload, payload_len);

	return send_mem_command(handle, buf, header_len + payload_len);
}

static bool
bootrom_mem_end(void *handle, uint8_t *buf, uint32_t buf_len)
{
	uint32_t header_len = sizeof(bootrom_command_req_t) + sizeof(bootrom_mem_end_t);
	bootrom_command_req_t *cmd = (bootrom_command_req_t *)buf;
	bootrom_mem_end_t *mem = (bootrom_mem_end_t *)(buf + sizeof(bootrom_command_req_t));

	assert(buf != NULL);
	assert(buf_len >= header_len);

	memset(buf, 0, header_len);

	cmd->op = BOOTROM_MEM_END;
	cmd->data_len = (uint16_t)sizeof(bootrom_mem_end_t);
	cmd->checksum = 0;

	mem->exec_flag = 0;
	mem->entry_point = 0;

	return send_mem_command(handle, buf, header_len);
}

bool
bootrom_load(void *handle, uint8_t *burner, uint32_t len)
{
	uint32_t hdr_len = sizeof(bootrom_command_req_t) + sizeof(bootrom_mem_data_t);
	uint32_t buf_len = hdr_len + DATA_BUF_SIZE;
	uint8_t *buf = malloc(buf_len);

	if (!bootrom_mem_begin(handle, buf, buf_len, len)) {
		LOGD("错误: MEM_BEGIN 发送失败");
		goto err;
	}

	uint32_t xferred = 0;
	uint32_t packet_len = 0;
	uint32_t seq = 0;
	while (1) {
		packet_len = DATA_BUF_SIZE;
		if (xferred + packet_len > len) {
			packet_len = len - xferred;
		}

		if (!bootrom_mem_data(handle, buf, buf_len, burner + xferred, packet_len, seq)) {
			LOGD("错误: MEM_DATA 发送失败");
			goto err;
		}

		xferred += packet_len;
		seq++;

		if (xferred >= len) {
			break;
		}

		msleep(10);
	}

	if (!bootrom_mem_end(handle, buf, buf_len)) {
		LOGD("错误: MEM_END 发送失败");
		goto err;
	}

	return true;

err:
	free(buf);
	return false;
}
