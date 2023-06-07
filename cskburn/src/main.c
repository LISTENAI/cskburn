#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "msleep.h"
#include "read_parts.h"
#include "utils.h"
#ifndef WITHOUT_USB
#include "cskburn_usb.h"
#endif
#include "cskburn_serial.h"
#include "verify.h"

#define MAX_IMAGE_SIZE (32 * 1024 * 1024)
#define MAX_FLASH_PARTS 20
#define MAX_ERASE_PARTS 20
#define MAX_VERIFY_PARTS 20
#define ENTER_TRIES 5

#define DEFAULT_BAUD 3000000

#define DEFAULT_PROBE_TIMEOUT 10 * 1000
#define DEFAULT_RESET_ATTEMPTS 4
#define DEFAULT_RESET_DELAY 500

#define DEFAULT_CHIP 4

static struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'V'},
		{"verbose", no_argument, NULL, 'v'},
		{"trace", no_argument, NULL, 0},
		{"no-progress", no_argument, NULL, 0},
		{"wait", no_argument, NULL, 'w'},
		{"chip", required_argument, NULL, 'C'},
#ifndef WITHOUT_USB
		{"usb", required_argument, NULL, 'u'},
#endif
		{"serial", required_argument, NULL, 's'},
		{"baud", required_argument, NULL, 'b'},
#ifndef WITHOUT_USB
		{"repeat", no_argument, NULL, 'R'},
		{"check", no_argument, NULL, 'c'},
#endif
		{"nand", no_argument, NULL, 'n'},
		{"nand-4bit", no_argument, NULL, 0},
		{"nand-cmd", required_argument, NULL, 0},
		{"nand-clk", required_argument, NULL, 0},
		{"nand-dat0", required_argument, NULL, 0},
		{"nand-dat1", required_argument, NULL, 0},
		{"nand-dat2", required_argument, NULL, 0},
		{"nand-dat3", required_argument, NULL, 0},
		{"chip-id", no_argument, NULL, 0},
		{"erase", required_argument, NULL, 0},
		{"erase-all", no_argument, NULL, 0},
		{"verify", required_argument, NULL, 0},
		{"verify-all", no_argument, NULL, 0},
		{"probe-timeout", required_argument, NULL, 0},
		{"reset-attempts", required_argument, NULL, 0},
		{"reset-delay", required_argument, NULL, 0},
		{"pass-delay", required_argument, NULL, 0},
		{"fail-delay", required_argument, NULL, 0},
		{"burner", required_argument, NULL, 0},
		{"update-high", no_argument, NULL, 0},
		{"reset-nanokit", no_argument, NULL, 0},  // 不再需要，但是留着以便向后兼容
		{0, 0, NULL, 0},
};

static const char option_string[] = {
		"hVv"
		"w"
		"C:"
		"n"
#ifndef WITHOUT_USB
		"u:"
		"R"
		"c"
#endif
		"s:b:",
};

static const char example_serial_dev[] =
#if defined(_WIN32) || defined(_WIN64)
		"COM24";
#elif __APPLE__
		"/dev/cu.usbserial-0001";
#else
		"/dev/ttyUSB0";
#endif

typedef enum {
#ifndef WITHOUT_USB
	PROTO_USB,
#endif
	PROTO_SERIAL,
} cskburn_protocol_t;

typedef enum {
	ACTION_NONE = 0,
	ACTION_CHECK,
} cskburn_action_t;

static struct {
	bool progress;
	bool wait;
	bool repeat;
	cskburn_protocol_t protocol;
	cskburn_action_t action;
	uint32_t chip;
#ifndef WITHOUT_USB
	char *usb;
	int16_t usb_bus;
	int16_t usb_addr;
#endif
	char *serial;
	uint32_t serial_baud;
	bool read_chip_id;
	uint16_t erase_count;
	struct {
		uint32_t addr;
		uint32_t size;
	} erase_parts[MAX_ERASE_PARTS];
	bool erase_all;
	uint16_t verify_count;
	struct {
		uint32_t addr;
		uint32_t size;
	} verify_parts[MAX_VERIFY_PARTS];
	bool verify_all;
	uint32_t probe_timeout;
	uint32_t reset_attempts;
	uint32_t reset_delay;
	char *burner;
	uint8_t *burner_buf;
	uint32_t burner_len;
	bool update_high;
} options = {
		.progress = true,
		.wait = false,
		.chip = 4,
#ifndef WITHOUT_USB
		.repeat = false,
		.action = ACTION_NONE,
		.usb = NULL,
		.usb_bus = -1,
		.usb_addr = -1,
#endif
		.serial = NULL,
		.serial_baud = DEFAULT_BAUD,
		.read_chip_id = false,
		.erase_count = 0,
		.erase_all = false,
		.verify_count = 0,
		.verify_all = false,
		.probe_timeout = DEFAULT_PROBE_TIMEOUT,
		.reset_attempts = DEFAULT_RESET_ATTEMPTS,
		.reset_delay = DEFAULT_RESET_DELAY,
		.burner = NULL,
		.burner_len = 0,
		.update_high = false,
};

