#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

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

	uint32_t addr;
	char *address, *image;
	for (int i = 0; i < part_count; i++) {
		address = parts[i * 2];
		image = parts[i * 2 + 1];

		if (sscanf(address, "0x%x", &addr) != 1 && sscanf(address, "%d", &addr) != 1) {
			printf("错误: 分区 %d 的地址格式不合法: %s\n", i + 1, address);
			return 0;
		}

		if (access(image, F_OK) != 0) {
			printf("错误: 分区 %d 的文件不存在: %s\n", i + 1, image);
			return 0;
		}

		printf("分区 %d: 0x%08x %s\n", i + 1, addr, image);
	}

	return 0;
}
