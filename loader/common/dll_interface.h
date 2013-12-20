#ifndef _DLL_INTERFACE_H_
#define _DLL_INTERFACE_H_
#include <WinSock2.h>
#include <Ws2ipdef.h>
#include "drv_interface_type.h"
#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI LoaderGetAllModule(PAPP_MODULE_INFO pModuleList, PDWORD pDataSize);

#ifdef __cplusplus
}
#endif
#endif