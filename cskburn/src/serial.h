#ifndef __CSKBURN_SERIAL__
#define __CSKBURN_SERIAL__

#include "features.h"

#ifdef FEATURE_SERIAL
int serial_open(const char *path);
void serial_close(int fd);

int serial_set_rts(int fd, int val);
int serial_set_dtr(int fd, int val);
#endif

#endif  // __CSKBURN_SERIAL__
