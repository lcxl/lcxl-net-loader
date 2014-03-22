#ifndef _DLL_INTERFACE_H_
#define _DLL_INTERFACE_H_
#include <WinSock2.h>
#include <Ws2ipdef.h>
#include "drv_interface_type.h"
#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI lnlGetAllModule(IN OUT PAPP_MODULE_INFO module_list, IN OUT PDWORD module_list_count);
BOOL WINAPI lnlStartDriver(IN LUID miniport_net_luid);
BOOL WINAPI lnlStopDriver(IN LUID miniport_net_luid);

#ifdef __cplusplus
}
#endif
#endif