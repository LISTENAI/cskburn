#ifndef __LIB_SLIP__
#define __LIB_SLIP__

#include <stdint.h>

#include "serial.h"

struct _slip_dev_t;
typedef struct _slip_dev_t slip_dev_t;

/**
 * @brief Allocate and initialize a SLIP object
 *
 * @param serial Serial device
 * @param tx_buf_len Length of transmit buffer
 * @param rx_buf_len Length of receive buffer
 *
 * @return Pointer to the allocated SLIP object
 */
slip_dev_t *slip_init(serial_dev_t *serial, size_t tx_buf_len, size_t rx_buf_len);

/**
 * @brief Deinitialize a SLIP object
 *
 * @param dev Pointer to the SLIP object
 */
void slip_deinit(slip_dev_t **dev);

/**
 * @brief Read from a SLIP object
 *
 * @param dev SLIP object
 * @param buf Buffer to store the read data
 * @param count Number of bytes to read
 * @param timeout Timeout in milliseconds
 *
 * @return Number of bytes read
 * @retval -ETIMEDOUT if timeout
 * @retval -ENOMEM if buffer is too small to hold the upcoming packet
 * @retval -errno on other errors from serial device
 */
ssize_t slip_read(slip_dev_t *dev, uint8_t *buf, size_t count, uint64_t timeout);

/**
 * @brief Write to a SLIP object
 *
 * @param dev SLIP object
 * @param buf Buffer to write
 * @param count Number of bytes to write
 * @param timeout Timeout in milliseconds
 *
 * @return Number of bytes written
 * @retval -ETIMEDOUT if timeout
 * @retval -ENOMEM if the transmit buffer is full
 * @retval -errno on other errors from serial device
 */
ssize_t slip_write(slip_dev_t *dev, const uint8_t *buf, size_t count, uint64_t timeout);

#endif  // __LIB_SLIP__
