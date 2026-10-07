#include <IOKit/serial/ioss.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "set_baud.h"

static bool
is_standard_baud(speed_t baud)
{
	static const speed_t rates[] = {B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800,
			B2400, B4800, B7200, B9600, B14400, B19200, B28800, B38400, B57600, B76800, B115200,
			B230400};

	for (unsigned i = 0; i < sizeof(rates) / sizeof(rates[0]); i++) {
		if (baud == rates[i]) {
			return true;
		}
	}
	return false;
}

int
set_baud(int fd, int speed)
{
	speed_t baud = (speed_t)speed;
	if (is_standard_baud(baud)) {
		struct termios tty;
		if (tcgetattr(fd, &tty) != 0 || cfsetspeed(&tty, baud) != 0 ||
				tcsetattr(fd, TCSANOW, &tty) != 0) {
			return -errno;
		}
		return 0;
	}

	return ioctl(fd, IOSSIOSPEED, &baud) == 0 ? 0 : -errno;
}
