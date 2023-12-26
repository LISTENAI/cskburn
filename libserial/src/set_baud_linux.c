#include <asm/termios.h>
#include <errno.h>

#include "set_baud.h"

extern int ioctl(int, unsigned long, ...);

int
set_baud(int fd, int speed)
{
	int ret = 0;
	struct termios2 tio2;

	ret = ioctl(fd, TCGETS2, &tio2);
	if (ret != 0) {
		return -errno;
	}

	tio2.c_cflag &= ~CBAUD;
	tio2.c_cflag |= BOTHER;
	tio2.c_ispeed = speed;
	tio2.c_ospeed = speed;

	ret = ioctl(fd, TCSETS2, &tio2);
	if (ret != 0) {
		return -errno;
	}

	return 0;
}
