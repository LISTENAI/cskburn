#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#include <msleep.h>
#include <exists.h>
#ifndef WITHOUT_USB
#include <cskburn_usb.h>
#endif
#include <cskburn_serial.h>

#define MAX_IMAGE_SIZE (20 * 1024 * 1024)
#define ENTER_TRIES 5

static struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'V'},
		{"verbose", no_argument, NULL, 'v'},
		{"wait", no_argument, NULL, 'w'},
#ifndef WITHOUT_USB
		{"usb", required_argument, NULL, 'u'},
#endif
		{"serial", required_argument, NULL, 's'},
		{"baud", required_argument, NULL, 'b'},
#ifndef WITHOUT_USB
		{"repeat", no_argument, NULL, 'R'},
		{"check", no_argument, NULL, 'c'},
#endif
		{0, 0, NULL, 0},
};

static const char option_string[] = {
		"hVv"
		"w"
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

static struct {
	bool verbose;
	bool wait;
	bool repeat;
	bool check;
#ifndef WITHOUT_USB
	char *usb;
	int16_t usb_bus;
	int16_t usb_addr;
#endif
	char *serial;
	uint32_t serial_baud;
} options = {
		.verbose = false,
		.wait = false,
#ifndef WITHOUT_USB
		.repeat = false,
		.check = false,
		.usb = NULL,
		.usb_bus = -1,
		.usb_addr = -1,
#endif
		.serial = NULL,
		.serial_baud = 115200,
};

static void
print_help(const char *progname)
{
	printf("用法: %s [<选项>] <地址1> <文件1> [<地址2> <文件2>...]\n", progname);
	printf("\n");
	printf("烧录选项:\n");
#ifndef WITHOUT_USB
	printf("  -u, --usb (-|<总线>:<设备>)\t\t使用指定 USB 设备烧录。传 - 表示自动选取第一个 CSK "
		   "设备\n");
#endif
	printf("  -s, --serial <端口>\t\t\t使用指定串口设备 (如 %s) 烧录\n", example_serial_dev);
	printf("\n");
	printf("其它选项:\n");
	printf("  -h, --help\t\t\t\t显示帮助\n");
	printf("  -V, --version\t\t\t\t显示版本号\n");
	printf("  -v, --verbose\t\t\t\t显示详细日志\n");
	printf("  -w, --wait\t\t\t\t等待设备插入，并自动开始烧录\n");
#ifndef WITHOUT_USB
	printf("  -R, --repeat\t\t\t\t循环等待设备插入，并自动开始烧录\n");
	printf("  -c, --check\t\t\t\t检查设备是否插入 (不进行烧录)\n");
#endif
	printf("\n");
	printf("用例:\n");
	printf("  cskburn -w 0x0 flashboot.bin 0x10000 master.bin 0x100000 respack.bin\n");
}

static void
print_version(void)
{
	printf("%s (%d)\n", GIT_TAG, GIT_INCREMENT);
}

#ifndef WITHOUT_USB
static bool check_usb();
static bool burn_usb(uint32_t *addrs, char **images, int parts);
#endif

static bool burn_serial(uint32_t *addrs, char **images, int parts);

int
main(int argc, char **argv)
{
	while (1) {
		int c = getopt_long(argc, argv, option_string, long_options, NULL);
		if (c == EOF) break;
		switch (c) {
			case 'v':
				options.verbose = true;
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
				options.check = true;
				break;
			case 'V':
				print_version();
				return 0;
			case 'h':
			case '?':
				print_help(argv[0]);
				return 0;
		}
	}

#ifdef WITHOUT_USB
	if (options.serial == NULL || strlen(options.serial) == 0) {
		printf("错误: 必须指定一个串口设备 (如: -s %s)\n", example_serial_dev);
		return -1;
	}
#else
	if (options.usb != NULL && strcmp(options.usb, "-") != 0) {
		if (sscanf(options.usb, "%hu:%hu\n", &options.usb_bus, &options.usb_addr) != 2) {
			printf("错误: -u/--usb 参数的格式应为 <总线>:<设备> (如: -u 020:004)\n");
			return -1;
		}
	}

	if (options.check) {
		if (check_usb()) {
			return 0;
		} else {
			return -1;
		}
	}
#endif

	uint32_t addrs[20];
	char *images[20];
	int i = optind, j = 0;
	while (i < argc - 1) {
		if (sscanf(argv[i], "0x%x", &addrs[j]) != 1 && sscanf(argv[i], "%d", &addrs[j]) != 1) {
			i++;
			continue;
		}

		images[j] = argv[i + 1];

		if (!exists(images[j])) {
			printf("错误: 分区 %d 的文件不存在: %s\n", j + 1, images[j]);
			return -1;
		}

		printf("分区 %d: 0x%08X %s\n", j + 1, addrs[j], images[j]);

		i += 2;
		j += 1;
	}

	if (j < 1) {
		printf("错误: 你必须指定至少一个烧录地址及文件\n");
		return -1;
	}

#ifdef WITHOUT_USB
	if (!burn_serial(addrs, images, j)) {
		return -1;
	}
#else
	if (options.repeat) {
		while (1) {
			burn_usb(addrs, images, j);
			printf("----------\n");
			msleep(2000);
		}
	} else {
		if (options.serial != NULL) {
			if (!burn_serial(addrs, images, j)) {
				return -1;
			}
		} else {
			if (!burn_usb(addrs, images, j)) {
				return -1;
			}
		}
	}
#endif

	return 0;
}

static uint32_t
read_file(const char *path, uint8_t *buf, uint32_t limit)
{
	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		printf("错误: 文件打开失败: %s\n", strerror(errno));
		return 0;
	}

	size_t len = fread(buf, 1, limit, f);
	fclose(f);
	return (uint32_t)len;
}

