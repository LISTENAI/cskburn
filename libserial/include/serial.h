#ifndef __LIB_SERIAL__
#define __LIB_SERIAL__

#include <stdbool.h>

struct _serial_dev_t;
typedef struct _serial_dev_t serial_dev_t;

serial_dev_t *serial_open(const char *path);
void serial_close(serial_dev_t **dev);

bool serial_set_rts(serial_dev_t *dev, bool val);
bool serial_set_dtr(serial_dev_t *dev, bool val);

#endif  // __LIB_SERIAL__
