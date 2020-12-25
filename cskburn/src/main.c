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
print_help(const char *progname)
{
	printf("用法: %s [<选项>] <burner> <地址1> <文件1> [<地址2> <文件2>...]\n", progname);
	printf("\n");
	printf("烧录选项 (二选一，不传默认 USB):\n");
	printf("  -u, --usb (-|<总线>:<设备>)\t\t使用指定 USB 设备烧录。传 - 表示自动选取第一个 CSK "
		   "设备\n");
	printf("  -s, --serial <端口>\t\t\t使用指定串口端口烧录 (如 /dev/cu.usbserial-*)\n");
	printf("\n");
	printf("其它选项:\n");
	printf("  -h, --help\t\t\t\t显示帮助\n");
	printf("  -V, --version\t\t\t\t显示版本号\n");
	printf("  -w, --wait\t\t\t\t等待设备插入，并自动开始烧录\n");
	printf("  -r, --repeat\t\t\t\t烧录成功后不退出，等待下次插入\n");
	printf("\n");
	printf("用例:\n");
	printf("  cskburn -w -u 128:000 burner.img 0x0 flashboot.bin 0x10000 master.bin 0x100000 "
		   "respack.bin\n");
	printf("  cskburn -w -s /dev/cu.usbserial-1440 burner.img 0x0 flashboot.bin 0x10000 "
		   "master.bin 0x100000 respack.bin\n");
}

static void
print_version(void)
{
	printf("0.0.0\n");
}

static void burn_usb(int16_t bus, int16_t address, bool wait, char *burner, uint32_t *addrs,
		char **images, int parts);

int
main(int argc, char **argv)
{
	bool wait;
	char *usb = NULL, *serial = NULL;
	int16_t usb_bus = -1, usb_addr = -1;

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
				print_help(argv[0]);
				return 0;
		}
	}

	if (serial == NULL && usb != NULL && strcmp(usb, "-") != 0) {
		if (sscanf(usb, "%hu:%hu\n", &usb_bus, &usb_addr) != 2) {
			printf("错误: -u/--usb 参数的格式应为 <总线>:<设备> (如: -u 020:004)\n");
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

	if (serial != NULL) {
		printf("serial: %lu\n", strlen(serial));
	} else {
		burn_usb(usb_bus, usb_addr, wait, burner, addrs, images, part_count);
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
print_progress(uint32_t wrote_bytes, uint32_t total_bytes)
{
	printf("%.2f KB / %.2f KB (%.2f%%)\r", (float)wrote_bytes / 1024.0f,
			(float)total_bytes / 1024.0f, (float)wrote_bytes / (float)total_bytes * 100.0f);
	if (wrote_bytes == total_bytes) {
		printf("\n");
	}
	fflush(stdout);
}

static void
burn_usb(int16_t bus, int16_t address, bool wait, char *burner, uint32_t *addrs, char **images,
		int parts)
{
	if (cskburn_usb_init() != 0) {
		printf("错误: 初始化失败\n");
		goto err_init;
	}

	if (wait) {
		int w = 0;
		while (!cskburn_usb_wait(bus, address, 10)) {
			if (w == 0) {
				w = 1;
			} else if (w == 1) {
				printf("正在等待设备接入…\n");
				w = 2;
			}
		}
	}

	cskburn_usb_device_t *dev = cskburn_usb_open(bus, address);
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
	usleep(2000 * 1000);
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

		printf("正在烧录分区 %d/%d… (0x%08X, %.2f KB)\n", i + 1, parts, addrs[i],
				(float)image_len / 1024.0f);
		if (cskburn_usb_write(dev, addrs[i], image_buf, image_len, print_progress)) {
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
