#include <serial.h>

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "set_baud.h"
#include "usb_info.h"

struct _serial_dev_t {
	const char *path;
	int fd;
};

static int
set_interface_attribs(int fd, int speed)
{
	int ret = 0;
	struct termios tty;

	ret = ioctl(fd, TIOCEXCL);
	if (ret < 0) {
		return ret;
	}

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

	fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		return NULL;
	}

	ret = set_interface_attribs(fd, 115200);
	if (ret != 0) {
		return NULL;
	}

	serial_dev_t *dev = (serial_dev_t *)malloc(sizeof(serial_dev_t));
	dev->path = path;
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

bool
serial_set_speed(serial_dev_t *dev, uint32_t speed)
{
	return set_baud(dev->fd, speed) == 0;
}

static inline void
set_fd_timeout(serial_dev_t *dev, fd_set *fds, struct timeval *tv, uint64_t timeout)
{
	FD_ZERO(fds);
	FD_SET(dev->fd, fds);

	tv->tv_sec = timeout / 1000;
	tv->tv_usec = (timeout % 1000) * 1000;
}

int32_t
serial_read(serial_dev_t *dev, void *buf, size_t count, uint64_t timeout)
{
	fd_set fds;
	struct timeval tv;
	set_fd_timeout(dev, &fds, &tv, timeout);

	int ret = select(dev->fd + 1, &fds, NULL, NULL, &tv);
	if (ret <= 0) {
		return ret;
	}

	return read(dev->fd, buf, count);
}

int32_t
serial_write(serial_dev_t *dev, const void *buf, size_t count, uint64_t timeout)
{
	fd_set fds;
	struct timeval tv;
	set_fd_timeout(dev, &fds, &tv, timeout);

	int ret = select(dev->fd + 1, NULL, &fds, NULL, &tv);
	if (ret <= 0) {
		return ret;
	}

	return write(dev->fd, buf, count);
}

void
serial_discard_input(serial_dev_t *dev)
{
	tcflush(dev->fd, TCIFLUSH);
}

void
serial_discard_output(serial_dev_t *dev)
{
	tcflush(dev->fd, TCOFLUSH);
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

bool
serial_get_usb_info(serial_dev_t *dev, serial_usb_info_t *info)
{
	return get_usb_info(dev->path, info);
}
