#include <stdbool.h>

#include "slip.h"

#define END 0300
#define ESC 0333
#define ESC_END 0334
#define ESC_ESC 0335

uint32_t
slip_encode(uint8_t *in, uint8_t *out, uint32_t len)
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

uint32_t
slip_decode(uint8_t *buf, uint32_t *len, uint32_t limit)
{
	uint32_t ii = 0, oi = 0;
	while (ii < limit) {
		if (buf[ii] == END) {
			*len = oi;
			return ii + 1;
		} else if (ii < limit - 1 && buf[ii] == ESC && buf[ii + 1] == ESC_END) {
			buf[oi++] = END;
			ii += 2;
		} else if (ii < limit - 1 && buf[ii] == ESC && buf[ii + 1] == ESC_ESC) {
			buf[oi++] = ESC;
			ii += 2;
		} else {
			buf[oi++] = buf[ii];
			ii += 1;
		}
	}
	return 0;
}
