#include <IOKit/serial/ioss.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "set_baud.h"

int
set_baud(int fd, int speed)
{
	int ret = 0;

	ret = ioctl(fd, IOSSIOSPEED, &speed);
	if (ret != 0) {
		return -errno;
	}

	return 0;
}
