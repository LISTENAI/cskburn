#ifndef __LIB_CSKBURN_SERIAL_SLIP__
#define __LIB_CSKBURN_SERIAL_SLIP__

#include <stdint.h>

uint32_t slip_encode(uint8_t *in, uint8_t *out, uint32_t len);
uint32_t slip_decode(uint8_t *buf, uint32_t *len, uint32_t limit);

#endif  // __LIB_CSKBURN_SERIAL_SLIP__
