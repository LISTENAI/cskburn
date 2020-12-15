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

	usleep(800 * 1000);

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
