#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libusb.h>

#include <log.h>
#include <msleep.h>

#include "core.h"
#include "burner.h"
#include "crc64.h"

#define IMG_DATA_BUF_SIZE 4096  // bytes
#define RETRY_COMMAND_COUNT 3

#define DATA_OUT_TIMEOUT_MS 40 * 5
#define DATA_INTERVAL_MS 20
#define CR_OUT_TIMEOUT_MS 20 * 5
#define CR_IN_TIMEOUT_MS 20 * 10  // 100ms is not enough if usb cable is too long or instable
#define CR_INTERVAL_MS 10

#define FLAG_D2H (1 << 0)
#define FLAG_RESPONSE (1 << 1)
#define FLAG_EXT_FLASH (1 << 2)
#define FLAG_VERIFY (1 << 3)
#define FLAG_LOCK (1 << 4)
#define FLAG_ERASE_ALL (1 << 5)
#define FLAG_UNLOCK (1 << 6)

typedef enum {
	// CMD_CODE_UNUSED = 0,
	CMD_CODE_ANY = 0,
	CMD_CODE_H2D_FLASH_WRITE,  // flash burn from host
	CMD_CODE_H2D_VERIFY_ONLY,  // verify flash content with image data
	CMD_CODE_H2D_ERASE_ONLY,  // erase flash of specified range
	CMD_CODE_H2D_SYNC,  // sync with device
	CMD_CODE_H2D_GET_RESULT,  // get result of last operation (flash_write, verify_only and
							  // erease_only)
	CMD_CODE_H2D_SET_CRC64,  // set crc64 value of image file prior to FLASH_WRITE (set verify flag)
							 // or VERIFY_ONLY
	CMD_CODE_H2D_SHOW_DONE,  // notify device to show that burning is done

	// CMD_CODE_D2H_PAUSE_SEND,	// pause send from device
	// CMD_CODE_D2H_RESUME_SEND,	// resume send from device
	// CMD_CODE_D2H_STOP_SEND,		// stop send from device
	CMD_CODE_COUNT
} burner_cmd;

typedef enum {
	RESP_CODE_SUCCESS = 0,  // exectute command successfully
	RESP_CODE_FAILED = 1,  // failed to execute command
	RESP_CODE_READY = 2,  // device/host is ready to transfer data
	RESP_CODE_ONGOING = 3,  // command is being processing
	RESP_CODE_BUSY = 4,  // device/host is busy (need resend command or data later)
	RESP_CODE_COUNT
} burner_resp;

// USB command common header
typedef struct {
	uint32_t flag : 25;
	// bit[0] 0: host to device,		1: device to host
	// bit[1] MUST be 0 (0: command,	1: response)
	// bit[2] 0: internal flash,		1: external flash
	uint32_t cmdcode : 7;  // command
	uint32_t rsvd[3];  // reseverd (actually UNNECESSARY!)
} burner_cmd_common_t;

// FLASH_WRITE, data sent @ EP_ADDR_DATA_OUT from host to device
typedef struct {
	uint32_t flag : 25;
	// bit[0] MUST be 0 (host to device)
	// bit[1] MUST be 0 (0: command, 1: response)
	// bit[2] 0: internal flash, 			1: external flash
	// bit[3] 0: don't verify after write,	1: verify after write
	// bit[4] 0: don't lock after write,	1: lock after write
	// bit[5] 0: don't erase all,			1: erase all
	// bit[6] 0: don't unlock before write, 1: unlock before write
	uint32_t cmdcode : 7;  // command
	uint32_t address;  // address offset from flash start
	uint32_t size;  // image size, in byte
	uint32_t params;  // reserved for future use
} burner_cmd_flash_write_t;

// SET_CRC64 (called before verify), no data sent from host to device
typedef struct {
	uint32_t flag : 25;
	// bit[0] = 0: host to device,		1: device to host
	// bit[1] MUST be 0 (0: command,	1: response)
	// bit[2] 0: internal flash,		1: external flash
	uint32_t cmdcode : 7;  // command
	uint8_t crc64[8];  // crc64 result calculated from image file
	uint32_t rsvd;  // reseverd
} burner_cmd_set_crc64_t;

