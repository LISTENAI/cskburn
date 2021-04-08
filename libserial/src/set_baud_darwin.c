#include "set_baud.h"

#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>

int
set_baud(int fd, int speed)
{
	int ret = 0;

	ret = ioctl(fd, IOSSIOSPEED, &speed);
	if (ret != 0) {
		return ret;
	}

	return 0;
}
