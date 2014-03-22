#include <tchar.h>
#include <WinSock2.h>
#include <Ws2ipdef.h>
#include <windows.h>
#include <winioctl.h>
#include <assert.h>
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

BOOL WINAPI lnlGetAllModule(IN OUT PAPP_MODULE_INFO pModuleList, IN OUT PDWORD pModuleListCount)
{
	HANDLE file_handle;
	DWORD last_error = ERROR_SUCCESS;
	DWORD data_size = *pModuleListCount*sizeof(APP_MODULE_INFO);

	file_handle = OpenDirver();
	
	if (INVALID_HANDLE_VALUE == file_handle) {
		return FALSE;
	}
	if (DeviceIoControl(file_handle, IOCTL_LOADER_ALL_APP_MODULE, NULL, 0, pModuleList, data_size, &data_size, NULL)) {
		assert(data_size%sizeof(APP_MODULE_INFO) == 0);
		*pModuleListCount = data_size/sizeof(APP_MODULE_INFO);
	} else {
		last_error = GetLastError();
	}
	CloseHandle(file_handle);
	return last_error != ERROR_SUCCESS;
}

BOOL WINAPI lnlStartDriver(IN LUID miniport_net_luid)
{
	HANDLE file_handle;
	DWORD last_error = ERROR_SUCCESS;
	DWORD data_size = 0;

	file_handle = OpenDirver();
	if (INVALID_HANDLE_VALUE == file_handle) {
		return FALSE;
	}
	if (DeviceIoControl(file_handle, IOCTL_LOADER_START_DRIVER, &miniport_net_luid, sizeof(miniport_net_luid), NULL, 0, &data_size, NULL)) {

	} else {
		last_error = GetLastError();
	}
	CloseHandle(file_handle);
	return last_error != ERROR_SUCCESS;
}

BOOL WINAPI lnlStopDriver(IN LUID miniport_net_luid)
{
	HANDLE file_handle;
	DWORD last_error = ERROR_SUCCESS;
	DWORD data_size = 0;

	file_handle = OpenDirver();
	if (INVALID_HANDLE_VALUE == file_handle) {
		return FALSE;
	}
	if (DeviceIoControl(file_handle, IOCTL_LOADER_STOP_DRIVER, &miniport_net_luid, sizeof(miniport_net_luid), NULL, 0, &data_size, NULL)) {
		
	} else {
		last_error = GetLastError();
	}
	CloseHandle(file_handle);
	return last_error != ERROR_SUCCESS;
}