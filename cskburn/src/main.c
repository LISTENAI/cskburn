#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "exists.h"
#include "log.h"
#include "msleep.h"
#include "utils.h"
#ifndef WITHOUT_USB
#include "cskburn_usb.h"
#endif
#include "cskburn_serial.h"
#include "mbedtls/md5.h"

#define MAX_IMAGE_SIZE (32 * 1024 * 1024)
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
		{"chip-id", no_argument, NULL, 0},
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
#ifndef WITHOUT_USB
		"u:"
		"R"
		"c"
#endif
		"s:b:",
};

static const char example_serial_dev[] =
#if defined(_WIN32) || defined(_WIN64)
		"\\\\.\\COM24";
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
		.verify_count = 0,
		.verify_all = false,
		.probe_timeout = DEFAULT_PROBE_TIMEOUT,
		.reset_attempts = DEFAULT_RESET_ATTEMPTS,
		.reset_delay = DEFAULT_RESET_DELAY,
		.burner = NULL,
		.burner_len = 0,
		.update_high = false,
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
static bool usb_burn(uint32_t *addrs, char **images, int parts);
#endif

static bool serial_burn(uint32_t *addrs, char **images, int parts);

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
			case 0: { /* long-only options */
				const char *name = long_options[long_index].name;
				if (strcmp(name, "chip-id") == 0) {
					options.read_chip_id = true;
					break;
				} else if (strcmp(name, "verify") == 0) {
					if (options.verify_count >= MAX_VERIFY_PARTS) {
						LOGE("ERROR: Only up to %d partitions can be verified at the same time",
								MAX_VERIFY_PARTS);
						return -1;
					}

					char *split = strstr(optarg, ":");
					if (split == NULL) {
						LOGE("ERROR: Argument of --verify should be addr:size (e.g. -u "
							 "0x00000000:102400)");
						return -1;
					}

					char *addr = optarg;
					char *size = split + 1;
					split[0] = 0;

					uint16_t index = options.verify_count;

					if (!scan_int(addr, &options.verify_parts[index].addr)) {
						LOGE("ERROR: Argument of --verify should be addr:size (e.g. -u "
							 "0x00000000:102400)");
						return -1;
					}

					if (!scan_int(size, &options.verify_parts[index].size)) {
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
			LOGE("ERROR: USB burning is not support by chip family 6");
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

	uint32_t addrs[20];
	char *images[20];
	int i = optind, j = 0;
	while (i < argc - 1) {
		if (!scan_int(argv[i], &addrs[j])) {
			i++;
			continue;
		}

		images[j] = argv[i + 1];

		if (!exists(images[j])) {
			LOGE("ERROR: File for partition %d not found: %s", j + 1, images[j]);
			return -1;
		}

		LOGI("Partition %d: 0x%08X %s", j + 1, addrs[j], images[j]);

		i += 2;
		j += 1;
	}

	if (options.protocol == PROTO_SERIAL) {
		if (!serial_burn(addrs, images, j)) {
			return -1;
		}
#ifndef WITHOUT_USB
	} else if (options.protocol == PROTO_USB) {
		if (options.repeat) {
			while (1) {
				usb_burn(addrs, images, j);
				LOGI("----------");
				msleep(2000);
			}
		} else {
			if (!usb_burn(addrs, images, j)) {
				return -1;
			}
		}
#endif
	}

	return 0;
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
usb_burn(uint32_t *addrs, char **images, int parts)
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

	uint8_t *image_buf = malloc(MAX_IMAGE_SIZE);
	uint32_t image_len;
	for (int i = 0; i < parts; i++) {
		image_len = read_file(images[i], image_buf, MAX_IMAGE_SIZE);
		if (image_len == 0) {
			LOGE("ERROR: Failed reading %s: %s", images[i], strerror(errno));
			goto err_enter;
		}

		LOGI("Burning partition %d/%d... (0x%08X, %.2f KB)", i + 1, parts, addrs[i],
				(float)image_len / 1024.0f);
		if (!cskburn_usb_write(dev, addrs[i], image_buf, image_len,
					options.progress ? print_progress : NULL)) {
			LOGE("ERROR: Failed burning partition %d", i + 1);
			goto err_write;
		}
	}

	cskburn_usb_show_done(dev);

	LOGI("Finished");
	ret = true;

err_write:
	free(image_buf);
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
serial_burn(uint32_t *addrs, char **images, int parts)
{
	bool ret = false;

	int flags = 0;
	if (options.update_high) flags |= FLAG_INVERT_RTS;
	cskburn_serial_init(flags);

	cskburn_serial_device_t *dev = cskburn_serial_open(options.serial, options.chip);
	if (dev == NULL) {
		LOGE("ERROR: Failed opening device");
		goto err_open;
	}

	if (!serial_connect(dev)) {
		goto err_enter;
	}

	if (options.read_chip_id) {
		uint64_t chip_id = 0;
		if (!cskburn_serial_read_chip_id(dev, &chip_id)) {
			LOGE("ERROR: Failed reading device");
			goto err_enter;
		}

		LOGI("chip-id: %016llX", chip_id);
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

	uint8_t *image_buf = malloc(MAX_IMAGE_SIZE);
	uint32_t image_len;
	for (int i = 0; i < parts; i++) {
		image_len = read_file(images[i], image_buf, MAX_IMAGE_SIZE);
		if (image_len == 0) {
			LOGE("ERROR: Failed reading %s: %s", images[i], strerror(errno));
			goto err_write;
		}

		LOGI("Burning partition %d/%d... (0x%08X, %.2f KB)", i + 1, parts, addrs[i],
				(float)image_len / 1024.0f);
		if (!cskburn_serial_write(dev, addrs[i], image_buf, image_len,
					options.progress ? print_progress : NULL)) {
			LOGE("ERROR: Failed burning partition %d", i + 1);
			goto err_write;
		}

		if (options.verify_all) {
			uint8_t image_md5[MD5_SIZE] = {0};
			uint8_t flash_md5[MD5_SIZE] = {0};
			char md5_str[MD5_SIZE * 2 + 1] = {0};
			if (mbedtls_md5(image_buf, image_len, image_md5) != 0) {
				LOGE("ERROR: Failed calculating MD5");
				goto err_write;
			}
			if (!cskburn_serial_verify(dev, addrs[i], image_len, flash_md5)) {
				LOGE("ERROR: Failed reading device");
				goto err_write;
			}
			md5_to_str(md5_str, flash_md5);
			LOGI("md5 (0x%08X-0x%08X): %s", addrs[i], addrs[i] + image_len, md5_str);
			if (memcmp(image_md5, flash_md5, MD5_SIZE) != 0) {
				LOGE("ERROR: MD5 mismatch");
				goto err_write;
			}
		}
	}

	LOGI("Finished");
	ret = true;

err_write:
	free(image_buf);
err_enter:
	cskburn_serial_reset(dev, options.reset_delay);
	cskburn_serial_close(&dev);
err_open:
	return ret;
}
