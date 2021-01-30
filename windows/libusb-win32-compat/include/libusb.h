#ifndef __LIBUSB_WIN32_COMPAT_H__
#define __LIBUSB_WIN32_COMPAT_H__

#include <stdint.h>
#include <lusb0_usb.h>

enum libusb_endpoint_direction {
	/** In: device-to-host */
	LIBUSB_ENDPOINT_IN = 0x80,

	/** Out: host-to-device */
	LIBUSB_ENDPOINT_OUT = 0x00
};

struct libusb_device_descriptor {
	uint16_t idVendor;
	uint16_t idProduct;
};

typedef void libusb_context;
typedef usb_dev_handle libusb_device_handle;
typedef struct usb_device libusb_device;

int libusb_init(void *ignored);
void libusb_exit(void *ignored);

int libusb_open(libusb_device *dev, libusb_device_handle **dev_handle);
void libusb_close(libusb_device_handle *dev_handle);

int libusb_bulk_transfer(libusb_device_handle *dev_handle, unsigned char endpoint,
		unsigned char *data, int length, int *actual_length, unsigned int timeout);

int64_t libusb_get_device_list(libusb_context *ctx, libusb_device ***list);
void libusb_free_device_list(libusb_device **list, int unref_devices);

int libusb_get_device_descriptor(libusb_device *dev, struct libusb_device_descriptor *desc);
uint8_t libusb_get_bus_number(libusb_device *dev);
uint8_t libusb_get_device_address(libusb_device *dev);

int libusb_set_configuration(libusb_device_handle *dev_handle, int configuration);
int libusb_get_configuration(libusb_device_handle *dev, int *config);

int libusb_claim_interface(libusb_device_handle *dev_handle, int interface_number);
int libusb_set_interface_alt_setting(
		libusb_device_handle *dev_handle, int interface_number, int alternate_setting);

#endif
