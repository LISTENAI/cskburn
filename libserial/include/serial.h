#ifndef __LIB_SERIAL__
#define __LIB_SERIAL__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

struct _serial_dev_t;
typedef struct _serial_dev_t serial_dev_t;

serial_dev_t *serial_open(const char *path);
void serial_close(serial_dev_t **dev);

bool serial_set_speed(serial_dev_t *dev, uint32_t speed);

int32_t serial_read(serial_dev_t *dev, void *buf, size_t count, uint64_t timeout);
int32_t serial_write(serial_dev_t *dev, const void *buf, size_t count, uint64_t timeout);

void serial_discard_input(serial_dev_t *dev);
void serial_discard_output(serial_dev_t *dev);

#define SERIAL_HIGH false
#define SERIAL_LOW true

/**
 * @brief  Set RTS signal
 *
 * @param dev serial device
 * @param active true to set RTS signal (low), false to clear it (high)
 * @return true if succeed, false otherwise
 */
bool serial_set_rts(serial_dev_t *dev, bool active);

/**
 * @brief  Set DTR signal
 *
 * @param dev serial device
 * @param active true to set DTR signal (low), false to clear it (high)
 * @return true if succeed, false otherwise
 */
bool serial_set_dtr(serial_dev_t *dev, bool active);

typedef struct {
	uint16_t vendor_id;
	uint16_t product_id;
} serial_usb_info_t;

bool serial_get_usb_info(serial_dev_t *dev, serial_usb_info_t *info);

#endif  // __LIB_SERIAL__
