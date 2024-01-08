#include <errno.h>
#include <log.h>
#include <stdlib.h>

#define ERRID(name) [name] = #name

static const char *errids[] = {
		ERRID(EPERM),
		ERRID(ENOENT),
		ERRID(EIO),
		ERRID(ENXIO),
		ERRID(EBADF),
		ERRID(ENOMEM),
		ERRID(EACCES),
		ERRID(EFAULT),
		ERRID(EBUSY),
		ERRID(EEXIST),
		ERRID(ENODEV),
		ERRID(EINVAL),
		ERRID(EAGAIN),
		ERRID(ENOTSUP),
		ERRID(ETIMEDOUT),
		ERRID(ENOSYS),
		ERRID(EBADMSG),
};

#define ERRID_MAX (sizeof(errids) / sizeof(errids[0]))

const char *
errid(int errnum)
{
	if (errnum < 0 || errnum >= ERRID_MAX) return "UNKNOWN";
	const char *errid = errids[errnum];
	if (errid == NULL) return "UNKNOWN";
	return errid;
}
