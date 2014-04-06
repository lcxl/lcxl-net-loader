// LCXLNetLoaderService.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "LCXLNetLoaderService.h"
#include "resource.h"
#include "../common/dll_interface.h"
#include "lcxl_func.h"
#include "lcxl_net_code.h"
#include <Ws2tcpip.h>

TCHAR LCXLSHADOW_SER_NAME[] = _T("LCXLNetLoaderService");


#ifdef LCXL_SHADOW_SER_TEST

#include <conio.h>

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
	OutputDebugStr(_T("sizeof APP_MODULE=%d, sizeof router_mac_addr=%d, sizeof lcxl_addr_info=%d, sizeof CONFIG_SERVER=%d"), sizeof(APP_MODULE), sizeof(IF_PHYSICAL_ADDRESS), sizeof(LCXL_ADDR_INFO), sizeof(CONFIG_SERVER));
	//初始化一个分配表  
	printf("%s\n", "输入任意字符开始");
	_getch();
	
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

void CNetLoaderService::IOCPEvent(IocpEventEnum EventType, CLLSockObj *SockObj, PIOCPOverlapped Overlapped)
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
		RecvEvent(SockObj, Overlapped);
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
		
#ifdef LCXL_SHADOW_SER_TEST
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
#endif
		//lnlAddServer((*it).miniport_net_luid, &server);
	}
	SetServiceName(LCXLSHADOW_SER_NAME);
	SetListenPort(m_Config.GetPort());

	m_Config.SaveXMLFile(tstring_to_string(ExtractFilePath(GetAppFilePath())) + CONFIG_FILE_NAME);
	return true;
}

void CNetLoaderService::RecvEvent(CLLSockObj *SockObj, PIOCPOverlapped Overlapped)
{
	long datalen = SockObj->GetRecvDataLen();
	PVOID data = SockObj->GetRecvData();
	Json::Value ret;

	if (datalen%sizeof(WCHAR)==0) {
		std::wstring datastr_w(PWCHAR(data), datalen / sizeof(WCHAR));
		std::string datastr = wstring_to_string(datastr_w);

		Json::Reader reader;
		Json::Value root;
		
		if (reader.parse(datastr, root)) {
			ProcessJsonData(root, ret);
		}
		
	}
	if (ret.isNull()) {
		ret[JSON_CODE] = JC_NONE;
		ret[JSON_STATUS] = JS_FAIL;
	}
	Json::FastWriter writer;
	std::wstring retstr = string_to_wstring(writer.write(ret));
	SockObj->SendData((PVOID)retstr.c_str(), (ULONG)retstr.size()*sizeof(wchar_t));
}

bool CNetLoaderService::ProcessJsonData(const Json::Value &root, Json::Value &ret)
{
	int code = root[JSON_CODE].asInt();
	int status = JS_SUCCESS;
	Json::Value data;
	
	switch (code) {
	case JC_MODULE_LIST:
	{
		std::vector<CONFIG_MODULE>::iterator it;
		for (it = m_Config.ModuleList().begin(); it != m_Config.ModuleList().end(); it++) {
			Json::Value module;
			
			module["filter_module_name"] = wstring_to_string(wstring((*it).module.filter_module_name)).c_str();
			module["mac_addr"] = string_format(
				"%02x-%02x-%02x-%02x-%02x-%02x",
				(*it).module.mac_addr.Address[0],
				(*it).module.mac_addr.Address[1],
				(*it).module.mac_addr.Address[2],
				(*it).module.mac_addr.Address[3],
				(*it).module.mac_addr.Address[4],
				(*it).module.mac_addr.Address[5]).c_str();
			module["miniport_friendly_name"] = wstring_to_string(wstring((*it).module.miniport_friendly_name)).c_str();
			module["miniport_ifindex"] = (UINT)(*it).module.miniport_ifindex;
			module["miniport_name"] = wstring_to_string(wstring((*it).module.miniport_name)).c_str();
			module["miniport_net_luid"] = (*it).module.miniport_net_luid.Value;
			module["server_count"] = (*it).module.server_count;
			module["router_mac_addr"] = string_format(
				"%02x-%02x-%02x-%02x-%02x-%02x",
				(*it).module.router_mac_addr.Address[0],
				(*it).module.router_mac_addr.Address[1],
				(*it).module.router_mac_addr.Address[2],
				(*it).module.router_mac_addr.Address[3],
				(*it).module.router_mac_addr.Address[4],
				(*it).module.router_mac_addr.Address[5]).c_str();

			Json::Value virtual_addr;
			char ipv4[16];
			inet_ntop(AF_INET, const_cast<IN_ADDR*>(&(*it).module.virtual_addr.ipv4), ipv4, sizeof(ipv4) / sizeof(ipv4[0]));
			char ipv6[100];
			inet_ntop(AF_INET6, const_cast<IN6_ADDR*>(&(*it).module.virtual_addr.ipv6), ipv6, sizeof(ipv6) / sizeof(ipv6[0]));

			virtual_addr["status"] = (*it).module.virtual_addr.status;
			virtual_addr["ipv4"] = ipv4;
			virtual_addr["ipv6"] = ipv6;
			module["virtual_addr"] = virtual_addr;

			data.append(module);
		}
	}
		
		break;
	case JC_SERVER_LIST:
	{
		NET_LUID luid;

		luid.Value= root[JSON_DATA].asInt64();
		status = JS_JSON_DATA_NOT_FOUND;
		if (luid.Value != 0) {
			PCONFIG_MODULE_INFO module = m_Config.FindModuleByLuid(luid);
			if (module) {
				std::vector<CONFIG_SERVER>::iterator sit;
					
				status = JS_SUCCESS;
				for (sit = module->server_list.begin(); sit != module->server_list.end(); sit++) {
					Json::Value server;

					server["status"] = (*sit).server.status;
					server["ip_status"] = (*sit).server.ip_status;
					server["mac_addr"] = string_format(
						"%02x-%02x-%02x-%02x-%02x-%02x",
						(*sit).server.mac_addr.Address[0],
						(*sit).server.mac_addr.Address[1],
						(*sit).server.mac_addr.Address[2],
						(*sit).server.mac_addr.Address[3],
						(*sit).server.mac_addr.Address[4],
						(*sit).server.mac_addr.Address[5]).c_str();
					server["comment"] = wstring_to_string(wstring((*sit).comment)).c_str();
					data.append(server);
				}
			}
		}
	}
		break;
	case JC_SET_VIRTUAL_ADDR:
	{
		NET_LUID miniport_net_luid;
		LCXL_ADDR_INFO addr;

		const Json::Value &data = root[JSON_DATA];
		const Json::Value &virtual_addr = data["virtual_addr"];

		miniport_net_luid.Value = data["miniport_net_luid"].asInt64();
		addr.status = virtual_addr["status"].asInt();
		inet_pton(AF_INET, virtual_addr["ipv4"].asCString(), &addr.ipv4);
		inet_pton(AF_INET6, virtual_addr["ipv6"].asCString(), &addr.ipv6);
		if (lnlSetVirtualAddr(miniport_net_luid, &addr)){
			PCONFIG_MODULE_INFO module = m_Config.FindModuleByLuid(miniport_net_luid);
			if (module) {
				module->module.virtual_addr = addr;
			}
		} else {
			status = JS_FAIL;
		}
	}
		break;
	default:
		status = JS_JSON_CODE_NOT_FOUND;
		break;
	}

	ret[JSON_CODE] = code;
	ret[JSON_STATUS] = status;
	ret[JSON_DATA] = data;
	return code == JS_SUCCESS;
}
