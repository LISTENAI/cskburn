#ifndef __LIB_SERIAL__
#define __LIB_SERIAL__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct _serial_dev_t;
typedef struct _serial_dev_t serial_dev_t;

serial_dev_t *serial_open(const char *path);
void serial_close(serial_dev_t **dev);

bool serial_set_speed(serial_dev_t *dev, uint32_t speed);

ssize_t serial_read(serial_dev_t *dev, void *buf, size_t count, uint64_t timeout);
ssize_t serial_write(serial_dev_t *dev, const void *buf, size_t count, uint64_t timeout);

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

#endif  // __LIB_SERIAL__
