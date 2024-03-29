#ifndef __LIB_CSKBURN_USB__
#define __LIB_CSKBURN_USB__

#include <stdbool.h>
#include <stdint.h>

#include "io.h"

bool cskburn_usb_init(void);
void cskburn_usb_exit(void);

bool cskburn_usb_wait(int16_t bus, int16_t address, int timeout);

typedef struct {
	void *handle;
} cskburn_usb_device_t;

cskburn_usb_device_t *cskburn_usb_open(int16_t bus, int16_t address);
void cskburn_usb_close(cskburn_usb_device_t **dev);

bool cskburn_usb_enter(cskburn_usb_device_t *dev, uint8_t *burner, uint32_t len);
bool cskburn_usb_write(cskburn_usb_device_t *dev, uint32_t addr, reader_t *reader,
		void (*on_progress)(int32_t wrote_bytes, uint32_t total_bytes));

bool cskburn_usb_show_done(cskburn_usb_device_t *dev);

#endif  // __LIB_CSKBURN_USB__
