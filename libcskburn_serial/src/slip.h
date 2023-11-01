#ifndef __LIB_CSKBURN_SERIAL_SLIP__
#define __LIB_CSKBURN_SERIAL_SLIP__

#include <stdbool.h>
#include <stdint.h>

uint32_t slip_encode(const uint8_t *in, uint8_t *out, uint32_t len);
bool slip_decode(const uint8_t *in, uint8_t *out, uint32_t *ii, uint32_t *oi, uint32_t limit);

#endif  // __LIB_CSKBURN_SERIAL_SLIP__
