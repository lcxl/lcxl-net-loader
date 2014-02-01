#include <tchar.h>
#include <WinSock2.h>
#include <Ws2ipdef.h>
#include <windows.h>
#include <winioctl.h>

#include "../../common/lcxl_type.h"
#include "../common/drv_interface.h"
#include "interface.h"

BOOL WINAPI DllMain(
	_In_  HINSTANCE hinstDLL,
	_In_  DWORD fdwReason,
	_In_  LPVOID lpvReserved
	)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(fdwReason);
	UNREFERENCED_PARAMETER(lpvReserved);
	return TRUE;
}

DWORD WINAPI LoaderGetAllModule(PAPP_MODULE_INFO pModuleList, PDWORD pDataSize) {
	HANDLE file_handle;
	DWORD last_error = ERROR_SUCCESS;
	file_handle = OpenDirver();
	
	if (INVALID_HANDLE_VALUE == file_handle) {
		return GetLastError();
	}
	if (DeviceIoControl(file_handle, IOCTL_LOADER_ALL_APP_MODULE, NULL, 0, pModuleList, *pDataSize, pDataSize, NULL)) {

	} else {
		last_error = GetLastError();
	}
	CloseHandle(file_handle);
	return last_error;
}