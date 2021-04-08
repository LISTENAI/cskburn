#include <serial.h>

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "set_baud.h"

struct _serial_dev_t {
	int fd;
};

static int
set_interface_attribs(int fd, int speed)
{
	int ret = 0;
	struct termios tty;

	ret = tcgetattr(fd, &tty);
	if (ret < 0) {
		return ret;
	}

	tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8; /* 8-bit characters */
	tty.c_cflag &= ~PARENB; /* no parity bit */
	tty.c_cflag &= ~CSTOPB; /* only need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 1;

	ret = tcsetattr(fd, TCSANOW, &tty);
	if (ret != 0) {
		return ret;
	}

	ret = set_baud(fd, speed);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int
set_modem_control(int fd, int flag, int val)
{
	int ret = 0;
	int bits = 0;

	ret = ioctl(fd, TIOCMGET, &bits);
	if (ret != 0) {
		return ret;
	}

	if (val) {
		bits |= flag;
	} else {
		bits &= ~flag;
	}

	ret = ioctl(fd, TIOCMSET, &bits);
	if (ret == 0) {
		return ret;
	}

	return 1;
}

serial_dev_t *
serial_open(const char *path)
{
	int fd, ret;

	fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		return NULL;
	}

	ret = set_interface_attribs(fd, 115200);
	if (ret != 0) {
		return NULL;
	}

	serial_dev_t *dev = (serial_dev_t *)malloc(sizeof(serial_dev_t));
	dev->fd = fd;

	return dev;
}

void
serial_close(serial_dev_t **dev)
{
	close((*dev)->fd);
	free(*dev);
	*dev = NULL;
}

uint32_t
serial_get_speed(serial_dev_t *dev)
{
#if 0
	int ret = 0;
	struct termios tty;

	ret = tcgetattr(dev->fd, &tty);
	if (ret < 0) {
		return 0;
	}

	return cfgetospeed(&tty);
#else
	return 115200;
#endif
}

bool
serial_set_speed(serial_dev_t *dev, uint32_t speed)
{
	return set_baud(dev->fd, speed) == 0;
}

ssize_t
serial_read(serial_dev_t *dev, void *buf, size_t count)
{
	return read(dev->fd, buf, count);
}

ssize_t
serial_write(serial_dev_t *dev, const void *buf, size_t count)
{
	ssize_t wrote = write(dev->fd, buf, count);
	tcdrain(dev->fd);
	return wrote;
}

bool
serial_set_rts(serial_dev_t *dev, bool val)
{
	return set_modem_control(dev->fd, TIOCM_RTS, val);
}

bool
serial_set_dtr(serial_dev_t *dev, bool val)
{
	return set_modem_control(dev->fd, TIOCM_DTR, val);
}
