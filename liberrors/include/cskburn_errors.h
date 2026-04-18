#ifndef __LIB_CSKBURN_ERRORS__
#define __LIB_CSKBURN_ERRORS__

/**
 * cskburn error codes.
 *
 * All cskburn APIs follow the Linux convention: 0 on success, a positive value
 * for a device-reported status byte (contextual to the specific command), and
 * a negative value for an error. The absolute value of a negative return is
 * either a standard errno or one of the CSKBURN_ERR_* codes below.
 *
 * Codes are assigned in ranges so GUI front-ends can map them to more detailed
 * guidance without parsing text. Each code corresponds to exactly one root
 * cause at the software level.
 */
typedef enum {
	CSKBURN_OK = 0,

	/* 1xxx — CLI / argument validation */
	CSKBURN_ERR_ARG_INVALID = 1001,
	CSKBURN_ERR_ARG_NO_SERIAL = 1002,
	CSKBURN_ERR_ARG_UNSUPPORTED_OP = 1003,
	CSKBURN_ERR_ARG_TOO_MANY_PARTS = 1004,
	CSKBURN_ERR_ARG_ADDR_UNALIGNED = 1005,
	CSKBURN_ERR_ARG_ADDR_OUT_OF_BOUNDS = 1006,

	/* 2xxx — local file I/O */
	CSKBURN_ERR_FILE_READ_FAILED = 2001,
	CSKBURN_ERR_FILE_WRITE_FAILED = 2002,
	CSKBURN_ERR_HEX_PARSE_FAILED = 2003,

	/* 3xxx — serial port open/config */
	CSKBURN_ERR_SERIAL_NOT_FOUND = 3001,
	CSKBURN_ERR_SERIAL_ACCESS_DENIED = 3002,
	CSKBURN_ERR_SERIAL_BUSY = 3003,
	CSKBURN_ERR_SERIAL_OPEN_FAILED = 3004,
	CSKBURN_ERR_SERIAL_CONFIG_FAILED = 3005,

	/* 4xxx — probe / reset */
	CSKBURN_ERR_PROBE_NO_SYNC = 4001,
	CSKBURN_ERR_RESET_PIN_FAILED = 4002,

	/* 5xxx — entering update mode (burner load, baud change) */
	CSKBURN_ERR_ROM_BAUD_REJECTED = 5001,
	CSKBURN_ERR_ROM_SYNC_LOST = 5002,
	CSKBURN_ERR_BURNER_LOAD_FAILED = 5003,
	CSKBURN_ERR_BURNER_NO_RESPONSE = 5004,
	CSKBURN_ERR_BAUD_REJECTED = 5005,
	CSKBURN_ERR_BAUD_SYNC_LOST = 5006,

	/* 6xxx — device/flash info */
	CSKBURN_ERR_CHIP_ID_READ_FAILED = 6001,
	CSKBURN_ERR_FLASH_ID_READ_FAILED = 6002,
	CSKBURN_ERR_FLASH_NOT_DETECTED = 6003,
	CSKBURN_ERR_NAND_INIT_FAILED = 6004,

	/* 7xxx — flash / NAND / RAM operations */
	CSKBURN_ERR_FLASH_READ_FAILED = 7001,
	CSKBURN_ERR_FLASH_ERASE_FAILED = 7002,
	CSKBURN_ERR_FLASH_WRITE_FAILED = 7003,
	CSKBURN_ERR_NAND_WRITE_FAILED = 7004,
	CSKBURN_ERR_RAM_WRITE_FAILED = 7005,

	/* 8xxx — verification */
	CSKBURN_ERR_VERIFY_READ_FAILED = 8001,
	CSKBURN_ERR_VERIFY_LOCAL_MD5_FAILED = 8002,
	CSKBURN_ERR_VERIFY_MISMATCH = 8003,
} cskburn_err_t;

/**
 * @brief Translate a cskburn error code to a short English description.
 *
 * @param err A non-zero return value from a cskburn API. May be:
 *   - a negative CSKBURN_ERR_* code
 *   - a negative errno
 *   - a positive device-reported status byte
 *
 * @return A static string describing the error. Never NULL.
 */
const char *cskburn_strerror(int err);

#endif  // __LIB_CSKBURN_ERRORS__