static void
print_progress(int32_t wrote_bytes, uint32_t total_bytes)
{
	if (wrote_bytes < 0) {
		printf("正在擦除");
		for (int i = 0; i <= -wrote_bytes; i++) {
			printf(".");
		}
		printf("  \r");
	} else {
		printf("%.2f KB / %.2f KB (%.2f%%)  \r", (float)wrote_bytes / 1024.0f,
				(float)total_bytes / 1024.0f, (float)wrote_bytes / (float)total_bytes * 100.0f);
		if (wrote_bytes == total_bytes) {
			printf("\n");
		}
	}
	fflush(stdout);
}

#ifndef WITHOUT_USB
static bool
check_usb()
{
	bool ret = false;

	if (!cskburn_usb_init(options.verbose)) {
		printf("错误: 初始化失败\n");
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
burn_usb(uint32_t *addrs, char **images, int parts)
{
	if (!cskburn_usb_init(options.verbose)) {
		printf("错误: 初始化失败\n");
		goto err_init;
	}

	if (options.wait) {
		int w = 0;
		while (!cskburn_usb_wait(options.usb_bus, options.usb_addr, 10)) {
			if (w == 0) {
				w = 1;
			} else if (w == 1) {
				printf("正在等待设备接入…\n");
				w = 2;
			}
		}
	}

	printf("正在进入烧录模式…\n");
	cskburn_usb_device_t *dev;
	for (int i = 0; i < ENTER_TRIES; i++) {
		if ((dev = cskburn_usb_open(options.usb_bus, options.usb_addr)) == NULL) {
			if (i == ENTER_TRIES - 1) {
				printf("错误: 设备打开失败\n");
				goto err_open;
			} else {
				msleep(2000);
				continue;
			}
		}
		msleep(500);
		if (!cskburn_usb_enter(dev)) {
			if (i == ENTER_TRIES - 1) {
				printf("错误: 无法进入烧录模式\n");
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
			printf("错误: 无法读取 %s\n", images[i]);
			goto err_enter;
		}

		printf("正在烧录分区 %d/%d… (0x%08X, %.2f KB)\n", i + 1, parts, addrs[i],
				(float)image_len / 1024.0f);
		if (!cskburn_usb_write(dev, addrs[i], image_buf, image_len, print_progress)) {
			printf("错误: 无法烧录分区 %d\n", i + 1);
			goto err_write;
		}
	}

	free(image_buf);
	printf("烧录完成\n");

	return true;

err_write:
	free(image_buf);
err_enter:
	cskburn_usb_close(&dev);
err_open:
err_init:
	cskburn_usb_exit();

	return false;
}
#endif

static bool
burn_serial(uint32_t *addrs, char **images, int parts)
{
	cskburn_serial_init(options.verbose);

	cskburn_serial_device_t *dev = cskburn_serial_open(options.serial);
	if (dev == NULL) {
		printf("错误: 设备打开失败\n");
		goto err_open;
	}

	for (int i = 0; options.wait || i < ENTER_TRIES; i++) {
		if (!cskburn_serial_connect(dev)) {
			if (i == 0) {
				printf("正在等待设备接入…\n");
			}
			if (!options.wait && i == ENTER_TRIES - 1) {
				printf("错误: 设备打开失败\n");
				goto err_enter;
			} else {
				continue;
			}
		}
		printf("正在进入烧录模式…\n");
		if (!cskburn_serial_enter(dev, options.serial_baud)) {
			if (!options.wait && i == ENTER_TRIES - 1) {
				printf("错误: 无法进入烧录模式\n");
				goto err_enter;
			} else {
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
			printf("错误: 无法读取 %s\n", images[i]);
			goto err_enter;
		}

		printf("正在烧录分区 %d/%d… (0x%08X, %.2f KB)\n", i + 1, parts, addrs[i],
				(float)image_len / 1024.0f);
		if (!cskburn_serial_write(dev, addrs[i], image_buf, image_len, print_progress)) {
			printf("错误: 无法烧录分区 %d\n", i + 1);
			goto err_write;
		}
	}

	free(image_buf);
	printf("烧录完成\n");

	return true;

err_write:
	free(image_buf);
err_enter:
	cskburn_serial_close(&dev);
err_open:
	return false;
}
