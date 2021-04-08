#include "set_baud.h"

#include <sys/ioctl.h>
#include <termios.h>

int
set_baud(int fd, int speed)
{
	int ret = 0;
	struct termios tty;

	ret = tcgetattr(fd, &tty);
	if (ret < 0) {
		return ret;
	}

	cfsetospeed(&tty, (speed_t)speed);
	cfsetispeed(&tty, (speed_t)speed);

	ret = tcsetattr(fd, TCSANOW, &tty);
	if (ret != 0) {
		return ret;
	}

	return 0;
}