static cskburn_serial_nand_t nand_config = {
		.enable = false,
		.config =
				{
						.mode_4bit = 0,
						.sd_cmd = {.set = 0},
						.sd_clk = {.set = 0},
						.sd_dat0 = {.set = 0},
						.sd_dat1 = {.set = 0},
						.sd_dat2 = {.set = 0},
						.sd_dat3 = {.set = 0},
				},
};

static void
print_help(const char *progname)
{
	LOGI("Usage: %s [<options>] <addr1> <file1> [<addr2> <file2>...]", basename((char *)progname));
	LOGI("");
	LOGI("Burning options:");
#ifndef WITHOUT_USB
	LOGI("  -u, --usb (-|<bus>:<device>)\tburn with specified USB device. Pass \"-\" to select "
		 "first CSK device automatically");
#endif
	LOGI("  -s, --serial <port>\t\tburn with specified serial device (e.g. %s)",
			example_serial_dev);
	LOGI("");
	LOGI("Other options:");
	LOGI("  -h, --help\t\t\tshow help");
	LOGI("  -V, --version\t\t\tshow version");
	LOGI("  -v, --verbose\t\t\tprint verbose log");
	LOGI("  -w, --wait\t\t\twait for device presence and start burning");
	LOGI("  -b, --baud\t\t\tbaud rate used for serial burning (default: %d)", DEFAULT_BAUD);
#ifndef WITHOUT_USB
	LOGI("  -R, --repeat\t\t\trepeatly wait for device presence and start burning");
	LOGI("  -c, --check\t\t\tcheck for device presence (without burning)");
	LOGI("  -C, --chip\t\t\tchip family, acceptable values: 3/4/6 (default: %d)", DEFAULT_CHIP);
#endif
	LOGI("  -n, --nand\t\t\tburn to NAND flash (CSK6 only)");
	LOGI("");
	LOGI("Example:");
	LOGI("  cskburn -w 0x0 flashboot.bin 0x10000 master.bin 0x100000 respack.bin");
}

static void
print_version(void)
{
	LOGI("%s (%d)", GIT_TAG, GIT_INCREMENT);
}

#ifndef WITHOUT_USB
static bool usb_check(void);
static bool usb_burn(cskburn_partition_t *parts, int parts_cnt);
#endif

static bool serial_burn(cskburn_partition_t *parts, int parts_cnt);

