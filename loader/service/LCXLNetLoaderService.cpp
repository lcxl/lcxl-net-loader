// LCXLNetLoaderService.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "LCXLNetLoaderService.h"
#include "resource.h"
#include "../common/dll_interface.h"
#include "lcxl_func.h"

TCHAR LCXLSHADOW_SER_NAME[] = _T("LCXLNetLoaderService");


#ifdef LCXL_SHADOW_SER_TEST

class CTestNetLoaderSer : public CNetLoaderService {
public:
	virtual BOOL Run() {
		SerRun();
		return TRUE;
	}
};
//全局变量  
CTestNetLoaderSer g_NetLoadSer;

int _tmain(int argc, _TCHAR* argv[]) 
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	SetErrorMode(SEM_FAILCRITICALERRORS);//使程序出现异常时不报错
	//初始化一个分配表  
	printf("%s\n", "在回车之后开始执行");
	char test[20];
	scanf("%s", test);
	
	g_NetLoadSer.Run();
	return 0;
}
#else

//全局变量  
CNetLoaderService g_NetLoadSer;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);
	SetErrorMode(SEM_FAILCRITICALERRORS);//使程序出现异常时不报错;
	//初始化一个分配表  
	g_NetLoadSer.Run();
	return 0;
}

#endif

void CNetLoaderService::IOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped)
{
	vector<CSocketObj*> *SockList;
	switch (EventType) {
	case ieAddSocket:
		SockList = mSerList->GetSockObjList();
		OutputDebugStr(_T("SerCore::IOCPEvent ieAddSocket %d\n"), SockList->size());
		break;
	case ieDelSocket:
		SockList = mSerList->GetSockObjList();
		OutputDebugStr(_T("SerCore::IOCPEvent ieDelSocket %d\n"), SockList->size());
		break;
	case ieCloseSocket:
		SockList = mSerList->GetSockObjList();
		OutputDebugStr(_T("SerCore::IOCPEvent ieCloseSocket %d\n"), SockList->size());
		break;
	case ieError:
		break;
	case ieRecvPart:
		break;
	case ieRecvAll:
		break;
	case ieRecvFailed:
		break;
	case ieSendPart:
		break;
	case ieSendAll:
		break;
	case ieSendFailed:
		break;
	default:
		break;
	}
}

bool CNetLoaderService::PreSerRun()
{
	//加载配置文件
	if (!m_Config.LoadXMLFile(tstring_to_string(ExtractFilePath(GetAppFilePath())) + CONFIG_FILE_NAME)) {
		return false;
	}
	//获取网卡列表信息
	DWORD module_count = 20;
	std::vector<APP_MODULE> module_list;

	module_list.resize(module_count);
	if (!lnlGetModuleList(&module_list[0], &module_count)) {
		OutputDebugStr(_T("lnlGetModuleList failed.Error Code=%d\n"), GetLastError());
		module_list.resize(module_count);
		//return false;
	}
	module_list.resize(module_count);
	//更新到配置文件中
	m_Config.UpdateModuleList(module_list);
	//设置
	//获取角色信息
	INT sys_lcxl_role = lnlGetLcxlRole();
	if (sys_lcxl_role != m_Config.GetRole()) {
		if (sys_lcxl_role != LCXL_ROLE_UNKNOWN) {
			lnlSetLcxlRole(LCXL_ROLE_UNKNOWN);
		}
		if (m_Config.GetRole() != LCXL_ROLE_UNKNOWN) {
			lnlSetLcxlRole(m_Config.GetRole());
		}
	}
	//开始进行设置
	std::vector<CONFIG_MODULE>::iterator it;
	for (it = m_Config.ModuleList().begin(); it != m_Config.ModuleList().end(); it++) {
#ifdef _DEBUG
		OutputDebugStr(_T("APP:-----------------------------\n"));
		OutputDebugStr(_T(
"mac_addr=%02x-%02x-%02x-%02x-%02x-%02x\n\
filter_module_name=%ws\n\
miniport_friendly_name=%ws\n\
miniport_name=%ws\n\
miniport_net_luid=%I64x\n"), 
(*it).module.mac_addr.Address[0], (*it).module.mac_addr.Address[1], (*it).module.mac_addr.Address[2], (*it).module.mac_addr.Address[3], (*it).module.mac_addr.Address[4], (*it).module.mac_addr.Address[5],
(*it).module.filter_module_name,
(*it).module.miniport_friendly_name,
(*it).module.miniport_name,
(*it).module.miniport_net_luid
			);
		OutputDebugStr(_T("APP:-----------------------------\n"));
#endif
		switch (sys_lcxl_role) {
		case LCXL_ROLE_ROUTER:
		{
			vector<LCXL_SERVER> server_list;
			DWORD server_list_count = 100;
			server_list.resize(server_list_count);
			if (!lnlGetServerList((*it).module.miniport_net_luid, &server_list[0], &server_list_count)) {
				server_list.resize(server_list_count);
			}
			server_list.resize(server_list_count);
		}
			break;
		case LCXL_ROLE_SERVER:

			break;
		}
		

		//启用虚拟IPv6
		(*it).module.virtual_addr.status = SA_ENABLE_IPV6;
		lnlSetVirtualAddr((*it).module.miniport_net_luid, &(*it).module.virtual_addr);


		CONFIG_SERVER server;

		server.server.status = SS_ONLINE;
		server.server.ip_status = SA_ENABLE_IPV6;
		wcscpy_s(server.comment, L"测试用");
		server.server.mac_addr.Length = 6;
		server.server.mac_addr.Address[0] = 0x00;
		server.server.mac_addr.Address[1] = 0x0C;
		server.server.mac_addr.Address[2] = 0x29;
		server.server.mac_addr.Address[3] = 0x6F;
		server.server.mac_addr.Address[4] = 0x05;
		server.server.mac_addr.Address[5] = 0xCE;

		(*it).server_list.push_back(server);

		//lnlAddServer((*it).miniport_net_luid, &server);
	}
	SetServiceName(LCXLSHADOW_SER_NAME);
	SetListenPort(m_Config.GetPort());

	


	m_Config.SaveXMLFile(tstring_to_string(ExtractFilePath(GetAppFilePath())) + CONFIG_FILE_NAME);
	return true;
}
