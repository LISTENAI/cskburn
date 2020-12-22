#ifndef __CSKBURN_SERIAL__
#define __CSKBURN_SERIAL__

int serial_open(const char *path);
void serial_close(int fd);

int serial_set_rts(int fd, int val);
int serial_set_dtr(int fd, int val);

#endif  // __CSKBURN_SERIAL__
