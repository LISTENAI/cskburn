#ifndef __LIB_CSKBURN_USB__
#define __LIB_CSKBURN_USB__

#include <stdbool.h>
#include <stdint.h>

int cskburn_usb_init(bool verbose);
void cskburn_usb_exit(void);

int cskburn_usb_wait(int16_t bus, int16_t address, int timeout);

typedef struct {
	void *handle;
} cskburn_usb_device_t;

cskburn_usb_device_t *cskburn_usb_open(int16_t bus, int16_t address);
void cskburn_usb_close(cskburn_usb_device_t **dev);

int cskburn_usb_enter(cskburn_usb_device_t *dev);
int cskburn_usb_write(cskburn_usb_device_t *dev, uint32_t addr, uint8_t *image, uint32_t len,
		void (*on_progress)(int32_t wrote_bytes, uint32_t total_bytes));
int cskburn_usb_finish(cskburn_usb_device_t *dev);

#endif  // __LIB_CSKBURN_USB__
