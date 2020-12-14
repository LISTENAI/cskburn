#ifndef __LIB_CSKBURN_USB_CORE__
#define __LIB_CSKBURN_USB_CORE__

#include <libusb.h>
#include <cskburn_usb.h>

#define CSK_VID 0x1234
#define CSK_PID 0x5678

#define EP_ADDR_DATA_IN (3 | LIBUSB_ENDPOINT_IN)
#define EP_ADDR_DATA_OUT (4 | LIBUSB_ENDPOINT_OUT)
#define EP_ADDR_CMD_RESP_IN (5 | LIBUSB_ENDPOINT_IN)
#define EP_ADDR_CMD_RESP_OUT (6 | LIBUSB_ENDPOINT_OUT)

#endif  // __LIB_CSKBURN_USB_CORE__
