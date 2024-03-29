#include "core.h"

#include <stdio.h>
#include <stdlib.h>

#include "bootrom.h"
#include "burner.h"
#include "cskburn_usb.h"
#include "libusb.h"
#include "log.h"
#include "msleep.h"

extern uint8_t burner_usb[];
extern uint32_t burner_usb_len;

bool
cskburn_usb_init()
{
	return libusb_init(NULL) == 0;
}

void
cskburn_usb_exit(void)
{
	libusb_exit(NULL);
}

static libusb_device *
find_device(int16_t bus, int16_t address)
{
	int64_t count = 0;
	libusb_device **devs;

	if ((count = libusb_get_device_list(NULL, &devs)) < 0) {
		return NULL;
	}

	libusb_device *found = NULL;
	struct libusb_device_descriptor desc = {0};

	for (size_t i = 0; i < count; i++) {
		if (libusb_get_device_descriptor(devs[i], &desc) < 0) {
			continue;
		}
		if (desc.idVendor != CSK_VID || desc.idProduct != CSK_PID) {
			continue;
		}
		if (bus != -1 && address != -1) {
			if (bus != libusb_get_bus_number(devs[i]) ||
					address != libusb_get_device_address(devs[i])) {
				continue;
			}
		}
		found = devs[i];
		break;
	}

	libusb_free_device_list(devs, 1);
	return found;
}

bool
cskburn_usb_wait(int16_t bus, int16_t address, int timeout)
{
	while (timeout > 0) {
		libusb_device *found = find_device(bus, address);
		if (found != NULL) {
			return true;
		}
		timeout -= 10;
		msleep(10);
	}

	return false;
}

cskburn_usb_device_t *
cskburn_usb_open(int16_t bus, int16_t address)
{
	int ret;
	cskburn_usb_device_t *dev = (cskburn_usb_device_t *)malloc(sizeof(cskburn_usb_device_t));

	libusb_device *device = find_device(bus, address);
	if (device == NULL) {
		goto err_open;
	}

	libusb_device_handle *handle = NULL;
	if (libusb_open(device, &handle) < 0) {
		goto err_open;
	}

	libusb_set_auto_detach_kernel_driver(handle, 1);
	if ((ret = libusb_set_configuration(handle, 1)) != 0) {
		LOGD("DEBUG: Failed configuring device: %d", ret);
	}
	if ((ret = libusb_claim_interface(handle, 0)) != 0) {
		LOGD("DEBUG: Failed setting interface: %d", ret);
	}
	libusb_set_interface_alt_setting(handle, 0, 0);

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
			libusb_release_interface((*dev)->handle, 0);
			libusb_close((*dev)->handle);
			(*dev)->handle = NULL;
		}
		free(*dev);
		*dev = NULL;
	}
}

bool
cskburn_usb_enter(cskburn_usb_device_t *dev, uint8_t *burner, uint32_t len)
{
	if (len == 0) {
		burner = burner_usb;
		len = burner_usb_len;
	}

	if (burner_sync(dev->handle, 10)) {
		return true;
	}

	if (!bootrom_load(dev->handle, burner, len)) {
		return false;
	}

	msleep(100);

	int tmp = 0;
	if (libusb_get_configuration(dev->handle, &tmp) != 0) {
		LOGD("DEBUG: Device not responding");
		return false;
	}

	return true;
}

bool
cskburn_usb_write(cskburn_usb_device_t *dev, uint32_t addr, reader_t *reader,
		void (*on_progress)(int32_t wrote_bytes, uint32_t total_bytes))
{
	uint8_t image[8 * 1024 * 1024];
	uint32_t len = reader->read(reader, image, sizeof(image));
	return burner_burn(dev->handle, addr, image, len, on_progress);
}

bool
cskburn_usb_show_done(cskburn_usb_device_t *dev)
{
	return burner_show_done(dev->handle, 3);
}
