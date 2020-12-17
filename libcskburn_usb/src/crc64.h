#ifndef __LIB_CSKBURN_USB_CRC64__
#define __LIB_CSKBURN_USB_CRC64__

#include <stdint.h>

uint64_t crc64(uint64_t crc, const unsigned char *s, uint64_t l);

#endif  // __LIB_CSKBURN_USB_CRC64__
