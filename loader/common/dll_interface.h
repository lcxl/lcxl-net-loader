#ifndef _DLL_INTERFACE_H_
#define _DLL_INTERFACE_H_
#include <WinSock2.h>
#include <Ws2ipdef.h>
#include "drv_interface_type.h"
#ifdef __cplusplus
extern "C" {
#endif

	//INT WINAPI lnlGetLcxlRole();
	BOOL WINAPI lnlSetLcxlRole(IN NET_LUID miniport_net_luid, IN INT lcxl_role);
	BOOL WINAPI lnlGetModuleList(OUT PAPP_MODULE module_list, IN OUT PDWORD module_list_count);
	BOOL WINAPI lnlSetVirtualAddr(IN NET_LUID miniport_net_luid, IN PLCXL_ADDR_INFO addr);
	//-------------------LCXL_ROLE_ROUTER-------------------
	BOOL WINAPI lnlGetServerList(IN NET_LUID miniport_net_luid, OUT PLCXL_SERVER server_list, IN OUT PDWORD server_list_count);
	BOOL WINAPI lnlAddServer(IN NET_LUID miniport_net_luid, IN PLCXL_SERVER server);
	BOOL WINAPI lnlDelServer(IN NET_LUID miniport_net_luid, IN PIF_PHYSICAL_ADDRESS mac_addr);
	//-------------------LCXL_ROLE_SERVER-------------------
	//BOOL WINAPI lnlSetRouterMacAddr(IN NET_LUID miniport_net_luid, PIF_PHYSICAL_ADDRESS mac_addr);
#ifdef __cplusplus
}
#endif
#endif