#ifndef __LIB_CSKBURN_USB_PROTOCOL__
#define __LIB_CSKBURN_USB_PROTOCOL__

#include <stdint.h>
#include <stdbool.h>

#include <cskburn_usb.h>

#define EP_ADDR_DATA_IN (3 | LIBUSB_ENDPOINT_IN)
#define EP_ADDR_DATA_OUT (4 | LIBUSB_ENDPOINT_OUT)
#define EP_ADDR_CMD_RESP_IN (5 | LIBUSB_ENDPOINT_IN)
#define EP_ADDR_CMD_RESP_OUT (6 | LIBUSB_ENDPOINT_OUT)

#define DATA_BUF_SIZE 2048

typedef enum {
	CSK_READ_VERSION = 0x01,
	CSK_MEM_BEGIN = 0x05,
	CSK_MEM_END = 0x06,
	CSK_MEM_DATA = 0x07,
	CSK_SYNC = 0x08,
} csk_command;

typedef enum {
	CSK_OK = 0,

	CSK_BAD_DATA_LEN = 0xC0,
	CSK_BAD_DATA_CHECKSUM = 0xC1,
	CSK_BAD_BLOCKSIZE = 0xC2,
	CSK_INVALID_COMMAND = 0xC3,
	CSK_FAILED_SPI_OP = 0xC4,
	CSK_FAILED_SPI_UNLOCK = 0xC5,
	CSK_NOT_IN_FLASH_MODE = 0xC6,
	CSK_INFLATE_ERROR = 0xC7,
	CSK_NOT_ENOUGH_DATA = 0xC8,
	CSK_TOO_MUCH_DATA = 0xC9,
	CSK_BAD_DATA_SEQ = 0xCA,

	CSK_IMG_HDR_MARK_ERROR = 0xF0,
	CSK_IMG_HDR_RSAKEY_OFFSET_ERROR = 0xF1,
	CSK_IMG_HDR_IMGHASH_OFFSET_ERROR = 0xF2,
	CSK_IMG_HDR_AESKEY_OFFSET_ERROR = 0xF3,
	CSK_IMG_HDR_CMDSBLK_OFFSET_ERROR = 0xF4,
	CSK_IMG_HDR_RSA_KEY_ERROR = 0xF5,
	CSK_IMG_HDR_RSA_DECRYT_ERROR = 0xF6,
	CSK_IMG_HDR_RSA_SIG_ERROR = 0xF7,
	CSK_IMG_HASH_ERROR = 0xF8,
	CSK_IMG_UNKNOWN_ERROR = 0xFE,

	CSK_CMD_NOT_IMPLEMENTED = 0xFF,
} csk_command_error;

typedef struct {
	uint8_t zero;
	uint8_t op;  // csk_command
	uint16_t data_len;
	uint64_t checksum;
	uint8_t data_buf[0];
} csk_command_req_t;

typedef struct {
	uint8_t error;  // csk_command_error
	uint8_t status;
} csk_command_data_status_t;

typedef struct {
	uint64_t total_size;
	uint64_t packets;
	uint64_t pkt_size;
	uint64_t moffset;
} csk_mem_begin_t;

typedef struct {
	uint64_t mdata_len;  // Mem data length, little endian 32bit word.
	uint64_t seq_no;  // Sequence number, little endian 32bit word, 0 based.
	uint64_t rsvd[2];  // Two words of 0, unused.
	uint8_t mdata[0];  // Payload of mem data
} csk_mem_data_t;

typedef struct {
	uint64_t exec_flag;
	uint64_t entry_point;
} csk_mem_end_t;

bool send_mem_begin_command(
		cskburn_usb_device_t *dev, uint8_t *buf, uint32_t buf_len, uint32_t total_len);

bool send_mem_data_command(cskburn_usb_device_t *dev, uint8_t *buf, uint32_t buf_len,
		uint8_t *payload, uint32_t payload_len, uint32_t seq_no);

bool send_mem_end_command(cskburn_usb_device_t *dev, uint8_t *buf, uint32_t buf_len);

#endif  // __LIB_CSKBURN_USB_PROTOCOL__
