#ifndef __LIB_SERIAL__
#define __LIB_SERIAL__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct _serial_dev_t;
typedef struct _serial_dev_t serial_dev_t;

typedef enum {
	SERIAL_RTS_OFF,  // DISABLE RTS
	SERIAL_RTS_ON,  // ENABLE RTS
	SERIAL_RTS_FLOW  // RTS FLOW
} serial_rts_state_t;

typedef enum {
	SERIAL_DTR_OFF,  // DISABLE DTR
	SERIAL_DTR_ON,  // ENABLE DTR
	SERIAL_DTR_FLOW  // DTR FLOW
} serial_dtr_state_t;

int serial_open(const char *path, serial_dev_t **dev);
void serial_close(serial_dev_t **dev);

int serial_set_speed(serial_dev_t *dev, uint32_t speed);

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
 * @return 0 if succeed
 */
int serial_set_rts(serial_dev_t *dev, bool active);

/**
 * @brief  Set DTR signal
 *
 * @param dev serial device
 * @param active true to set DTR signal (low), false to clear it (high)
 * @return 0 if succeed
 */
int serial_set_dtr(serial_dev_t *dev, bool active);

/**
 * @brief  Config RTS state
 *
 * @param dev serial device
 * @param state SERIAL_RTS_OFF, SERIAL_RTS_ON, SERIAL_RTS_FLOW
 * @return 0 if succeed
 */
int serial_config_rts_state(serial_dev_t *dev, serial_rts_state_t state);

/**
 * @brief  Config DTR state
 *
 * @param dev serial device
 * @param state SERIAL_DTR_OFF, SERIAL_DTR_ON, SERIAL_DTR_FLOW
 * @return 0 if succeed
 */
int serial_config_dtr_state(serial_dev_t *dev, serial_dtr_state_t state);

#endif  // __LIB_SERIAL__
