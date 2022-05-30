#include <serial.h>
#include <windows.h>

struct _serial_dev_t {
	HANDLE handle;
	OVERLAPPED overlapped_read;
	OVERLAPPED overlapped_write;
	bool rts;
	bool dtr;
};

static bool
configure_port(HANDLE handle, DWORD baud_rate)
{
	DCB dcb = {0};
	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	if (GetCommState(handle, &dcb) == 0) {
		return false;
	}

	dcb.BaudRate = baud_rate;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	if (SetCommState(handle, &dcb) == 0) {
		return false;
	}

	return true;
}

serial_dev_t *
serial_open(const char *path)
{
	HANDLE handle = CreateFileA(path,
			GENERIC_READ | GENERIC_WRITE,  // Read/Write
			0,  // No Sharing
			NULL,  // No Security
			OPEN_EXISTING,  // Open existing port only
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,  // Overlapped I/O
			NULL);  // Null for Comm Devices

	if (handle == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	if (!configure_port(handle, CBR_115200)) {
		CloseHandle(handle);
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

	SecureZeroMemory(&dev->overlapped_read, sizeof(OVERLAPPED));
	dev->overlapped_read.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

	SecureZeroMemory(&dev->overlapped_write, sizeof(OVERLAPPED));
	dev->overlapped_write.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

	return dev;
}

void
serial_close(serial_dev_t **dev)
{
	CloseHandle((*dev)->overlapped_read.hEvent);
	CloseHandle((*dev)->overlapped_write.hEvent);
	CloseHandle((*dev)->handle);
	free(*dev);
	*dev = NULL;
}

bool
serial_set_speed(serial_dev_t *dev, uint32_t speed)
{
	if (!configure_port(dev->handle, speed)) {
		return false;
	}

	if (!serial_set_dtr(dev, dev->dtr)) {
		return false;
	}

	if (!serial_set_rts(dev, dev->rts)) {
		return false;
	}

	return true;
}

static bool
handle_overlapped_result(HANDLE handle, LPOVERLAPPED overlapped, LPDWORD count, uint64_t timeout)
{
	if (GetLastError() != ERROR_IO_PENDING) {
		return false;
	}

	if (WaitForSingleObject(overlapped->hEvent, timeout) == WAIT_OBJECT_0) {
		return GetOverlappedResult(handle, overlapped, count, FALSE);
	}

	return false;
}

int32_t
serial_read(serial_dev_t *dev, void *buf, size_t count, uint64_t timeout)
{
	DWORD read = 0;

	ResetEvent(dev->overlapped_read.hEvent);

	if (ReadFile(dev->handle, buf, count, (LPDWORD)&read, &dev->overlapped_read) == FALSE) {
		if (!handle_overlapped_result(dev->handle, &dev->overlapped_read, &read, timeout)) {
			return -1;
		}
	}

	return read;
}

int32_t
serial_write(serial_dev_t *dev, const void *buf, size_t count, uint64_t timeout)
{
	DWORD wrote = 0;

	if (WriteFile(dev->handle, buf, count, (LPDWORD)&wrote, &dev->overlapped_write) == FALSE) {
		if (!handle_overlapped_result(dev->handle, &dev->overlapped_write, &wrote, timeout)) {
			return -1;
		}
	}

	return wrote;
}

void
serial_discard_input(serial_dev_t *dev)
{
	PurgeComm(dev->handle, PURGE_RXCLEAR | PURGE_RXABORT);
}

void
serial_discard_output(serial_dev_t *dev)
{
	PurgeComm(dev->handle, PURGE_TXCLEAR | PURGE_TXABORT);
}

bool
serial_set_rts(serial_dev_t *dev, bool val)
{
	dev->rts = val;
	if (val) {
		return EscapeCommFunction(dev->handle, SETRTS) != 0;
	} else {
		return EscapeCommFunction(dev->handle, CLRRTS) != 0;
	}
}

bool
serial_set_dtr(serial_dev_t *dev, bool val)
{
	dev->dtr = val;
	if (val) {
		return EscapeCommFunction(dev->handle, SETDTR) != 0;
	} else {
		return EscapeCommFunction(dev->handle, CLRDTR) != 0;
	}
}
