#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "core.h"
#include "protocol.h"

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

static libusb_device *
find_device(int16_t bus, int16_t address)
{
	libusb_device **devs;

	if (libusb_get_device_list(NULL, &devs) < 0) {
		return NULL;
	}

	libusb_device *found = NULL;
	struct libusb_device_descriptor desc = {0};

	for (size_t i = 0; devs[i] != NULL; i++) {
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

int
cskburn_usb_wait(int16_t bus, int16_t address, int timeout)
{
	while (timeout > 0) {
		libusb_device *found = find_device(bus, address);
		if (found != NULL) {
			return 1;
		}
		timeout -= 10;
		usleep(10 * 1000);
	}

	return 0;
}

cskburn_usb_device_t *
cskburn_usb_open(int16_t bus, int16_t address)
{
	cskburn_usb_device_t *dev = (cskburn_usb_device_t *)malloc(sizeof(cskburn_usb_device_t));

	libusb_device *device = find_device(bus, address);
	if (device == NULL) {
		goto err_open;
	}

	libusb_device_handle *handle = NULL;
	if (libusb_open(device, &handle) < 0) {
		goto err_open;
	}

	libusb_set_configuration(handle, 1);
	libusb_claim_interface(handle, 0);
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
	uint32_t hdr_len = sizeof(csk_command_req_t) + sizeof(csk_mem_data_t);
	uint32_t buf_len = hdr_len + DATA_BUF_SIZE;
	uint8_t *buf = malloc(buf_len);

	if (!send_mem_begin_command(dev, buf, buf_len, len)) {
		printf("错误: MEM_BEGIN 发送失败\n");
		goto err;
	}

	uint32_t xferred = 0;
	uint32_t packet_len = 0;
	uint32_t seq = 0;
	while (1) {
		packet_len = DATA_BUF_SIZE;
		if (xferred + packet_len > len) {
			packet_len = len - xferred;
		}

		if (!send_mem_data_command(dev, buf, buf_len, burner + xferred, packet_len, seq)) {
			printf("错误: MEM_DATA 发送失败\n");
			goto err;
		}

		xferred += packet_len;
		seq++;

		if (xferred >= len) {
			break;
		}

		usleep(10 * 1000);
	}

	if (!send_mem_end_command(dev, buf, buf_len)) {
		printf("错误: MEM_END 发送失败\n");
		goto err;
	}

	usleep(100 * 1000);

	int tmp = 0;
	if (libusb_get_configuration(dev->handle, &tmp) != 0) {
		printf("错误: 设备未响应\n");
		goto err;
	}

	return 0;

err:
	free(buf);
	return 1;
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
