#ifndef __LIB_CSKBURN_USB_BURNER__
#define __LIB_CSKBURN_USB_BURNER__

#include <stdint.h>
#include <stdbool.h>

typedef void (*burner_burn_progress_cb)(int32_t wrote_bytes, uint32_t total_bytes);

bool burner_burn(void *handle, uint32_t addr, uint8_t *image, uint32_t len,
		burner_burn_progress_cb on_progress);

#endif  // __LIB_CSKBURN_USB_BURNER__
