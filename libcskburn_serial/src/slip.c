#include "slip.h"

#include <stdbool.h>

#define END 0300
#define ESC 0333
#define ESC_END 0334
#define ESC_ESC 0335

uint32_t
slip_encode(const uint8_t *in, uint8_t *out, uint32_t len)
{
	uint32_t oi = 0;
	out[oi++] = END;
	for (uint32_t ii = 0; ii < len; ii++) {
		if (in[ii] == END) {
			out[oi++] = ESC;
			out[oi++] = ESC_END;
		} else if (in[ii] == ESC) {
			out[oi++] = ESC;
			out[oi++] = ESC_ESC;
		} else {
			out[oi++] = in[ii];
		}
	}
	out[oi++] = END;
	return oi;
}

bool
slip_decode(const uint8_t *in, uint8_t *out, uint32_t *ii, uint32_t *oi, uint32_t limit)
{
	*ii = *oi = 0;
	while (*ii < limit) {
		if (in[*ii] == END) {
			*ii += 1;
			return true;
		} else if (*ii == limit - 1 && in[*ii] == ESC) {
			return false;
		} else if (*ii < limit - 1 && in[*ii] == ESC && in[*ii + 1] == ESC_END) {
			out[(*oi)++] = END;
			*ii += 2;
		} else if (*ii < limit - 1 && in[*ii] == ESC && in[*ii + 1] == ESC_ESC) {
			out[(*oi)++] = ESC;
			*ii += 2;
		} else {
			out[(*oi)++] = in[*ii];
			*ii += 1;
		}
	}
	return false;
}
