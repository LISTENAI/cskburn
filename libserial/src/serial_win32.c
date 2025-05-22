#include <errno.h>
#include <serial.h>
#include <stdlib.h>
#include <windows.h>

struct _serial_dev_t {
	HANDLE handle;
	OVERLAPPED overlapped_read;
	OVERLAPPED overlapped_write;
	bool rts;
	bool dtr;
};

static int
configure_port(HANDLE handle, DWORD baud_rate)
{
	DCB dcb;

	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	if (GetCommState(handle, &dcb) == 0) {
		return -EIO;
	}

	dcb.BaudRate = baud_rate;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;

	if (SetCommState(handle, &dcb) == 0) {
		return -EIO;
	}

	return 0;
}

int
serial_open(const char *path, serial_dev_t **dev)
{
	char full_path[MAX_PATH];
	int ret;

	if (strncmp(path, "\\\\.\\", 4) == 0) {
		strncpy(full_path, path, MAX_PATH);
	} else {
		snprintf(full_path, MAX_PATH, "\\\\.\\%s", path);
	}

	HANDLE handle = CreateFileA(full_path,
			GENERIC_READ | GENERIC_WRITE,  // Read/Write
			0,  // No Sharing
			NULL,  // No Security
			OPEN_EXISTING,  // Open existing port only
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,  // Overlapped I/O
			NULL);  // Null for Comm Devices

	if (handle == INVALID_HANDLE_VALUE) {
		return -ENODEV;
	}

	ret = configure_port(handle, CBR_115200);
	if (ret != 0) {
		CloseHandle(handle);
		return ret;
	}

	COMMTIMEOUTS timeouts;
	SecureZeroMemory(&timeouts, sizeof(COMMTIMEOUTS));

	if (GetCommTimeouts(handle, &timeouts) == 0) {
		CloseHandle(handle);
		return -EIO;
	}

	timeouts.ReadIntervalTimeout = 1;

	if (SetCommTimeouts(handle, &timeouts) == 0) {
		CloseHandle(handle);
		return -EIO;
	}

	(*dev) = (serial_dev_t *)calloc(1, sizeof(serial_dev_t));
	(*dev)->handle = handle;
	(*dev)->overlapped_read.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	(*dev)->overlapped_write.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

	return 0;
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

int
serial_set_speed(serial_dev_t *dev, uint32_t speed)
{
	int ret;
	DCB dcb;

	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	if (GetCommState(dev->handle, &dcb) == 0) {
		return -EIO;
	}

	dcb.BaudRate = speed;

	if (SetCommState(dev->handle, &dcb) == 0) {
		return -EIO;
	}

	if ((ret = serial_set_rts(dev, dev->rts)) != 0) {
		return ret;
	}

	if ((ret = serial_set_dtr(dev, dev->dtr)) != 0) {
		return ret;
	}

	return 0;
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

static ssize_t
serial_do_read(serial_dev_t *dev, void *buf, size_t count, uint64_t timeout)
{
	DWORD read = 0;

	ResetEvent(dev->overlapped_read.hEvent);

	if (ReadFile(dev->handle, buf, count, &read, &dev->overlapped_read) == FALSE) {
		if (!handle_overlapped_result(dev->handle, &dev->overlapped_read, &read, timeout)) {
			return -ETIMEDOUT;
		}
	}

	return (ssize_t)read;
}

ssize_t
serial_read(serial_dev_t *dev, void *buf, size_t count, uint64_t timeout)
{
	uint64_t start = GetTickCount64();
	size_t total_read = 0;
	bool is_first_attempt = true;

	while (total_read < count) {
		uint64_t elapsed = GetTickCount64() - start;
		if (elapsed >= timeout) {
			break;
		}

		COMSTAT stat;
		if (!ClearCommError(dev->handle, NULL, &stat)) {
			return -EIO;
		}

		if (stat.cbInQue > 0) {
			size_t to_read = min(count - total_read, (size_t)stat.cbInQue);
			ssize_t ret = serial_do_read(dev, (char *)buf + total_read, to_read, timeout - elapsed);

			if (ret < 0) return ret;
			if (ret == 0) continue;

			total_read += ret;
			is_first_attempt = false;
		} else {
			if (!is_first_attempt) {
				return total_read;
			}

			DWORD wait_time = min(10, (DWORD)(timeout - elapsed));
			if (wait_time == 0) break;
			Sleep(wait_time);
		}
	}

	return (total_read > 0) ? total_read : -ETIMEDOUT;
}

ssize_t
serial_write(serial_dev_t *dev, const void *buf, size_t count, uint64_t timeout)
{
	DWORD wrote = 0;

	ResetEvent(dev->overlapped_write.hEvent);

	if (WriteFile(dev->handle, buf, count, &wrote, &dev->overlapped_write) == FALSE) {
		if (!handle_overlapped_result(dev->handle, &dev->overlapped_write, &wrote, timeout)) {
			return -ETIMEDOUT;
		}
	}

	return (ssize_t)wrote;
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

int
serial_set_rts(serial_dev_t *dev, bool val)
{
	dev->rts = val;

	if (EscapeCommFunction(dev->handle, val ? SETRTS : CLRRTS) == 0) {
		return -EIO;
	}

	return 0;
}

int
serial_set_dtr(serial_dev_t *dev, bool val)
{
	dev->dtr = val;

	if (EscapeCommFunction(dev->handle, val ? SETDTR : CLRDTR) == 0) {
		return -EIO;
	}

	return 0;
}

int
serial_config_rts_state(serial_dev_t *dev, serial_rts_state_t state)
{
	DCB dcb;
	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	if (!GetCommState(dev->handle, &dcb)) {
		return -EIO;
	}

	switch (state) {
		case SERIAL_RTS_OFF:
			dcb.fRtsControl = RTS_CONTROL_DISABLE;
			break;
		case SERIAL_RTS_ON:
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
			break;
		case SERIAL_RTS_FLOW:
			dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
			break;
		default:
			return -EINVAL;
	}

	if (!SetCommState(dev->handle, &dcb)) {
		return -EIO;
	}

	return 0;
}

int
serial_config_dtr_state(serial_dev_t *dev, serial_dtr_state_t state)
{
	DCB dcb;
	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	if (!GetCommState(dev->handle, &dcb)) {
		return -EIO;
	}

	switch (state) {
		case SERIAL_DTR_OFF:
			dcb.fDtrControl = DTR_CONTROL_DISABLE;
			break;
		case SERIAL_DTR_ON:
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
			break;
		case SERIAL_DTR_FLOW:
			dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
			break;
		default:
			return -EINVAL;
	}

	if (!SetCommState(dev->handle, &dcb)) {
		return -EIO;
	}

	return 0;
}
