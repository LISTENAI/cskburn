#include <errno.h>
#include <fcntl.h>
#include <serial.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "set_baud.h"
#include "time_monotonic.h"

struct _serial_dev_t {
	int fd;
};

static int
set_interface_attribs(int fd, int speed)
{
	int ret = 0;
	struct termios tty;

	ret = ioctl(fd, TIOCEXCL);
	if (ret != 0) {
		return -errno;
	}

	ret = tcgetattr(fd, &tty);
	if (ret != 0) {
		return -errno;
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
		return -errno;
	}

	return set_baud(fd, speed);
}

static int
set_modem_control(int fd, int flag, int val)
{
	int ret = 0;
	int bits = 0;

	ret = ioctl(fd, TIOCMGET, &bits);
	if (ret != 0) {
		return -errno;
	}

	if (val) {
		bits |= flag;
	} else {
		bits &= ~flag;
	}

	ret = ioctl(fd, TIOCMSET, &bits);
	if (ret != 0) {
		return -errno;
	}

	return 0;
}

int
serial_open(const char *path, serial_dev_t **dev)
{
	int fd, ret;

	fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		return -errno;
	}

	ret = set_interface_attribs(fd, 115200);
	if (ret != 0) {
		return ret;
	}

	(*dev) = (serial_dev_t *)calloc(1, sizeof(serial_dev_t));
	(*dev)->fd = fd;

	return 0;
}

void
serial_close(serial_dev_t **dev)
{
	close((*dev)->fd);
	free(*dev);
	*dev = NULL;
}

int
serial_set_speed(serial_dev_t *dev, uint32_t speed)
{
	return set_baud(dev->fd, speed);
}

static inline void
set_fd_timeout(serial_dev_t *dev, fd_set *fds, struct timeval *tv, uint64_t timeout)
{
	FD_ZERO(fds);
	FD_SET(dev->fd, fds);

	tv->tv_sec = timeout / 1000;
	tv->tv_usec = (timeout % 1000) * 1000;
}

static ssize_t
do_serial_read(serial_dev_t *dev, void *buf, size_t count, uint64_t timeout)
{
	ssize_t ret;

	fd_set fds;
	struct timeval tv;
	set_fd_timeout(dev, &fds, &tv, timeout);

	ret = select(dev->fd + 1, &fds, NULL, NULL, &tv);
	if (ret < 0) {
		return -errno;
	}

	ret = read(dev->fd, buf, count);
	if (ret < 0) {
		return -errno;
	}

	return ret;
}

ssize_t
serial_read(serial_dev_t *dev, void *buf, size_t count, uint64_t timeout)
{
	ssize_t ret;

	uint64_t start = time_monotonic();
	while ((ret = do_serial_read(dev, buf, count, timeout)) <= 0) {
		if (ret == 0 || ret == -EAGAIN) {
			if (TIME_SINCE_MS(start) >= timeout) {
				return -ETIMEDOUT;
			}
		} else {
			return ret;
		}
	}

	return ret;
}

static ssize_t
do_serial_write(serial_dev_t *dev, const void *buf, size_t count, uint64_t timeout)
{
	ssize_t ret;

	fd_set fds;
	struct timeval tv;
	set_fd_timeout(dev, &fds, &tv, timeout);

	ret = select(dev->fd + 1, NULL, &fds, NULL, &tv);
	if (ret < 0) {
		return -errno;
	}

	ret = write(dev->fd, buf, count);
	if (ret < 0) {
		return -errno;
	}

	return ret;
}

ssize_t
serial_write(serial_dev_t *dev, const void *buf, size_t count, uint64_t timeout)
{
	ssize_t ret;

	uint64_t start = time_monotonic();
	while ((ret = do_serial_write(dev, buf, count, timeout)) <= 0) {
		if (ret == 0 || ret == -EAGAIN) {
			if (TIME_SINCE_MS(start) >= timeout) {
				return -ETIMEDOUT;
			}
		} else {
			return ret;
		}
	}

	return ret;
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

int
serial_set_rts(serial_dev_t *dev, bool val)
{
	return set_modem_control(dev->fd, TIOCM_RTS, val);
}

int
serial_set_dtr(serial_dev_t *dev, bool val)
{
	return set_modem_control(dev->fd, TIOCM_DTR, val);
}
