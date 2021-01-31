#include <serial.h>

struct _serial_dev_t {
}

serial_dev_t *
serial_open(const char *path)
{
	return NULL;
}

void
serial_close(serial_dev_t **dev)
{
}

bool
serial_set_rts(serial_dev_t *dev, bool val)
{
	return false;
}

bool
serial_set_dtr(serial_dev_t *dev, bool val)
{
	return false;
}
