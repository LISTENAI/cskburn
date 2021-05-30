#include <serial.h>

#include <windows.h>

struct _serial_dev_t {
	HANDLE handle;
};

serial_dev_t *
serial_open(const char *path)
{
	HANDLE handle = CreateFileA(path,
			GENERIC_READ | GENERIC_WRITE,  // Read/Write
			0,  // No Sharing
			NULL,  // No Security
			OPEN_EXISTING,  // Open existing port only
			0,  // Non Overlapped I/O
			NULL);  // Null for Comm Devices

	if (handle == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	DCB dcb = {0};
	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	if (GetCommState(handle, &dcb) == 0) {
		return NULL;
	}

	dcb.BaudRate = CBR_115200;  //  baud rate
	dcb.ByteSize = 8;  //  data size, xmit and rcv
	dcb.Parity = NOPARITY;  //  parity bit
	dcb.StopBits = ONESTOPBIT;  //  stop bit

	if (SetCommState(handle, &dcb) == 0) {
		return NULL;
	}

	COMMTIMEOUTS timeouts = {0};
	SecureZeroMemory(&timeouts, sizeof(COMMTIMEOUTS));

	if (GetCommTimeouts(handle, &timeouts) == 0) {
		return NULL;
	}

	timeouts.ReadIntervalTimeout = 1;

	if (SetCommTimeouts(handle, &timeouts) == 0) {
		return NULL;
	}

	serial_dev_t *dev = (serial_dev_t *)malloc(sizeof(serial_dev_t));
	dev->handle = handle;

	return dev;
}

void
serial_close(serial_dev_t **dev)
{
	CloseHandle((*dev)->handle);
	free(*dev);
	*dev = NULL;
}

bool
serial_set_speed(serial_dev_t *dev, uint32_t speed)
{
	DCB dcb = {0};
	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	if (GetCommState(dev->handle, &dcb) == 0) {
		return false;
	}

	dcb.BaudRate = speed;

	if (SetCommState(dev->handle, &dcb) == 0) {
		return false;
	}

	return true;
}

int32_t
serial_read(serial_dev_t *dev, void *buf, size_t count)
{
	size_t read = 0;
	if (ReadFile(dev->handle, buf, count, &read, NULL)) {
		return read;
	} else {
		return -1;
	}
}

int32_t
serial_write(serial_dev_t *dev, const void *buf, size_t count)
{
	size_t wrote = 0;
	if (WriteFile(dev->handle, buf, count, &wrote, NULL)) {
		return wrote;
	} else {
		return -1;
	}
}

void
serial_discard_input(serial_dev_t *dev)
{
	// no-op on Windows
}

void
serial_discard_output(serial_dev_t *dev)
{
	// no-op on Windows
}

bool
serial_set_rts(serial_dev_t *dev, bool val)
{
	if (val) {
		return EscapeCommFunction(dev->handle, SETRTS) != 0;
	} else {
		return EscapeCommFunction(dev->handle, CLRRTS) != 0;
	}
}

bool
serial_set_dtr(serial_dev_t *dev, bool val)
{
	if (val) {
		return EscapeCommFunction(dev->handle, SETDTR) != 0;
	} else {
		return EscapeCommFunction(dev->handle, CLRDTR) != 0;
	}
}
