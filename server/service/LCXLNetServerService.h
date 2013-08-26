#ifndef _LCXL_NET_LOADER_SERVICE_H_
#define _LCXL_NET_LOADER_SERVICE_H_

#include "resource.h"

TCHAR szServiceName[] = _T("LCXLNetServerService");  

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);  //服务主函数  
void WINAPI ControlHandler(DWORD dwMsg);            //服务控制函数
BOOL ReporttoSCM(DWORD dwCurrentState);
int InitService();

#endif