// Should always be the first include
#include <windows.h>

// Should always appear after the #include <windows.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <winreg.h>

#include "usb_info.h"

static bool
GetPortName(HDEVINFO devs, SP_DEVINFO_DATA *dev, LPBYTE buf, DWORD buf_len)
{
	HKEY key = SetupDiOpenDevRegKey(devs, dev, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
	if (key == INVALID_HANDLE_VALUE) {
		return false;
	}

	bool ret = false;
	buf_len -= 1;
	if (RegQueryValueEx(key, "PortName", NULL, NULL, buf, &buf_len) == ERROR_SUCCESS) {
		ret = true;
		buf[buf_len] = '\0';
		return true;
	}

	RegCloseKey(key);
	return ret;
}

static bool
GetHardwareID(HDEVINFO devs, SP_DEVINFO_DATA *dev, LPBYTE buf, DWORD buf_len)
{
	return SetupDiGetDeviceInstanceId(devs, dev, buf, buf_len - 1, NULL) ||
		   SetupDiGetDeviceRegistryProperty(
				   devs, dev, SPDRP_HARDWAREID, NULL, buf, buf_len - 1, NULL);
}

bool
get_usb_info(const char *path, serial_usb_info_t *info)
{
	if (strncmp(path, "\\\\.\\", 4) == 0) {
		path = path + 4;
	}
	if (strncmp(path, "COM", 3) != 0) {
		return false;
	}

	bool ret = false;

	HDEVINFO devs = SetupDiGetClassDevs(NULL, "USB", NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
	if (devs == INVALID_HANDLE_VALUE) {
		return false;
	}

	SP_DEVINFO_DATA dev = {0};
	dev.cbSize = sizeof(SP_DEVINFO_DATA);

	DWORD index = 0;
	BYTE port_name[250] = {0};
	BYTE hardware_id[250] = {0};
	uint32_t vid, pid;
	while (SetupDiEnumDeviceInfo(devs, index, &dev)) {
		index++;

		if (!GetPortName(devs, &dev, port_name, sizeof(port_name))) {
			continue;
		}

		if (strcmp(path, port_name) != 0) {
			continue;
		}

		if (!GetHardwareID(devs, &dev, hardware_id, sizeof(hardware_id))) {
			continue;
		}

		if (sscanf(hardware_id, "USB\\VID_%04X&PID_%04X", &vid, &pid) == 2) {
			info->vendor_id = vid;
			info->product_id = pid;
			ret = true;
			break;
		}
	}

	if (devs) {
		SetupDiDestroyDeviceInfoList(devs);
	}

	return ret;
}