// USB response
typedef struct {
	uint32_t flag : 25;
	// bit[0] = 0: host to device,		1: device to host
	// bit[1] MUST be 1 (0: command,	1: response)
	// bit[2] 0: internal flash,		1: external flash
	uint32_t respcode : 7;  // RESP_CODE_XXX
	uint8_t cmdorg;  // CMD_CODE_XXX (original usb command code)
	uint8_t errdesc_len;  // size of error description, in bytes
	uint16_t val;  // return value
	uint32_t rsvd1;  // reserved for future use
	uint32_t rsvd2;  // reserved for future use
	// uint8_t errdesc[0];  // error description
} burner_resp_common_t;

static bool
burner_transmit(void *handle, void *data, uint32_t len, burner_resp_common_t *resp)
{
	int ret, xferred = 0;

	ret = libusb_bulk_transfer(
			handle, EP_ADDR_CMD_RESP_OUT, (unsigned char *)data, len, &xferred, CR_OUT_TIMEOUT_MS);

	if (ret != 0) {
		LOGD("错误: USB 数据写入失败: %d", ret);
		return false;
	}

	if ((uint32_t)xferred < len) {
		LOGD("错误: USB 数据写入中断");
		return false;
	}

	ret = libusb_bulk_transfer(handle, EP_ADDR_CMD_RESP_IN, (unsigned char *)resp,
			sizeof(burner_resp_common_t), &xferred, CR_IN_TIMEOUT_MS);

	if (ret != 0) {
		LOGD("错误: USB 数据读取失败: %d", ret);
		return false;
	}

	if (xferred != sizeof(burner_resp_common_t)) {
		// no response, it may result from that device didn't recevie command and was busy with
		// others...
		resp->respcode = RESP_CODE_BUSY;
		return false;
	}

	if (resp->respcode == RESP_CODE_FAILED) {
		LOGD("错误: 指令执行失败");
		uint8_t err[128] = {0};
		libusb_bulk_transfer(
				handle, EP_ADDR_CMD_RESP_IN, err, resp->errdesc_len, &xferred, CR_IN_TIMEOUT_MS);
		if (xferred <= resp->errdesc_len) {
			LOGD("设备返回: %s", err);
		}
		return false;
	}

	return true;
}

static inline uint64_t
calc_crc64(void *data, uint32_t len)
{
	return crc64(0, (const unsigned char *)data, len);
}

static bool
burner_set_crc64(void *handle, uint64_t crc64, uint32_t flag)
{
	burner_resp_common_t resp = {0};

	burner_cmd_set_crc64_t cmd = {
			.flag = flag,
			.cmdcode = CMD_CODE_H2D_SET_CRC64,
	};
	memcpy(&cmd.crc64[0], &crc64, sizeof(uint64_t));

	for (int i = 0; i < RETRY_COMMAND_COUNT; i++) {
		if (!burner_transmit(handle, &cmd, sizeof(cmd), &resp)) {
			return false;
		}
		if (resp.respcode == RESP_CODE_BUSY) {
			msleep(20);
		}
		if (resp.respcode == RESP_CODE_SUCCESS && resp.cmdorg == cmd.cmdcode) {
			return true;
		}
	}

	return false;
}

static bool
burner_flash_write(void *handle, uint32_t addr, uint32_t len, uint32_t flag)
{
	burner_resp_common_t resp = {0};

	burner_cmd_flash_write_t cmd = {
			.flag = flag,
			.cmdcode = CMD_CODE_H2D_FLASH_WRITE,
			.address = addr,
			.size = len,
	};

	for (int i = 0; i < 20; i++) {
		if (!burner_transmit(handle, &cmd, sizeof(cmd), &resp)) {
			return false;
		}
		if (resp.respcode == RESP_CODE_BUSY) {
			msleep(200);
		}
		if (resp.respcode == RESP_CODE_READY) {
			return true;
		}
	}

	return false;
}

