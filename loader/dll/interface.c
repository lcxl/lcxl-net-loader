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

BOOL lnlDeviceIoControl(_In_ DWORD dwIoControlCode,
	_In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
	_In_ DWORD nInBufferSize,
	_Out_writes_bytes_to_opt_(nOutBufferSize, *lpBytesReturned) LPVOID lpOutBuffer,
	_In_ DWORD nOutBufferSize,
	_Out_opt_ LPDWORD lpBytesReturned)
{
	HANDLE file_handle;
	DWORD last_error = ERROR_SUCCESS;

	file_handle = OpenDirver();

	if (INVALID_HANDLE_VALUE == file_handle) {
		return FALSE;
	}
	if (DeviceIoControl(file_handle, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, NULL)) {

	} else {
		last_error = GetLastError();
	}

	CloseHandle(file_handle);
	SetLastError(last_error);
	return last_error == ERROR_SUCCESS;
}

INT WINAPI lnlGetLcxlRole()
{
	INT resu;
	DWORD data_size = 0;

	lnlDeviceIoControl(IOCTL_GET_ROLE, NULL, 0, &resu, sizeof(resu), &data_size);
	return resu;
}

BOOL WINAPI lnlSetLcxlRole(IN INT lcxl_role)
{
	DWORD data_size = 0;

	return lnlDeviceIoControl(IOCTL_SET_ROLE, &lcxl_role, sizeof(lcxl_role), NULL, 0, &data_size);
}

BOOL WINAPI lnlGetModuleList(OUT PAPP_MODULE_INFO pModuleList, IN OUT PDWORD pModuleListCount)
{
	BOOL resu;
	DWORD data_size = *pModuleListCount*sizeof(APP_MODULE_INFO);

	resu = lnlDeviceIoControl(IOCTL_GET_MODULE_LIST, NULL, 0, pModuleList, data_size, &data_size);
	if (resu) {
		assert(data_size%sizeof(APP_MODULE_INFO) == 0);
		*pModuleListCount = data_size/sizeof(APP_MODULE_INFO);
	} 
	return resu;
}

BOOL WINAPI lnlSetVirtualIP(IN NET_LUID miniport_net_luid, IN LCXL_ADDR_INFO addr)
{
	DWORD data_size = 0;
	APP_VIRTUAL_IP virtual_ip;

	virtual_ip.miniport_net_luid = miniport_net_luid;
	virtual_ip.addr = addr;
	
	return lnlDeviceIoControl(IOCTL_LOADER_SET_VIRTUAL_IP, &virtual_ip, sizeof(virtual_ip), NULL, 0, &data_size);
}

BOOL WINAPI lnlGetServerList(IN NET_LUID miniport_net_luid, OUT PLCXL_SERVER_INFO server_list, IN OUT PDWORD server_list_count)
{
	BOOL resu;
	DWORD data_size = *server_list_count*sizeof(LCXL_SERVER_INFO);

	resu = lnlDeviceIoControl(IOCTL_LOADER_GET_SERVER_LIST, &miniport_net_luid, sizeof(miniport_net_luid), server_list, data_size, &data_size);
	if (resu) {
		assert(data_size%sizeof(LCXL_SERVER_INFO) == 0);
		*server_list_count = data_size / sizeof(LCXL_SERVER_INFO);
	} 
	return resu;
}
BOOL WINAPI lnlAddServer(IN NET_LUID miniport_net_luid, IN PLCXL_SERVER_INFO server)
{
	DWORD data_size = 0;
	APP_ADD_SERVER add_server;

	add_server.miniport_net_luid = miniport_net_luid;
	add_server.server = *server;
	
	return lnlDeviceIoControl(IOCTL_LOADER_ADD_SERVER, &add_server, sizeof(add_server), NULL, 0, &data_size);
}
BOOL WINAPI lnlDelServer(IN NET_LUID miniport_net_luid, IN PIF_PHYSICAL_ADDRESS mac_addr)
{
	DWORD data_size = 0;
	APP_DEL_SERVER del_server;

	del_server.miniport_net_luid = miniport_net_luid;
	del_server.mac_addr = *mac_addr;
	
	return lnlDeviceIoControl(IOCTL_LOADER_DEL_SERVER, &del_server, sizeof(del_server), NULL, 0, &data_size);
}