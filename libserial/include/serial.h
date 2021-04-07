#ifndef __LIB_SERIAL__
#define __LIB_SERIAL__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

struct _serial_dev_t;
typedef struct _serial_dev_t serial_dev_t;

serial_dev_t *serial_open(const char *path);
void serial_close(serial_dev_t **dev);

uint32_t serial_get_speed(serial_dev_t *dev);
bool serial_set_speed(serial_dev_t *dev, uint32_t speed);

ssize_t serial_read(serial_dev_t *dev, void *buf, size_t count);
ssize_t serial_write(serial_dev_t *dev, const void *buf, size_t count);

bool serial_set_rts(serial_dev_t *dev, bool val);
bool serial_set_dtr(serial_dev_t *dev, bool val);

#endif  // __LIB_SERIAL__
