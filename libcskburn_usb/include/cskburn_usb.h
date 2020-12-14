#ifndef __LIB_CSKBURN_USB__
#define __LIB_CSKBURN_USB__

#include <stdint.h>

int cskburn_usb_init(void);
void cskburn_usb_exit(void);

int cskburn_usb_wait(char *device, int timeout);

typedef struct {
	void *handle;
} cskburn_usb_device_t;

cskburn_usb_device_t *cskburn_usb_open(char *device);
void cskburn_usb_close(cskburn_usb_device_t **dev);

int cskburn_usb_enter(cskburn_usb_device_t *dev, uint8_t *burner, uint32_t len);
int cskburn_usb_write(cskburn_usb_device_t *dev, uint32_t addr, uint8_t *image, uint32_t len);
int cskburn_usb_finish(cskburn_usb_device_t *dev);

#endif  // __LIB_CSKBURN_USB__
