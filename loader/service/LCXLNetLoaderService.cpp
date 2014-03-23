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
	if (!m_Config.LoadXMLFile(tstring_to_string(ExtractFilePath(GetAppFilePath())) + "lcxlnetloader.xml")) {
		return false;
	}
	SetServiceName(LCXLSHADOW_SER_NAME);
	SetListenPort(m_Config.GetPort());
	DWORD module_count = 20;
	m_ModuleList.resize(module_count);
	if (!lnlGetModuleList(&m_ModuleList[0], &module_count)) {
		OutputDebugStr(_T("lnlGetModuleList failed.Error Code=%d\n"), GetLastError());
		return false;
	}
	m_ModuleList.resize(module_count);
#ifdef _DEBUG
	std::vector<APP_MODULE_INFO>::iterator it;
	for (it = m_ModuleList.begin(); it != m_ModuleList.end(); it++) {
		OutputDebugStr(_T("APP:-----------------------------\n"));
		OutputDebugStr(_T(
"mac_addr=%02x-%02x-%02x-%02x-%02x-%02x\n\
filter_module_name=%ws\n\
miniport_friendly_name=%ws\n\
miniport_name=%ws\n\
miniport_net_luid=%I64x\n"), 
			(*it).mac_addr.Address[0], (*it).mac_addr.Address[1], (*it).mac_addr.Address[2], (*it).mac_addr.Address[3], (*it).mac_addr.Address[4], (*it).mac_addr.Address[5],
			(*it).filter_module_name,
			(*it).miniport_friendly_name,
			(*it).miniport_name,
			(*it).miniport_net_luid
			);
		OutputDebugStr(_T("APP:-----------------------------\n"));
	}
#endif // _DEBUG

	return true;
}
