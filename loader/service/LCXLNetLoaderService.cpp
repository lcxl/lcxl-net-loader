// LCXLNetLoaderService.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "LCXLNetLoaderService.h"

//全局变量  
SERVICE_STATUS         g_ser_status;  
SERVICE_STATUS_HANDLE  g_ser_ctrl_handler;  

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	//初始化一个分配表  
	SERVICE_TABLE_ENTRY ServiceTable[] =  
	{  
		{ szServiceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain },  
		{ NULL, NULL } //分派表的最后一项必须是服务名和服务主函数域的 NULL 指针，所以这两项为NULL  
	};  
	if(!StartServiceCtrlDispatcher(ServiceTable))  // 启动服务的控制分派机线程  
	{  
		OutputDebugString(_T("分派机启动服务失败！"));  
	} 
	return 0;
}


void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)   
{  
	int error;  

	//指定服务特征和其当前状态
	g_ser_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;  //服务类型  
	g_ser_status.dwCurrentState = SERVICE_START_PENDING;     //指定服务的当前状态  
	g_ser_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;   //这个成员表示哪些控制通执服务是可接受的  
	g_ser_status.dwWin32ExitCode = 0;                        //这两个域在你终止服务并报告退出细节时很有用。  
	g_ser_status.dwServiceSpecificExitCode = 0;              //初始化服务时并不退出，因此，它们的值为0  
	g_ser_status.dwCheckPoint = 0;                           //这二个成员允许  
	g_ser_status.dwWaitHint = 0;                             //一个服务回报它的进度  

	//注册服务控制  
	g_ser_ctrl_handler = RegisterServiceCtrlHandler(szServiceName, ControlHandler);   

	if (NULL == g_ser_ctrl_handler)  
	{  
		//注册失败就返回  
		OutputDebugString(TEXT("注册服务失败！"));  
		return;   
	}  

	error = InitService(); //初始化数据  

	if (error)   
	{  
		// 初始化失败，终止服务  
		g_ser_status.dwWin32ExitCode = -1; 
		 //向 SCM 报告服务的状态  
		ReporttoSCM(SERVICE_STOPPED);
		return; // 退出 ServiceMain  
	}  

	// 如果初始化成功，向 SCM 报告运行状态     
	ReporttoSCM(SERVICE_RUNNING);
	while (g_ser_status.dwCurrentState == SERVICE_RUNNING)  
	{  
		//这里放你要实现的功能函数  

		MessageBeep(0);  
		Sleep(3000);  
	}  

	return;   

}   

int InitService()  
{  
	//获取系统目录地址，失败就返回-1  
	
	return 0;  
}  

void WINAPI ControlHandler(DWORD dwMsg)   
{   
	switch(dwMsg)   
	{  
	case SERVICE_CONTROL_STOP:   
		//响应停止服务控制  
		g_ser_status.dwWin32ExitCode = 0;
		ReporttoSCM(SERVICE_STOPPED);
		return;   

	default:  
		break;  
	}   
	//向 SCM 报告服务的状态  
	SetServiceStatus(g_ser_ctrl_handler, &g_ser_status);  
	return;   
}  

BOOL ReporttoSCM( DWORD dwCurrentState )
{
	g_ser_status.dwCurrentState = dwCurrentState;   
	return SetServiceStatus(g_ser_ctrl_handler, &g_ser_status);  
}
