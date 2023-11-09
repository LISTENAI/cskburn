#include "slip.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "serial.h"
#include "time_monotonic.h"

#define END 0300
#define ESC 0333
#define ESC_END 0334
#define ESC_ESC 0335

struct _slip_dev_t {
	serial_dev_t *serial;

	uint8_t *tx_buf;
	size_t tx_len;

	uint8_t *rx_buf;
	size_t rx_len;

	enum {
		STATE_SKIP,
		STATE_BYTE,
		STATE_ESC,
	} state;
};

slip_dev_t *
slip_init(serial_dev_t *serial, size_t tx_buf_len, size_t rx_buf_len)
{
	slip_dev_t *slip = calloc(1, sizeof(slip_dev_t));
	slip->serial = serial;

	slip->tx_len = tx_buf_len;
	slip->tx_buf = calloc(1, tx_buf_len);

	slip->rx_len = rx_buf_len;
	slip->rx_buf = calloc(1, rx_buf_len);

	return slip;
}

void
slip_deinit(slip_dev_t **dev)
{
	free((*dev)->tx_buf);
	free((*dev)->rx_buf);
	free(*dev);
	*dev = NULL;
}

ssize_t
slip_read(slip_dev_t *dev, uint8_t *buf, size_t count, uint64_t timeout)
{
	uint8_t *const rx_buf_head = dev->rx_buf;
	uint8_t *const rx_buf_tail = dev->rx_buf + dev->rx_len;

	uint8_t *rx_head = rx_buf_head;
	uint8_t *rx_tail = rx_buf_head;

	uint8_t *buf_head = buf;
	uint8_t *buf_tail = buf + count;

	dev->state = STATE_SKIP;

	uint64_t start = time_monotonic();
	while (rx_tail < rx_buf_tail) {
		ssize_t r = serial_read(dev->serial, rx_tail, rx_buf_tail - rx_tail, timeout);
		if (r == 0) {
			if (TIME_SINCE_MS(start) >= timeout) {
				return -ETIMEDOUT;
			} else {
				continue;
			}
		} else if (r < 0) {
			LOGD("DEBUG: Failed reading command: %zd (%s)", r, strerror(-r));
			return r;
		}

		LOG_TRACE("Read %zd bytes in %d ms", r, TIME_SINCE_MS(start));
#if TRACE_SLIP
		LOG_DUMP(rx_tail, r);
#endif

		rx_tail += r;

		for (uint8_t b = *rx_head; rx_head < rx_tail; b = *(++rx_head)) {
			switch (dev->state) {
				case STATE_SKIP:
					if (b == END) {
						dev->state = STATE_BYTE;
					}
					break;
				case STATE_BYTE:
					if (b == ESC) {
						dev->state = STATE_ESC;
					} else if (b == END) {
						LOG_DUMP(buf, buf_head - buf);
						return buf_head - buf;
					} else {
						if (buf_head + 1 > buf_tail) return -ENOMEM;
						*buf_head++ = b;
					}
					break;
				case STATE_ESC:
					if (b == ESC_END) {
						if (buf_head + 1 > buf_tail) return -ENOMEM;
						*buf_head++ = END;
						dev->state = STATE_BYTE;
					} else if (b == ESC_ESC) {
						if (buf_head + 1 > buf_tail) return -ENOMEM;
						*buf_head++ = ESC;
						dev->state = STATE_BYTE;
					} else {
						return -EINVAL;
					}
					break;
			}
		}
	}

	return -ENOMEM;
}

ssize_t
slip_write(slip_dev_t *dev, const uint8_t *buf, size_t count, uint64_t timeout)
{
	uint8_t *const tx_buf_head = dev->tx_buf;
	uint8_t *const tx_buf_tail = dev->tx_buf + dev->tx_len;

	uint8_t *tx_head = tx_buf_head;
	uint8_t *tx_tail = tx_buf_head;

	if (tx_tail + 1 > tx_buf_tail) return -ENOMEM;
	*tx_tail++ = END;

	for (const uint8_t *buf_head = buf; buf_head < buf + count; buf_head++) {
		if (*buf_head == END) {
			if (tx_tail + 2 > tx_buf_tail) return -ENOMEM;
			*tx_tail++ = ESC;
			*tx_tail++ = ESC_END;
		} else if (*buf_head == ESC) {
			if (tx_tail + 2 > tx_buf_tail) return -ENOMEM;
			*tx_tail++ = ESC;
			*tx_tail++ = ESC_ESC;
		} else {
			if (tx_tail + 1 > tx_buf_tail) return -ENOMEM;
			*tx_tail++ = *buf_head;
		}
	}

	if (tx_tail + 1 > tx_buf_tail) return -ENOMEM;
	*tx_tail++ = END;

	uint64_t start = time_monotonic();
	while (tx_head < tx_tail) {
		ssize_t r = serial_write(dev->serial, tx_head, tx_tail - tx_head, timeout);
		if (r == 0) {
			if (TIME_SINCE_MS(start) >= timeout) {
				return -ETIMEDOUT;
			} else {
				continue;
			}
		} else if (r < 0) {
			LOGD("DEBUG: Failed writing command: %zd (%s)", r, strerror(-r));
			return r;
		}

		LOG_TRACE("Wrote %zd bytes in %d ms", r, TIME_SINCE_MS(start));
#if TRACE_SLIP
		LOG_DUMP(tx_head, r);
#endif

		tx_head += r;
	}

	return tx_tail - dev->tx_buf;
}