int
main(int argc, char **argv)
{
	set_log_level(LOGLEVEL_INFO);

	int long_index = -1;
	while (1) {
		int c = getopt_long(argc, argv, option_string, long_options, &long_index);
		if (c == EOF) break;
		switch (c) {
			case 'v':
				set_log_level(LOGLEVEL_DEBUG);
				break;
			case 'w':
				options.wait = true;
				break;
			case 'R':
				options.repeat = true;
				break;
#ifndef WITHOUT_USB
			case 'u':
				options.usb = optarg;
				break;
#endif
			case 's':
				options.serial = optarg;
				break;
			case 'b':
				sscanf(optarg, "%d", &options.serial_baud);
				break;
			case 'c':
				options.action = ACTION_CHECK;
				break;
			case 'C':
				sscanf(optarg, "%d", &options.chip);
				if (options.chip != 6 && options.chip != 4 && options.chip != 3) {
					LOGE("ERROR: Only 3, 4 or 6 of chip is supported");
					return -1;
				}
				if (options.chip == 3) options.chip = 4;
				break;
			case 'n':
				nand_config.enable = true;
				break;
			case 0: { /* long-only options */
				const char *name = long_options[long_index].name;
				if (strcmp(name, "chip-id") == 0) {
					options.read_chip_id = true;
					break;
				} else if (strcmp(name, "erase") == 0) {
					if (options.erase_count >= MAX_ERASE_PARTS) {
						LOGE("ERROR: Only up to %d partitions can be erased at the same time",
								MAX_ERASE_PARTS);
						return -1;
					}

					uint16_t index = options.erase_count;

					if (!scan_addr_size(optarg, &options.erase_parts[index].addr,
								&options.erase_parts[index].size)) {
						LOGE("ERROR: Argument of --erase should be addr:size (e.g. -u "
							 "0x00000000:102400)");
						return -1;
					}

					options.erase_count++;
					break;
				} else if (strcmp(name, "erase-all") == 0) {
					options.erase_all = true;
					break;
				} else if (strcmp(name, "verify") == 0) {
					if (options.verify_count >= MAX_VERIFY_PARTS) {
						LOGE("ERROR: Only up to %d partitions can be verified at the same time",
								MAX_VERIFY_PARTS);
						return -1;
					}

					uint16_t index = options.verify_count;

					if (!scan_addr_size(optarg, &options.verify_parts[index].addr,
								&options.verify_parts[index].size)) {
						LOGE("ERROR: Argument of --verify should be addr:size (e.g. -u "
							 "0x00000000:102400)");
						return -1;
					}

					options.verify_count++;
					break;
				} else if (strcmp(name, "verify-all") == 0) {
					options.verify_all = true;
					break;
				} else if (strcmp(name, "probe-timeout") == 0) {
					sscanf(optarg, "%d", &options.probe_timeout);
					break;
				} else if (strcmp(name, "reset-attempts") == 0) {
					sscanf(optarg, "%d", &options.reset_attempts);
					break;
				} else if (strcmp(name, "reset-delay") == 0) {
					sscanf(optarg, "%d", &options.reset_delay);
					break;
				} else if (strcmp(name, "burner") == 0) {
					options.burner = optarg;
					break;
				} else if (strcmp(name, "update-high") == 0) {
					options.update_high = true;
					break;
				} else if (strcmp(name, "reset-nanokit") == 0) {
					LOGI("WARNING: --reset-nanokit is no longer needed");
					break;
				} else if (strcmp(name, "trace") == 0) {
					set_log_level(LOGLEVEL_TRACE);
					break;
				} else if (strcmp(name, "no-progress") == 0) {
					options.progress = false;
					break;
				} else if (strcmp(name, "nand-4bit") == 0) {
					nand_config.config.mode_4bit = 1;
					break;
				} else if (strcmp(name, "nand-cmd") == 0) {
					int pad, pin;
					sscanf(optarg, "%d:%d", &pad, &pin);
					nand_config.config.sd_cmd.set = 1;
					nand_config.config.sd_cmd.pad = pad;
					nand_config.config.sd_cmd.pin = pin;
					break;
				} else if (strcmp(name, "nand-clk") == 0) {
					int pad, pin;
					sscanf(optarg, "%d:%d", &pad, &pin);
					nand_config.config.sd_clk.set = 1;
					nand_config.config.sd_clk.pad = pad;
					nand_config.config.sd_clk.pin = pin;
					break;
				} else if (strcmp(name, "nand-dat0") == 0) {
					int pad, pin;
					sscanf(optarg, "%d:%d", &pad, &pin);
					nand_config.config.sd_dat0.set = 1;
					nand_config.config.sd_dat0.pad = pad;
					nand_config.config.sd_dat0.pin = pin;
					break;
				} else if (strcmp(name, "nand-dat1") == 0) {
					int pad, pin;
					sscanf(optarg, "%d:%d", &pad, &pin);
					nand_config.config.sd_dat1.set = 1;
					nand_config.config.sd_dat1.pad = pad;
					nand_config.config.sd_dat1.pin = pin;
					break;
				} else if (strcmp(name, "nand-dat2") == 0) {
					int pad, pin;
					sscanf(optarg, "%d:%d", &pad, &pin);
					nand_config.config.sd_dat2.set = 1;
					nand_config.config.sd_dat2.pad = pad;
					nand_config.config.sd_dat2.pin = pin;
					break;
				} else if (strcmp(name, "nand-dat3") == 0) {
					int pad, pin;
					sscanf(optarg, "%d:%d", &pad, &pin);
					nand_config.config.sd_dat3.set = 1;
					nand_config.config.sd_dat3.pad = pad;
					nand_config.config.sd_dat3.pin = pin;
					break;
				} else {
					print_help(argv[0]);
					return 0;
				}
			}
			case 'V':
				print_version();
				return 0;
			case 'h':
			case '?':
				print_help(argv[0]);
				return 0;
		}
	}

	if (options.burner != NULL) {
		options.burner_buf = (uint8_t *)malloc(MAX_IMAGE_SIZE);
		options.burner_len = read_file(options.burner, options.burner_buf, MAX_IMAGE_SIZE);
		if (options.burner_len == 0) {
			LOGE("ERROR: Failed reading %s: %s", options.burner, strerror(errno));
			return -1;
		}
	}

	if (options.serial != NULL && strlen(options.serial) > 0) {
		options.protocol = PROTO_SERIAL;
	} else {
#ifdef WITHOUT_USB
		LOGE("ERROR: A port of serial device should be specified (e.g. -s %s)", example_serial_dev);
		return -1;
#else
		if (options.chip == 6) {
			LOGE("ERROR: USB burning is not supported by chip family 6");
			return -1;
		}
		if (options.usb != NULL && strcmp(options.usb, "-") != 0) {
			if (sscanf(options.usb, "%hu:%hu\n", &options.usb_bus, &options.usb_addr) != 2) {
				LOGE("ERROR: Argument of -u/--usb should be <bus>:<device> (e.g. -u 020:004)");
				return -1;
			}
		}
		options.protocol = PROTO_USB;
#endif
	}

	if (nand_config.enable) {
#ifndef WITHOUT_USB
		if (options.protocol != PROTO_SERIAL) {
			LOGE("ERROR: NAND is supported only in serial burning");
			return -1;
		}
#endif
		if (options.chip != 6) {
			LOGE("ERROR: NAND is only supported by chip family 6");
			return -1;
		}
		if (options.erase_all || options.erase_count > 0) {
			LOGE("ERROR: Erasing is not supported on NAND yet");
			return -1;
		}
	}

	if (options.action == ACTION_CHECK) {
#ifndef WITHOUT_USB
		if (options.protocol == PROTO_USB) {
			if (usb_check()) {
				return 0;
			} else {
				return -1;
			}
		}
#endif
	}

	int ret = 0;

	uint32_t base_addr = options.chip == 6 ? 0x18000000 : 0x80000000;

	cskburn_partition_t parts[MAX_FLASH_PARTS];
	int parts_cnt = 0;

	char **parts_argv = argv + optind;
	int parts_argc = argc - optind;
	memset(parts, 0, sizeof(parts));
	if (!read_parts_bin(parts_argv, parts_argc, parts + parts_cnt, &parts_cnt,
				MAX_FLASH_PARTS - parts_cnt)) {
		ret = -1;
		goto exit;
	}
	if (!read_parts_hex(parts_argv, parts_argc, parts + parts_cnt, &parts_cnt, MAX_IMAGE_SIZE,
				MAX_FLASH_PARTS - parts_cnt, base_addr)) {
		ret = -1;
		goto exit;
	}

	for (int i = 0; i < parts_cnt; i++) {
		if (parts[i].path == NULL) {
			LOGI("Partition %d: 0x%08X (%.2f KB)", i + 1, parts[i].addr,
					(float)parts[i].reader->size / 1024.0f);
		} else {
			LOGI("Partition %d: 0x%08X (%.2f KB) - %s", i + 1, parts[i].addr,
					(float)parts[i].reader->size / 1024.0f, parts[i].path);
		}
	}

	if (options.protocol == PROTO_SERIAL) {
		if (!serial_burn(parts, parts_cnt)) {
			ret = -1;
			goto exit;
		}
#ifndef WITHOUT_USB
	} else if (options.protocol == PROTO_USB) {
		if (options.repeat) {
			while (1) {
				usb_burn(parts, parts_cnt);
				LOGI("----------");
				msleep(2000);
			}
		} else {
			if (!usb_burn(parts, parts_cnt)) {
				ret = -1;
				goto exit;
			}
		}
#endif
	}

exit:
	for (int i = 0; i < parts_cnt; i++) {
		parts[i].reader->close(&parts[i].reader);
	}
	return ret;
}

