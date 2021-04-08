#ifndef __LIB_CSKBURN_SERIAL__
#define __LIB_CSKBURN_SERIAL__

#include <stdbool.h>
#include <stdint.h>

void cskburn_serial_init(bool verbose);

typedef struct {
	void *handle;
} cskburn_serial_device_t;

cskburn_serial_device_t *cskburn_serial_open(const char *path);
void cskburn_serial_close(cskburn_serial_device_t **dev);

bool cskburn_serial_connect(cskburn_serial_device_t *dev);

bool cskburn_serial_enter(cskburn_serial_device_t *dev, uint32_t baud_rate);
bool cskburn_serial_write(cskburn_serial_device_t *dev, uint32_t addr, uint8_t *image, uint32_t len,
		void (*on_progress)(int32_t wrote_bytes, uint32_t total_bytes));

bool cskburn_serial_reset(cskburn_serial_device_t *dev);

#endif  // __LIB_CSKBURN_SERIAL__