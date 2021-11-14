#include <libusb.h>

int
libusb_init(void *ignored)
{
	usb_init();
	return 0;
}

void
libusb_exit(void *ignored)
{
	// do nothing
}

int
libusb_open(libusb_device *dev, libusb_device_handle **dev_handle)
{
	usb_dev_handle *handle = usb_open(dev);
	if (handle == NULL) {
		return -1;
	} else {
		*dev_handle = handle;
		return 0;
	}
}

void
libusb_close(libusb_device_handle *dev_handle)
{
	usb_close(dev_handle);
}

#define MAX_DEVICE_CNT 1000

int64_t
libusb_get_device_list(libusb_context *ctx, libusb_device ***list)
{
	usb_find_busses();
	usb_find_devices();

	struct usb_bus *bus;
	struct usb_device *dev;

	uint32_t i = 0;
	*list = (libusb_device **)malloc(sizeof(libusb_device *) * MAX_DEVICE_CNT);

	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			*list[i] = dev;
			i++;
		}
	}

	return i;
}

void
libusb_free_device_list(libusb_device **list, int unused)
{
	free(list);
	list = NULL;
}

int
libusb_get_device_descriptor(libusb_device *dev, struct libusb_device_descriptor *desc)
{
	desc->idVendor = dev->descriptor.idVendor;
	desc->idProduct = dev->descriptor.idProduct;
	return 0;
}

uint8_t
libusb_get_bus_number(libusb_device *dev)
{
	return -1;
}

uint8_t
libusb_get_device_address(libusb_device *dev)
{
	return -1;
}

int
libusb_set_configuration(libusb_device_handle *dev_handle, int configuration)
{
	return usb_set_configuration(dev_handle, configuration);
}

int
libusb_get_configuration(libusb_device_handle *dev, int *config)
{
	// 返回结果没用上，暂不实现
	return 0;
}

int
libusb_claim_interface(libusb_device_handle *dev_handle, int interface_number)
{
	return usb_claim_interface(dev_handle, interface_number);
}

int
libusb_set_interface_alt_setting(
		libusb_device_handle *dev_handle, int interface_number, int alternate_setting)
{
	return usb_set_altinterface(dev_handle, alternate_setting);
}

int
libusb_bulk_transfer(libusb_device_handle *dev_handle, unsigned char endpoint, unsigned char *data,
		int length, int *actual_length, unsigned int timeout)
{
	int ret;
	if (endpoint & LIBUSB_ENDPOINT_IN) {
		if ((ret = usb_bulk_read(dev_handle, endpoint, data, length, timeout)) < 0) {
			return ret;
		} else {
			*actual_length = ret;
			return 0;
		}
	} else {
		if ((ret = usb_bulk_write(dev_handle, endpoint, data, length, timeout)) < 0) {
			return ret;
		} else {
			*actual_length = ret;
			return 0;
		}
	}
}
