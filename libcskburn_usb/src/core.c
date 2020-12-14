#include <stdio.h>
#include <stdlib.h>

#include "core.h"

int
cskburn_usb_init(void)
{
	return libusb_init(NULL);
}

void
cskburn_usb_exit(void)
{
	libusb_exit(NULL);
}

int
cskburn_usb_wait(char *device, int timeout)
{
	return 1;
}

cskburn_usb_device_t *
cskburn_usb_open(char *device)
{
	cskburn_usb_device_t *dev = (cskburn_usb_device_t *)malloc(sizeof(cskburn_usb_device_t));

	libusb_device_handle *handle = libusb_open_device_with_vid_pid(NULL, CSK_VID, CSK_PID);
	if (handle == NULL) {
		goto err_open;
	}

	dev->handle = handle;
	return dev;

err_open:
	free(dev);
	return NULL;
}

void
cskburn_usb_close(cskburn_usb_device_t **dev)
{
	if (dev != NULL) {
		if ((*dev)->handle != NULL) {
			libusb_close((*dev)->handle);
			(*dev)->handle = NULL;
		}
		free(*dev);
		*dev = NULL;
	}
}

int
cskburn_usb_enter(cskburn_usb_device_t *dev, uint8_t *burner, uint32_t len)
{
	return 0;
}

int
cskburn_usb_write(cskburn_usb_device_t *dev, uint32_t addr, uint8_t *image, uint32_t len)
{
	return 0;
}

int
cskburn_usb_finish(cskburn_usb_device_t *dev)
{
	return 0;
}
