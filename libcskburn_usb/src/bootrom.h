#ifndef __LIB_CSKBURN_USB_BOOTROM__
#define __LIB_CSKBURN_USB_BOOTROM__

#include <stdint.h>
#include <stdbool.h>

bool bootrom_load(void *handle, uint8_t *burner, uint32_t len);

#endif  // __LIB_CSKBURN_USB_BOOTROM__