static void
print_progress(int32_t wrote_bytes, uint32_t total_bytes)
{
	if (wrote_bytes < 0) {
		printf("Erasing");
		for (int i = 0; i <= -wrote_bytes; i++) {
			printf(".");
		}
		printf("  \r");
		fflush(stdout);
	} else if (wrote_bytes % (32 * 1024) == 0 || wrote_bytes == total_bytes) {
		printf("%.2f KB / %.2f KB (%.2f%%)  \r", (float)wrote_bytes / 1024.0f,
				(float)total_bytes / 1024.0f, (float)wrote_bytes / (float)total_bytes * 100.0f);
		if (wrote_bytes == total_bytes) {
			printf("\n");
		}
		fflush(stdout);
	}
}

#ifndef WITHOUT_USB
static bool
usb_check(void)
{
	bool ret = false;

	if (!cskburn_usb_init()) {
		LOGE("ERROR: USB initialize failed");
		goto exit;
	}

	do {
		if (cskburn_usb_wait(options.usb_bus, options.usb_addr, 10)) {
			ret = true;
			goto exit;
		}
	} while (options.wait);

exit:
	cskburn_usb_exit();
	return ret;
}

static bool
usb_burn(cskburn_partition_t *parts, int parts_cnt)
{
	bool ret = false;

	if (!cskburn_usb_init()) {
		LOGE("ERROR: USB initialize failed");
		goto err_init;
	}

	if (options.wait) {
		int w = 0;
		while (!cskburn_usb_wait(options.usb_bus, options.usb_addr, 10)) {
			if (w == 0) {
				w = 1;
			} else if (w == 1) {
				LOGI("Waiting for device...");
				w = 2;
			}
		}
	}

	LOGI("Entering update mode...");
	cskburn_usb_device_t *dev;
	for (int i = 0; i < ENTER_TRIES; i++) {
		if ((dev = cskburn_usb_open(options.usb_bus, options.usb_addr)) == NULL) {
			if (i == ENTER_TRIES - 1) {
				LOGE("ERROR: Failed opening device");
				goto err_open;
			} else {
				msleep(2000);
				continue;
			}
		}
		msleep(500);
		if (!cskburn_usb_enter(dev, options.burner_buf, options.burner_len)) {
			if (i == ENTER_TRIES - 1) {
				LOGE("ERROR: Failed entering update mode");
				goto err_enter;
			} else {
				cskburn_usb_close(&dev);
				msleep(2000);
				continue;
			}
		}
		break;
	}

	for (int i = 0; i < parts_cnt; i++) {
		LOGI("Burning partition %d/%d... (0x%08X, %.2f KB)", i + 1, parts_cnt, parts[i].addr,
				(float)parts[i].reader->size / 1024.0f);
		if (!cskburn_usb_write(dev, parts[i].addr, parts[i].reader,
					options.progress ? print_progress : NULL)) {
			LOGE("ERROR: Failed burning partition %d", i + 1);
			goto err_write;
		}
	}

	cskburn_usb_show_done(dev);

	LOGI("Finished");
	ret = true;

err_write:
err_enter:
	cskburn_usb_close(&dev);
err_open:
err_init:
	cskburn_usb_exit();
	return ret;
}
#endif

