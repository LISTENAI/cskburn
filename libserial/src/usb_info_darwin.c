#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <stdint.h>

#include "usb_info.h"

static bool
get_string_property(io_object_t device, const char *property, char *buf, size_t limit)
{
	CFTypeRef container = IORegistryEntryCreateCFProperty(device,
			CFStringCreateWithCString(kCFAllocatorDefault, property, kCFStringEncodingUTF8),
			kCFAllocatorDefault, 0);

	if (container) {
		bool success = CFStringGetCString(container, buf, limit, kCFStringEncodingUTF8);
		CFRelease(container);
		return success;
	} else {
		return false;
	}
}

static bool
get_uint16_property(io_object_t device, const char *property, uint16_t *value)
{
	CFTypeRef container = IORegistryEntryCreateCFProperty(device,
			CFStringCreateWithCString(kCFAllocatorDefault, property, kCFStringEncodingUTF8),
			kCFAllocatorDefault, 0);

	if (container) {
		bool success = CFNumberGetValue(container, kCFNumberSInt16Type, value);
		CFRelease(container);
		return success;
	} else {
		return false;
	}
}

static bool
get_uint32_property(io_object_t device, const char *property, uint32_t *value)
{
	CFTypeRef container = IORegistryEntryCreateCFProperty(device,
			CFStringCreateWithCString(kCFAllocatorDefault, property, kCFStringEncodingUTF8),
			kCFAllocatorDefault, 0);

	if (container) {
		bool success = CFNumberGetValue(container, kCFNumberSInt32Type, value);
		CFRelease(container);
		return success;
	} else {
		return false;
	}
}

static bool
GetParentDeviceByType(io_object_t device, const char *type, io_object_t *parent)
{
	io_name_t name;
	while (true) {
		if (IOObjectGetClass(device, name) != KERN_SUCCESS) {
			return false;
		}

		if (strcmp(name, type) == 0) {
			*parent = device;
			return true;
		}

		if (IORegistryEntryGetParentEntry(device, kIOServicePlane, &device) != KERN_SUCCESS) {
			return false;
		}
	}

	return false;
}

bool
get_usb_info(const char *path, serial_usb_info_t *info)
{
	io_name_t name;
	io_object_t usb_device;
	bool success = false;

	io_iterator_t iterator;
	if (IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("IOSerialBSDClient"),
				&iterator) != KERN_SUCCESS) {
		return false;
	}
	while (IOIteratorIsValid(iterator)) {
		io_object_t service = IOIteratorNext(iterator);
		if (!service) break;
		memset(name, 0, sizeof(name));
		get_string_property(service, "IOCalloutDevice", name, sizeof(name));
		if (strcmp(name, path) == 0) {
			if (GetParentDeviceByType(service, "IOUSBHostDevice", &usb_device) ||
					GetParentDeviceByType(service, "IOUSBDevice", &usb_device)) {
				success = get_uint16_property(usb_device, "idVendor", &info->vendor_id) &&
						  get_uint16_property(usb_device, "idProduct", &info->product_id);
			}
			break;
		}
	}
	IOObjectRelease(iterator);

	return success;
}
