#pragma once

#include "resource.h"

TCHAR szServiceName[] = _T("LCXLNetLoaderService");  

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);  //服务主函数  
void WINAPI ControlHandler(DWORD dwMsg);            //服务控制函数
BOOL ReporttoSCM(DWORD dwCurrentState);