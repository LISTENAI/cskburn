#include <IOKit/serial/ioss.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "set_baud.h"

int
set_baud(int fd, int speed)
{
	// IOSSIOSPEED 的参数类型为 speed_t（64 位 macOS 上为 8 字节），
	// 不能直接传 int 的地址，否则内核会多读 4 字节相邻栈内存
	speed_t s = (speed_t)speed;

	int ret = ioctl(fd, IOSSIOSPEED, &s);
	if (ret != 0) {
		return -errno;
	}

	return 0;
}
