#ifndef __LIB_SLIP__
#define __LIB_SLIP__

#include <stdint.h>

#include "serial.h"

struct _slip_dev_t;
typedef struct _slip_dev_t slip_dev_t;

slip_dev_t *slip_init(serial_dev_t *serial, size_t tx_buf_len, size_t rx_buf_len);
void slip_deinit(slip_dev_t **dev);

ssize_t slip_read(slip_dev_t *dev, uint8_t *buf, size_t count, uint64_t timeout);
ssize_t slip_write(slip_dev_t *dev, const uint8_t *buf, size_t count, uint64_t timeout);

#endif  // __LIB_SLIP__
