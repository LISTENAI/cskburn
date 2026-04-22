#include "cskburn_errors.h"

#include <errno.h>

const char *
cskburn_strerror(int err)
{
	if (err > 0) {
		// Device-reported status byte. Meaning is contextual; CLI prints
		// the byte value alongside operation context.
		return "Device reported an error";
	}

	int code = -err;
	switch (code) {
		/* 1xxx — argument */
		case CSKBURN_ERR_ARG_INVALID:
			return "Invalid argument";
		case CSKBURN_ERR_ARG_NO_PORT:
			return "No serial port or USB device specified";
		case CSKBURN_ERR_ARG_UNSUPPORTED_OP:
			return "Operation not supported in this mode";
		case CSKBURN_ERR_ARG_TOO_MANY_PARTS:
			return "Too many partitions";
		case CSKBURN_ERR_ARG_ADDR_UNALIGNED:
			return "Address or size is not aligned";
		case CSKBURN_ERR_ARG_ADDR_OUT_OF_BOUNDS:
			return "Address or size exceeds target capacity";

		/* 2xxx — file I/O */
		case CSKBURN_ERR_FILE_READ_FAILED:
			return "Failed to read input file";
		case CSKBURN_ERR_FILE_WRITE_FAILED:
			return "Failed to write output file";
		case CSKBURN_ERR_HEX_PARSE_FAILED:
			return "Failed to parse HEX file";

		/* 3xxx — serial port */
		case CSKBURN_ERR_SERIAL_NOT_FOUND:
			return "Serial port not found";
		case CSKBURN_ERR_SERIAL_ACCESS_DENIED:
			return "Permission denied to access serial port";
		case CSKBURN_ERR_SERIAL_BUSY:
			return "Serial port is already in use";
		case CSKBURN_ERR_SERIAL_OPEN_FAILED:
			return "Failed to open serial port";
		case CSKBURN_ERR_SERIAL_CONFIG_FAILED:
			return "Failed to configure serial port";

		/* 4xxx — probe / reset */
		case CSKBURN_ERR_PROBE_NO_SYNC:
			return "Device did not respond after reset (check wiring and BOOT pin)";
		case CSKBURN_ERR_RESET_PIN_FAILED:
			return "Failed to toggle RTS/DTR lines for reset";

		/* 5xxx — entering update mode */
		case CSKBURN_ERR_ROM_BAUD_REJECTED:
			return "ROM rejected baud rate change";
		case CSKBURN_ERR_ROM_SYNC_LOST:
			return "Lost sync with ROM after baud rate change";
		case CSKBURN_ERR_BURNER_LOAD_FAILED:
			return "Failed to load RAM burner";
		case CSKBURN_ERR_BURNER_NO_RESPONSE:
			return "Burner did not start after being loaded";
		case CSKBURN_ERR_BAUD_REJECTED:
			return "Burner rejected baud rate change";
		case CSKBURN_ERR_BAUD_SYNC_LOST:
			return "Lost sync with burner after baud rate change";

		/* 6xxx — device info */
		case CSKBURN_ERR_CHIP_ID_READ_FAILED:
			return "Failed to read chip ID";
		case CSKBURN_ERR_FLASH_ID_READ_FAILED:
			return "Failed to read flash ID";
		case CSKBURN_ERR_FLASH_NOT_DETECTED:
			return "No flash detected (ID reads as all zeros or all ones)";
		case CSKBURN_ERR_NAND_INIT_FAILED:
			return "Failed to initialize NAND (check --nand-* pin configuration)";

		/* 7xxx — flash / NAND / RAM ops */
		case CSKBURN_ERR_FLASH_READ_FAILED:
			return "Failed to read flash";
		case CSKBURN_ERR_FLASH_ERASE_FAILED:
			return "Failed to erase flash";
		case CSKBURN_ERR_FLASH_WRITE_FAILED:
			return "Failed to write flash";
		case CSKBURN_ERR_NAND_WRITE_FAILED:
			return "Failed to write NAND";
		case CSKBURN_ERR_RAM_WRITE_FAILED:
			return "Failed to write RAM";

		/* 8xxx — verification */
		case CSKBURN_ERR_VERIFY_READ_FAILED:
			return "Failed to compute MD5 on device";
		case CSKBURN_ERR_VERIFY_LOCAL_MD5_FAILED:
			return "Failed to compute MD5 on local file";
		case CSKBURN_ERR_VERIFY_MISMATCH:
			return "Verification failed: MD5 does not match";

		/* 9xxx — USB backend */
		case CSKBURN_ERR_USB_INIT_FAILED:
			return "Failed to initialize USB backend";
		case CSKBURN_ERR_USB_DEVICE_NOT_FOUND:
			return "USB device not found";
		case CSKBURN_ERR_USB_ENTER_FAILED:
			return "Failed to enter USB update mode";
		case CSKBURN_ERR_USB_WRITE_FAILED:
			return "Failed to write data over USB";

		/* Passthrough errno values — rare, usually out-of-memory */
		case ENOMEM:
			return "Out of memory";
		case EINVAL:
			return "Invalid argument";
		case ENOTSUP:
			return "Operation not supported";

		default:
			return "Unknown error";
	}
}
