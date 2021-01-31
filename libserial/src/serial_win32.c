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