static bool
burner_sync(void *handle, int retries)
{
	burner_resp_common_t resp = {0};

	burner_cmd_common_t cmd = {
			.cmdcode = CMD_CODE_H2D_SYNC,
	};

	for (int i = 0; i < retries; i++) {
		if (!burner_transmit(handle, &cmd, sizeof(cmd), &resp)) {
			return false;
		}
		if (resp.respcode == RESP_CODE_BUSY) {
			msleep(200);
		}
		if ((resp.respcode == RESP_CODE_SUCCESS || resp.respcode == RESP_CODE_READY) &&
				(resp.cmdorg == cmd.cmdcode || resp.cmdorg == CMD_CODE_ANY)) {
			return true;
		}
	}

	return false;
}

static bool
burner_get_result(void *handle, uint8_t cmdcode, int timeout)
{
	burner_resp_common_t resp = {0};

	burner_cmd_common_t cmd = {
			.cmdcode = CMD_CODE_H2D_GET_RESULT,
	};

	while (true) {
		if (!burner_transmit(handle, &cmd, sizeof(cmd), &resp)) {
			return false;
		}
		if (resp.respcode == RESP_CODE_SUCCESS &&
				(resp.cmdorg == cmdcode || resp.cmdorg == CMD_CODE_ANY)) {
			return true;
		}

		timeout -= 200;
		if (timeout < 0) {
			break;
		} else {
			msleep(200);
		}
	}

	return false;
}

bool
burner_burn(void *handle, uint32_t addr, uint8_t *image, uint32_t len,
		burner_burn_progress_cb on_progress)
{
	if (!burner_sync(handle, 10)) {
		LOGD("错误: 设备未响应");
		return false;
	}

	uint64_t crc64 = calc_crc64(image, len);
	if (!burner_set_crc64(handle, crc64, FLAG_VERIFY)) {
		LOGD("错误: 向设备传输 CRC64 失败");
		return false;
	}

	if (!burner_flash_write(handle, addr, len, FLAG_VERIFY)) {
		return false;
	}

	msleep(CR_INTERVAL_MS);

	burner_resp_common_t resp = {0};
	int ret, xferred = 0;
	uint32_t slice, wrote = 0;
	while (len - wrote > 0) {
		slice = IMG_DATA_BUF_SIZE;
		if (len - wrote < slice) slice = len - wrote;

		ret = libusb_bulk_transfer(
				handle, EP_ADDR_DATA_OUT, image + wrote, slice, &xferred, DATA_OUT_TIMEOUT_MS);

		if (ret != 0) {
			LOGD("错误: USB 数据写入失败: %d", ret);
			return false;
		}

		if ((uint32_t)xferred < slice) {
			LOGD("错误: USB 数据写入中断");
			return false;
		}

		int wait = 0;
		while (true) {
			ret = libusb_bulk_transfer(handle, EP_ADDR_DATA_IN, (unsigned char *)&resp,
					sizeof(resp), &xferred, CR_IN_TIMEOUT_MS);

			if (xferred != sizeof(resp)) {
				wait++;
				on_progress(-wait, len);
				msleep(1000);
				continue;
			}

			if (!burner_sync(handle, 3)) {
				LOGD("错误: 设备未响应");
				return false;
			}

			break;
		}

		if (resp.respcode != RESP_CODE_READY) {
			LOGD("错误: 烧写失败");
			if (resp.respcode == RESP_CODE_FAILED) {
				uint8_t err[128] = {0};
				libusb_bulk_transfer(handle, EP_ADDR_CMD_RESP_IN, err, resp.errdesc_len, &xferred,
						CR_IN_TIMEOUT_MS);
				if (xferred <= resp.errdesc_len) {
					LOGD("设备返回: %s", err);
				}
			}
			return false;
		}

		wrote += slice;

		if (on_progress != NULL) {
			on_progress(wrote, len);
		}
	}

	// it takes abut 10~15 sec to write 1MB data onto flash according to flash datasheet or
	// experience, and here we double the time if verify flag is present...
	uint32_t max_wait_ms = (len + 1024 * 1024 - 1) / (1024 * 1024) * 15 * 1000;
	max_wait_ms *= 2;  // for verify

	if (!burner_get_result(handle, CMD_CODE_H2D_FLASH_WRITE, max_wait_ms)) {
		LOGD("错误: 分区烧录失败");
		return false;
	}

	return true;
}
