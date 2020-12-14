#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#include <cskburn_usb.h>

#define MAX_IMAGE_SIZE (20 * 1024 * 1024)

static struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'V'},
		{"wait", no_argument, NULL, 'w'},
		{"usb", required_argument, NULL, 'u'},
		{"serial", required_argument, NULL, 's'},
		{0, 0, NULL, 0},
};

static void
print_help(void)
{
	printf("用法: cskburn [<选项>] <burner> <地址1> <文件1> [<地址2> <文件2>...]\n");
	printf("\n");
	printf("选项:\n");
	printf("  -h, --help\t\t\t\t显示帮助\n");
	printf("  -V, --version\t\t\t\t显示版本号\n");
	printf("  -w, --wait\t\t\t\t等待设备插入\n");
	printf("  -u, --usb (-|<端口>|<总线>:<设备>)\t使用指定 USB 端口烧录\n");
	printf("  -s, --serial <端口>\t\t\t使用指定串口端口烧录\n");
	printf("\n");
	printf("用例:\n");
	printf("  cskburn -w -u 128:000 burner.img 0x0 flashboot.bin 0x10000 master.bin 0x100000 "
		   "respack.bin\n");
	printf("  cskburn -w -s /dev/tty.usbserial-142120 burner.img 0x0 flashboot.bin 0x10000 "
		   "master.bin 0x100000 respack.bin\n");
}

static void
print_version(void)
{
	printf("0.0.0\n");
}

static void burn_usb(
		char *device, bool wait, char *burner, uint32_t *addrs, char **images, int parts);

int
main(int argc, char **argv)
{
	bool wait;
	char *usb;
	char *serial;

	while (1) {
		int c = getopt_long(argc, argv, "hVwu:s:", long_options, NULL);
		if (c == EOF) break;
		switch (c) {
			case 'w':
				wait = true;
				break;
			case 'u':
				usb = optarg;
				break;
			case 's':
				serial = optarg;
				break;
			case 'V':
				print_version();
				return 0;
			case 'h':
			case '?':
				print_help();
				return 0;
		}
	}

	if (argc - optind < 3) {
		printf("错误: 你必须指定 burner 及至少一个烧录地址及文件\n");
		return 0;
	}

	if ((argc - optind) % 2 != 1) {
		printf("错误: 烧录地址及文件必须成对出现\n");
		return 0;
	}

	char *burner = argv[optind];
	if (access(burner, F_OK) != 0) {
		printf("错误: burner 不存在: %s\n", burner);
		return 0;
	}

	printf("burner: %s\n", burner);

	char **parts = &argv[optind + 1];
	int part_count = (argc - optind - 1) / 2;

	uint32_t addrs[20];
	char *images[20];
	char *address;
	for (int i = 0; i < part_count; i++) {
		address = parts[i * 2];
		images[i] = parts[i * 2 + 1];

		if (sscanf(address, "0x%x", &addrs[i]) != 1 && sscanf(address, "%d", &addrs[i]) != 1) {
			printf("错误: 分区 %d 的地址格式不合法: %s\n", i + 1, address);
			return 0;
		}

		if (access(images[i], F_OK) != 0) {
			printf("错误: 分区 %d 的文件不存在: %s\n", i + 1, images[i]);
			return 0;
		}

		printf("分区 %d: 0x%08X %s\n", i + 1, addrs[i], images[i]);
	}

	if (usb != NULL) {
		burn_usb(usb, wait, burner, addrs, images, part_count);
	} else if (serial != NULL) {
		printf("serial: %lu\n", strlen(serial));
	}

	return 0;
}

static uint32_t
read_file(const char *path, uint8_t *buf, uint32_t limit)
{
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		printf("错误: 文件打开失败: %s\n", strerror(errno));
		return 0;
	}

	size_t len = fread(buf, 1, limit, f);
	fclose(f);
	return len;
}

static void
burn_usb(char *device, bool wait, char *burner, uint32_t *addrs, char **images, int parts)
{
	if (cskburn_usb_init() != 0) {
		printf("错误: 初始化失败\n");
		goto err_init;
	}

	if (wait) {
		int w = 0;
		while (!cskburn_usb_wait(device, 10)) {
			if (w == 0) {
				w = 1;
			} else if (w == 1) {
				printf("正在等待设备接入…\n");
				w = 2;
			}
		}
	}

	cskburn_usb_device_t *dev = cskburn_usb_open(device);
	if (dev == NULL) {
		printf("错误: 设备打开失败\n");
		goto err_open;
	}

	uint8_t *burner_buf = malloc(MAX_IMAGE_SIZE);
	uint32_t burner_len = read_file(burner, burner_buf, MAX_IMAGE_SIZE);
	if (burner_len == 0) {
		printf("错误: 无法读取 %s\n", burner);
		goto err_enter;
	}

	printf("正在进入烧录模式…\n");
	if (cskburn_usb_enter(dev, burner_buf, burner_len)) {
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

		printf("正在烧录分区 %d (0x%08X)…\n", i + 1, addrs[i]);
		if (cskburn_usb_write(dev, addrs[i], image_buf, image_len)) {
			printf("错误: 无法烧录分区 %d\n", i + 1);
			goto err_write;
		}
	}

	cskburn_usb_finish(dev);

	printf("烧录完成\n");

err_write:
	free(image_buf);
err_enter:
	free(burner_buf);
	cskburn_usb_close(&dev);
err_open:
err_init:
	cskburn_usb_exit();
}
