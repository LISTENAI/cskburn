#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#include <msleep.h>
#include <exists.h>
#include <cskburn_usb.h>

#define MAX_IMAGE_SIZE (20 * 1024 * 1024)

static struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'V'},
		{"wait", no_argument, NULL, 'w'},
		{"usb", required_argument, NULL, 'u'},
		{"repeat", no_argument, NULL, 'R'},
		{"check", no_argument, NULL, 'c'},
		{0, 0, NULL, 0},
};

static struct {
	bool wait;
	bool repeat;
	bool check;
	char *usb;
	int16_t usb_bus;
	int16_t usb_addr;
} options = {
		.wait = false,
		.repeat = false,
		.check = false,
		.usb = NULL,
		.usb_bus = -1,
		.usb_addr = -1,
};

static void
print_help(const char *progname)
{
	printf("用法: %s [<选项>] <地址1> <文件1> [<地址2> <文件2>...]\n", progname);
	printf("\n");
	printf("选项:\n");
	printf("  -h, --help\t\t\t\t显示帮助\n");
	printf("  -V, --version\t\t\t\t显示版本号\n");
	printf("  -w, --wait\t\t\t\t等待设备插入，并自动开始烧录\n");
	printf("  -R, --repeat\t\t\t\t循环等待设备插入，并自动开始烧录\n");
	printf("  -c, --check\t\t\t\t检查设备是否插入 (不进行烧录)\n");
	printf("\n");
	printf("用例:\n");
	printf("  cskburn -w 0x0 flashboot.bin 0x10000 master.bin 0x100000 respack.bin\n");
}

static void
print_version(void)
{
	printf("%s (%d)\n", GIT_TAG, GIT_INCREMENT);
}

static bool check_usb();
static bool burn_usb(uint32_t *addrs, char **images, int parts);

int
main(int argc, char **argv)
{
	while (1) {
		int c = getopt_long(argc, argv, "hVvwRu:r:c", long_options, NULL);
		if (c == EOF) break;
		switch (c) {
			case 'w':
				options.wait = true;
				break;
			case 'R':
				options.repeat = true;
				break;
			case 'u':
				options.usb = optarg;
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

	if (options.repeat) {
		while (1) {
			burn_usb(addrs, images, j);
			printf("----------\n");
			msleep(2000);
		}
	} else {
		if (!burn_usb(addrs, images, j)) {
			return -1;
		}
	}

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

static bool
check_usb()
{
	bool ret = false;

	if (cskburn_usb_init() != 0) {
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
	if (cskburn_usb_init() != 0) {
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

	cskburn_usb_device_t *dev = cskburn_usb_open(options.usb_bus, options.usb_addr);
	if (dev == NULL) {
		printf("错误: 设备打开失败\n");
		goto err_open;
	}

	printf("正在进入烧录模式…\n");
	msleep(2000);
	if (cskburn_usb_enter(dev)) {
		printf("错误: 无法进入烧录模式\n");
		goto err_enter;
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
		if (cskburn_usb_write(dev, addrs[i], image_buf, image_len, print_progress)) {
			printf("错误: 无法烧录分区 %d\n", i + 1);
			goto err_write;
		}
	}

	cskburn_usb_finish(dev);

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
