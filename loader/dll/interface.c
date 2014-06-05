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
		*lpBytesReturned = 0;
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

BOOL WINAPI lnlRestartModule(IN NET_LUID miniport_net_luid)
{
	DWORD data_size = 0;

	return lnlDeviceIoControl(IOCTL_SET_ROLE, &miniport_net_luid, sizeof(miniport_net_luid), NULL, 0, &data_size);
}

BOOL WINAPI lnlSetLcxlRole(IN NET_LUID miniport_net_luid, IN INT lcxl_role)
{
	DWORD data_size = 0;
	APP_SET_ROLE set_role;

	set_role.miniport_net_luid = miniport_net_luid;
	set_role.lcxl_role = lcxl_role;

	return lnlDeviceIoControl(IOCTL_SET_ROLE, &set_role, sizeof(set_role), NULL, 0, &data_size);
}

BOOL WINAPI lnlGetModuleList(OUT PAPP_MODULE pModuleList, IN OUT PDWORD pModuleListCount)
{
	BOOL resu;
	DWORD data_size = *pModuleListCount*sizeof(APP_MODULE);

	resu = lnlDeviceIoControl(IOCTL_GET_MODULE_LIST, NULL, 0, pModuleList, data_size, &data_size);
	*pModuleListCount = data_size / sizeof(APP_MODULE);
	if (resu) {
		assert(data_size%sizeof(APP_MODULE) == 0);
	} 
	return resu;
}

BOOL WINAPI lnlSetVirtualAddr(IN NET_LUID miniport_net_luid, IN PLCXL_ADDR_INFO addr)
{
	DWORD data_size = 0;
	APP_SET_VIRTUAL_ADDR virtual_ip;

	virtual_ip.miniport_net_luid = miniport_net_luid;
	virtual_ip.addr = *addr;
	
	return lnlDeviceIoControl(IOCTL_SET_VIRTUAL_ADDR, &virtual_ip, sizeof(virtual_ip), NULL, 0, &data_size);
}

BOOL WINAPI lnlGetServerList(IN NET_LUID miniport_net_luid, OUT PLCXL_SERVER server_list, IN OUT PDWORD server_list_count)
{
	BOOL resu;
	DWORD data_size = *server_list_count*sizeof(LCXL_SERVER);

	resu = lnlDeviceIoControl(IOCTL_ROUTER_GET_SERVER_LIST, &miniport_net_luid, sizeof(miniport_net_luid), server_list, data_size, &data_size);
	*server_list_count = data_size / sizeof(LCXL_SERVER);
	if (resu) {
		assert(data_size%sizeof(LCXL_SERVER) == 0);
		
	} 
	return resu;
}
BOOL WINAPI lnlAddServer(IN NET_LUID miniport_net_luid, IN PLCXL_SERVER server)
{
	DWORD data_size = 0;
	APP_ADD_SERVER add_server;

	add_server.miniport_net_luid = miniport_net_luid;
	add_server.server = *server;
	
	return lnlDeviceIoControl(IOCTL_ROUTER_ADD_SERVER, &add_server, sizeof(add_server), NULL, 0, &data_size);
}
BOOL WINAPI lnlDelServer(IN NET_LUID miniport_net_luid, IN PIF_PHYSICAL_ADDRESS mac_addr)
{
	DWORD data_size = 0;
	APP_DEL_SERVER del_server;

	del_server.miniport_net_luid = miniport_net_luid;
	del_server.mac_addr = *mac_addr;
	
	return lnlDeviceIoControl(IOCTL_ROUTER_DEL_SERVER, &del_server, sizeof(del_server), NULL, 0, &data_size);
}

BOOL WINAPI lnlSetServerCheck(IN NET_LUID miniport_net_luid, IN PLCXL_SERVER_CHECK server_check)
{
	DWORD data_size = 0;
	APP_SET_SERVER_CHECK set_server_check;

	set_server_check.miniport_net_luid = miniport_net_luid;
	set_server_check.server_check = *server_check;

	return lnlDeviceIoControl(IOCTL_ROUTER_SET_SERVER_CHECK, &set_server_check, sizeof(set_server_check), NULL, 0, &data_size);
}

BOOL WINAPI lnlSetRoutingAlgorithm(IN NET_LUID miniport_net_luid, IN INT routing_algorithm)
{
	DWORD data_size = 0;
	APP_SET_ROUTING_ALGORITHM set_routing_algorithm;

	set_routing_algorithm.miniport_net_luid = miniport_net_luid;
	set_routing_algorithm.routing_algorithm = routing_algorithm;
	return lnlDeviceIoControl(IOCTL_ROUTER_SET_ROUTING_ALGORITHM, &set_routing_algorithm, sizeof(set_routing_algorithm), NULL, 0, &data_size);
}
