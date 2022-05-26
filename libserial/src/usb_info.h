#ifndef __LIB_SERIAL_USB_INFO__
#define __LIB_SERIAL_USB_INFO__

#include <serial.h>

bool get_usb_info(const char *path, serial_usb_info_t *info);

#endif  // __LIB_SERIAL_USB_INFO__
