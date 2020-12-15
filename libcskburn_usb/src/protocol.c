#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libusb.h>

#include "protocol.h"

static inline uint32_t
padded(uint32_t size)
{
	return (size + DATA_BUF_SIZE - 1) / DATA_BUF_SIZE;
}

static bool
send_mem_command(cskburn_usb_device_t *dev, uint8_t *buf, uint32_t data_len)
{
	int ret, xferred = 0;

	uint8_t resp[sizeof(csk_command_req_t) + sizeof(csk_command_data_status_t)] = {0};
	csk_command_data_status_t *data_status =
			(csk_command_data_status_t *)(resp + sizeof(csk_command_req_t));

	ret = libusb_bulk_transfer(
			dev->handle, EP_ADDR_DATA_OUT, (unsigned char *)buf, data_len, &xferred, 0);

	if (ret != 0) {
		printf("错误: USB 数据写入失败: %d\n", ret);
		return false;
	}

	if (xferred < data_len) {
		printf("错误: USB 数据写入中断\n");
		return false;
	}

	ret = libusb_bulk_transfer(
			dev->handle, EP_ADDR_CMD_RESP_IN, (unsigned char *)&resp, sizeof(resp), &xferred, 0);

	if (ret != 0) {
		printf("错误: USB 数据读取失败: %d\n", ret);
		return false;
	}

	if (xferred < sizeof(resp)) {
		printf("错误: USB 数据读取中断\n");
		return false;
	}

	if (data_status->error >= 0xF0) {
		printf("错误: 设备返回致命错误: %02X\n", data_status->error);
		return false;
	}

	if (data_status->error != 0) {
		printf("错误: 设备返回错误: %02X\n", data_status->error);
		return false;
	}

	return true;
}

bool
send_mem_begin_command(
		cskburn_usb_device_t *dev, uint8_t *buf, uint32_t buf_len, uint32_t total_len)
{
	uint32_t header_len = sizeof(csk_command_req_t) + sizeof(csk_mem_begin_t);
	csk_command_req_t *cmd = (csk_command_req_t *)buf;
	csk_mem_begin_t *mem = (csk_mem_begin_t *)(buf + sizeof(csk_command_req_t));

	assert(buf != NULL && total_len > 0);
	assert(buf_len >= header_len);

	memset(buf, 0, header_len);

	cmd->op = CSK_MEM_BEGIN;
	cmd->data_len = sizeof(csk_mem_begin_t);
	cmd->checksum = 0;

	mem->total_size = total_len;
	mem->packets = padded(total_len);
	mem->pkt_size = DATA_BUF_SIZE;
	mem->moffset = 0;

	return send_mem_command(dev, buf, header_len);
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

bool
send_mem_data_command(cskburn_usb_device_t *dev, uint8_t *buf, uint32_t buf_len, uint8_t *payload,
		uint32_t payload_len, uint32_t seq_no)
{
	uint32_t header_len = sizeof(csk_command_req_t) + sizeof(csk_mem_data_t);
	csk_command_req_t *cmd = (csk_command_req_t *)buf;
	csk_mem_data_t *mem = (csk_mem_data_t *)(buf + sizeof(csk_command_req_t));

	assert(buf != NULL && payload != NULL);
	assert(buf_len >= header_len + payload_len);

	memset(buf, 0, header_len);

	cmd->op = CSK_MEM_DATA;
	cmd->data_len = sizeof(csk_mem_data_t) + payload_len;
	cmd->checksum = calc_data_checksum(payload, payload_len);

	mem->mdata_len = payload_len;
	mem->seq_no = seq_no;

	memcpy(buf + header_len, payload, payload_len);

	return send_mem_command(dev, buf, header_len + payload_len);
}

bool
send_mem_end_command(cskburn_usb_device_t *dev, uint8_t *buf, uint32_t buf_len)
{
	uint32_t header_len = sizeof(csk_command_req_t) + sizeof(csk_mem_end_t);
	csk_command_req_t *cmd = (csk_command_req_t *)buf;
	csk_mem_end_t *mem = (csk_mem_end_t *)(buf + sizeof(csk_command_req_t));

	assert(buf != NULL);
	assert(buf_len >= header_len);

	memset(buf, 0, header_len);

	cmd->op = CSK_MEM_END;
	cmd->data_len = sizeof(csk_mem_end_t);
	cmd->checksum = 0;

	mem->exec_flag = 0;
	mem->entry_point = 0;

	return send_mem_command(dev, buf, header_len);
}
