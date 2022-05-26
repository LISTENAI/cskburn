#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usb_info.h"

static bool
read_uint16(const char *path, uint16_t *value)
{
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		return false;
	}

	bool success = fscanf(file, "%04hx", value) == 1;

	fclose(file);
	return success;
}

bool
get_usb_info(const char *path, serial_usb_info_t *info)
{
	char tmp[PATH_MAX];

	const char *name = basename((char *)path);

	char device_path[PATH_MAX];
	sprintf(tmp, "/sys/class/tty/%s/device", name);
	if (realpath(tmp, device_path) == NULL) {
		return false;
	}

	char subsystem_path[PATH_MAX];
	sprintf(tmp, "/sys/class/tty/%s/device/subsystem", name);
	if (realpath(tmp, subsystem_path) == NULL) {
		return false;
	}

	const char *subsystem = basename(subsystem_path);

	char *usb_interface_path;
	if (strcmp(subsystem, "usb-serial") == 0) {
		usb_interface_path = dirname(device_path);
	} else if (strcmp(subsystem, "usb") == 0) {
		usb_interface_path = device_path;
	} else {
		return false;
	}

	const char *usb_device_path = dirname(usb_interface_path);

	sprintf(tmp, "%s/idVendor", usb_device_path);
	if (!read_uint16(tmp, &info->vendor_id)) {
		return false;
	}

	sprintf(tmp, "%s/idProduct", usb_device_path);
	if (!read_uint16(tmp, &info->product_id)) {
		return false;
	}

	return true;
}