static bool
serial_connect(cskburn_serial_device_t *dev)
{
	for (int i = 0; options.wait || i < options.reset_attempts + 1; i++) {
		uint32_t reset_delay = i == 0 ? 0 : options.reset_delay;
		uint32_t probe_timeout = i == 0 ? 100 : options.probe_timeout;
		if (!cskburn_serial_connect(dev, reset_delay, probe_timeout)) {
			if (i == 0) {
				LOGI("Waiting for device...");
			}
			if (!options.wait && i == options.reset_attempts) {
				LOGE("ERROR: Failed opening device");
				return false;
			} else {
				continue;
			}
		}
		LOGI("Entering update mode...");
		if (!cskburn_serial_enter(
					dev, options.serial_baud, options.burner_buf, options.burner_len)) {
			if (!options.wait && i == options.reset_attempts) {
				LOGE("ERROR: Failed entering update mode");
				return false;
			} else {
				msleep(2000);
				continue;
			}
		}
		break;
	}

	return true;
}

static bool
serial_burn(cskburn_partition_t *parts, int parts_cnt)
{
	bool ret = false;

	int flags = 0;
	if (options.update_high) flags |= FLAG_INVERT_RTS;
	cskburn_serial_init(flags);

	if (nand_config.enable) {
		LOGD("Using NAND flash");
		LOGD("* 4-bit mode: %d", nand_config.config.mode_4bit);
		LOGD("* pin sd_cmd : set=%d, pad=%d, pin=%d", nand_config.config.sd_cmd.set,
				nand_config.config.sd_cmd.pad, nand_config.config.sd_cmd.pin);
		LOGD("* pin sd_clk : set=%d, pad=%d, pin=%d", nand_config.config.sd_clk.set,
				nand_config.config.sd_clk.pad, nand_config.config.sd_clk.pin);
		LOGD("* pin sd_dat0: set=%d, pad=%d, pin=%d", nand_config.config.sd_dat0.set,
				nand_config.config.sd_dat0.pad, nand_config.config.sd_dat0.pin);
		LOGD("* pin sd_dat1: set=%d, pad=%d, pin=%d", nand_config.config.sd_dat1.set,
				nand_config.config.sd_dat1.pad, nand_config.config.sd_dat1.pin);
		LOGD("* pin sd_dat2: set=%d, pad=%d, pin=%d", nand_config.config.sd_dat2.set,
				nand_config.config.sd_dat2.pad, nand_config.config.sd_dat2.pin);
		LOGD("* pin sd_dat3: set=%d, pad=%d, pin=%d", nand_config.config.sd_dat3.set,
				nand_config.config.sd_dat3.pad, nand_config.config.sd_dat3.pin);
	}

	cskburn_serial_device_t *dev = cskburn_serial_open(options.serial, options.chip, &nand_config);
	if (dev == NULL) {
		LOGE("ERROR: Failed opening device");
		goto err_open;
	}

	if (!serial_connect(dev)) {
		goto err_enter;
	}

	if (options.read_chip_id) {
		uint8_t id[CHIP_ID_LEN] = {0};
		if (!cskburn_serial_read_chip_id(dev, id)) {
			LOGE("ERROR: Failed reading device");
			goto err_enter;
		}

		LOGI("chip-id: %02X%02X%02X%02X%02X%02X%02X%02X", id[0], id[1], id[2], id[3], id[4], id[5],
				id[6], id[7]);
	}

	uint32_t flash_id = 0;
	uint64_t flash_size = 0;
	if (!cskburn_serial_get_flash_info(dev, &flash_id, &flash_size)) {
		LOGE("ERROR: Failed detecting flash type");
		goto err_enter;
	}

	if (flash_id != 0) {
		LOGD("flash-id: %02X%02X%02X", (flash_id)&0xFF, (flash_id >> 8) & 0xFF,
				(flash_id >> 16) & 0xFF);
	}

	if (nand_config.enable) {
		LOGI("Detected NAND size: %llu MB", flash_size >> 20);
	} else {
		LOGI("Detected flash size: %llu MB", flash_size >> 20);
	}

	for (int i = 0; i < options.erase_count; i++) {
		if (!is_aligned(options.erase_parts[i].addr, 4 * 1024)) {
			LOGE("ERROR: Erase address (0x%08X) should be 4K aligned", options.erase_parts[i].addr);
			goto err_enter;
		} else if (!is_aligned(options.erase_parts[i].size, 4 * 1024)) {
			LOGE("ERROR: Erase size (0x%08X) should be 4K aligned", options.erase_parts[i].size);
			goto err_enter;
		} else if (options.erase_parts[i].addr >= flash_size) {
			LOGE("ERROR: The starting boundary of erase address (0x%08X) exceeds the capacity of "
				 "flash (%llu MB)",
					options.erase_parts[i].addr, flash_size >> 20);
			goto err_enter;
		} else if (options.erase_parts[i].addr + options.erase_parts[i].size > flash_size) {
			LOGE("ERROR: The ending boundary of erase address (0x%08X) exceeds the capacity of "
				 "flash (%llu MB)",
					options.erase_parts[i].addr + options.erase_parts[i].size, flash_size >> 20);
			goto err_enter;
		}
	}

	for (int i = 0; i < parts_cnt; i++) {
		if (nand_config.enable) {
			if (!is_aligned(parts[i].addr, 512)) {
				LOGE("ERROR: Address of partition %d (0x%08X) should be 512 bytes aligned", i + 1,
						parts[i].addr);
				goto err_enter;
			}
		} else {
			if (!is_aligned(parts[i].addr, 4 * 1024)) {
				LOGE("ERROR: Address of partition %d (0x%08X) should be 4K aligned", i + 1,
						parts[i].addr);
				goto err_enter;
			}
		}
		if (parts[i].addr >= flash_size) {
			LOGE("ERROR: The starting boundary of partition %d (0x%08X) exceeds the capacity of "
				 "flash (%llu MB)",
					i + 1, parts[i].addr, flash_size >> 20);
			goto err_enter;
		} else if (parts[i].addr + parts[i].reader->size > flash_size) {
			LOGE("ERROR: The ending boundary of partition %d (0x%08X) exceeds the capacity of "
				 "flash (%llu MB)",
					i + 1, parts[i].addr + parts[i].reader->size, flash_size >> 20);
			goto err_enter;
		}

		if (options.verify_all) {
			verify_install(parts[i].reader);
		}
	}

	if (options.erase_all) {
		LOGI("Erasing entire flash...");
		if (!cskburn_serial_erase_all(dev)) {
			LOGE("ERROR: Failed erasing device");
			goto err_enter;
		}
	} else {
		for (int i = 0; i < options.erase_count; i++) {
			uint32_t addr = options.erase_parts[i].addr;
			uint32_t size = options.erase_parts[i].size;
			LOGI("Erasing region 0x%08X-0x%08X...", addr, addr + size);
			if (!cskburn_serial_erase(dev, addr, size)) {
				LOGE("ERROR: Failed erasing device");
				goto err_enter;
			}
		}
	}

	if (options.verify_count > 0) {
		uint8_t md5[MD5_SIZE] = {0};
		char md5_str[MD5_SIZE * 2 + 1] = {0};
		for (int i = 0; i < options.verify_count; i++) {
			uint32_t addr = options.verify_parts[i].addr;
			uint32_t size = options.verify_parts[i].size;
			if (!cskburn_serial_verify(dev, addr, size, md5)) {
				LOGE("ERROR: Failed reading device");
				goto err_enter;
			}
			md5_to_str(md5_str, md5);
			LOGI("md5 (0x%08X-0x%08X): %s", addr, addr + size, md5_str);
		}
	}

	for (int i = 0; i < parts_cnt; i++) {
		LOGI("Burning partition %d/%d... (0x%08X, %.2f KB)", i + 1, parts_cnt, parts[i].addr,
				(float)parts[i].reader->size / 1024.0f);
		if (!cskburn_serial_write(dev, parts[i].addr, parts[i].reader,
					options.progress ? print_progress : NULL)) {
			LOGE("ERROR: Failed burning partition %d", i + 1);
			goto err_write;
		}

		if (options.verify_all) {
			uint8_t image_md5[MD5_SIZE] = {0};
			uint8_t flash_md5[MD5_SIZE] = {0};
			char md5_str[MD5_SIZE * 2 + 1] = {0};
			if (verify_finish(parts[i].reader, image_md5) != 0) {
				LOGE("ERROR: Failed calculating MD5");
				goto err_write;
			}
			if (!cskburn_serial_verify(dev, parts[i].addr, parts[i].reader->size, flash_md5)) {
				LOGE("ERROR: Failed reading device");
				goto err_write;
			}
			md5_to_str(md5_str, flash_md5);
			LOGI("md5 (0x%08X-0x%08X): %s", parts[i].addr, parts[i].addr + parts[i].reader->size,
					md5_str);
			if (memcmp(image_md5, flash_md5, MD5_SIZE) != 0) {
				LOGE("ERROR: MD5 mismatch");
				goto err_write;
			}
		}
	}

	LOGI("Finished");
	ret = true;

err_write:
err_enter:
	cskburn_serial_reset(dev, options.reset_delay);
	cskburn_serial_close(&dev);
err_open:
	return ret;
}
