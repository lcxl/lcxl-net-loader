// LCXLNetLoaderService.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "LCXLNetLoaderService.h"
#include "resource.h"
#include "../common/dll_interface.h"
#include "lcxl_func.h"
#include "lcxl_net_code.h"
#include <Ws2tcpip.h>
#include <process.h>

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
	printf("%s\n", "服务开始");
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
	if (!LoadXMLFile()) {
		return false;
	}

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

	//在locker生存周期中锁定
	CCSLocker locker = m_Config.LockinLifeCycle();

	UpdateModuleList();

	//开始进行设置
	std::vector<CONFIG_MODULE>::iterator it;
	for (it = m_Config.ModuleList().begin(); it != m_Config.ModuleList().end(); it++) {
		if (!(*it).isexist) {
			continue;
		}
		//设置模块信息
		SetModuleInfo(&(*it));
	}

	m_NotifyEvent = NULL;
	m_IpInterfaceNotifyEvent = NULL;
	// Use NotifyUnicastIpAddressChange to determine when the address is ready
	if (NO_ERROR != NotifyUnicastIpAddressChange(AF_UNSPEC, &IpAddressChangeEvent, this, FALSE, &m_NotifyEvent)) {
		return false;
	}
	if (NO_ERROR != NotifyIpInterfaceChange(AF_UNSPEC, &IpInterfaceChangeEvent, this, FALSE, &m_IpInterfaceNotifyEvent)) {
		return false;
	}
	
	

	SetServiceName(LCXLSHADOW_SER_NAME);
	SetListenPort(m_Config.GetPort());
	SaveXMLFile();
	return true;
}

void CNetLoaderService::PostSerRun()
{
	if (m_NotifyEvent != NULL) {
		CancelMibChangeNotify2(m_NotifyEvent);
	}

	if (m_IpInterfaceNotifyEvent != NULL) {
		CancelMibChangeNotify2(m_IpInterfaceNotifyEvent);
	}

	//在locker生存周期中锁定
	CCSLocker locker = m_Config.LockinLifeCycle();

	//关闭线程
	std::vector<CONFIG_MODULE>::iterator it;
	for (it = m_Config.ModuleList().begin(); it != m_Config.ModuleList().end(); it++) {
		if ((*it).exit_event != NULL) {
			//激活退出事件
			SetEvent((*it).exit_event);
			//等待线程退出
			WaitForSingleObject((*it).thread_handle, INFINITE);
			//关闭线程句柄
			CloseHandle((*it).thread_handle);
			//删除事件
			CloseHandle((*it).exit_event);
			//相关变量置零
			(*it).exit_event = NULL;
			(*it).thread_handle = NULL;
		}
	}
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
			if (ProcessJsonData(root, ret)) {
				SaveXMLFile();
			}
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
	//在locker生存周期中锁定
	CCSLocker locker = m_Config.LockinLifeCycle();
	switch (code) {
	case JC_MODULE_LIST:
	{
		Json::Value module_list;
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
			
			Json::Value virtual_addr;
			char ipv4[16];
			inet_ntop(AF_INET, const_cast<IN_ADDR*>(&(*it).module.virtual_addr.ipv4), ipv4, sizeof(ipv4) / sizeof(ipv4[0]));
			char ipv6[100];
			inet_ntop(AF_INET6, const_cast<IN6_ADDR*>(&(*it).module.virtual_addr.ipv6), ipv6, sizeof(ipv6) / sizeof(ipv6[0]));

			virtual_addr["status"] = (*it).module.virtual_addr.status;
			virtual_addr["ipv4"] = ipv4;
			virtual_addr["ipv6"] = ipv6;
			module["virtual_addr"] = virtual_addr;

			module_list.append(module);
		}
		ret[JSON_MODULE_LIST] = module_list;
	}
		
		break;
	case JC_SERVER_LIST:
	{
		NET_LUID luid;
		Json::Value server_list;

		luid.Value = root[JSON_MINIPORT_NET_LUID].asInt64();
		status = JS_JSON_DATA_NOT_FOUND;
		if (luid.Value != 0) {
			PCONFIG_MODULE module = m_Config.FindModuleByLuid(luid);
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
					server["comment"] = wstring_to_utf8string(wstring((*sit).comment)).c_str();
					server_list.append(server);
				}
				ret[JSON_SERVER_LIST] = server_list;
			}
		}
	}
		break;
	case JC_SET_VIRTUAL_ADDR:
	{
		NET_LUID miniport_net_luid;
		LCXL_ADDR_INFO addr;

		const Json::Value &virtual_addr = root["virtual_addr"];

		miniport_net_luid.Value = root["miniport_net_luid"].asInt64();
		addr.status = virtual_addr["status"].asInt();
		if (inet_pton(AF_INET, virtual_addr["ipv4"].asCString(), &addr.ipv4) != 1 || inet_pton(AF_INET6, virtual_addr["ipv6"].asCString(), &addr.ipv6) != 1) {
			status = JS_JSON_CODE_IP_FORMAT_INVALID;
			break;
		}
		if (lnlSetVirtualAddr(miniport_net_luid, &addr)){
			PCONFIG_MODULE module = m_Config.FindModuleByLuid(miniport_net_luid);
			if (module) {
				module->module.virtual_addr = addr;
			}
		} else {
			status = JS_FAIL;
		}
	}
		break;

		break;
	default:
		status = JS_JSON_CODE_NOT_FOUND;
		break;
	}

	ret[JSON_CODE] = code;
	ret[JSON_STATUS] = status;
	
	return code == JS_SUCCESS;
}

bool CNetLoaderService::LoadXMLFile()
{
	return m_Config.LoadXMLFile(tstring_to_string(ExtractFilePath(GetAppFilePath())) + CONFIG_FILE_NAME);
}

bool CNetLoaderService::SaveXMLFile()
{
	return m_Config.SaveXMLFile(tstring_to_string(ExtractFilePath(GetAppFilePath())) + CONFIG_FILE_NAME);
}

bool CNetLoaderService::UpdateModuleList()
{
	//获取网卡列表信息
	DWORD module_count = 20;
	std::vector<APP_MODULE> module_list;

	module_list.resize(module_count);
	if (!lnlGetModuleList(&module_list[0], &module_count)) {
		OutputDebugStr(_T("lnlGetModuleList failed.Error Code=%d\n"), GetLastError());
		module_list.resize(module_count);
		return false;
	}
	module_list.resize(module_count);

	//更新到配置文件中
	m_Config.UpdateModuleList(module_list);
	return true;
}


unsigned long CNetLoaderService::SetIpAddress(NET_LUID miniport_net_luid, PSOCKADDR_INET address, UINT8 onlink_prefix_length)
{
	//设置IP地址
	unsigned long status = 0;

	MIB_UNICASTIPADDRESS_ROW ipRow;
	// Initialize the row
	InitializeUnicastIpAddressEntry(&ipRow);
	ipRow.InterfaceLuid = miniport_net_luid;
	ipRow.Address = *address;
	//ipv6,前缀;ipv4:子网掩码长度
	ipRow.OnLinkPrefixLength = onlink_prefix_length;

	status = CreateUnicastIpAddressEntry(&ipRow);
	if (status != NO_ERROR)
	{
		switch (status)
		{
		case ERROR_INVALID_PARAMETER:
			OutputDebugStr(_T("Error: CreateUnicastIpAddressEntry returned ERROR_INVALID_PARAMETER\n"));
			break;
		case ERROR_NOT_FOUND:
			OutputDebugStr(_T("Error: CreateUnicastIpAddressEntry returned ERROR_NOT_FOUND\n"));
			break;
		case ERROR_NOT_SUPPORTED:
			OutputDebugStr(_T("Error: CreateUnicastIpAddressEntry returned ERROR_NOT_SUPPORTED\n"));
			break;
		case ERROR_OBJECT_ALREADY_EXISTS:
			OutputDebugStr(_T("Error: CreateUnicastIpAddressEntry returned ERROR_OBJECT_ALREADY_EXISTS\n"));
			break;
		default:
			//NOTE: Is this case needed? If not, we can remove the ErrorExit() function
			OutputDebugStr(_T("CreateUnicastIpAddressEntry returned error: %d\n"), status);
			break;
		}
	} else {
		OutputDebugStr(_T("CreateUnicastIpAddressEntry succeeded\n"));
	}
	return status;
}

void CALLBACK CNetLoaderService::IpAddressChangeEvent(PVOID callerContext, PMIB_UNICASTIPADDRESS_ROW row, MIB_NOTIFICATION_TYPE notificationType)
{
	CNetLoaderService * service;
	ADDRESS_FAMILY addressFamily;

	service = static_cast<CNetLoaderService *>(callerContext);
	// Ensure that this is the correct notification before setting gCallbackComplete
	// NOTE: Is there a stronger way to do this?
	switch (notificationType) {
	case MibAddInstance:
		OutputDebugStr(_T("NotifyUnicastIpAddressChange received an Add instance\n"));
		addressFamily = (ADDRESS_FAMILY)row->Address.si_family;
		switch (addressFamily) {
		case AF_INET:
			OutputDebugStr(_T("\tAddressFamily: AF_INET\n"));
			break;
		case AF_INET6:
			OutputDebugStr(_T("\tAddressFamily: AF_INET6\n"));
			break;
		default:
			OutputDebugStr(_T("\tAddressFamily: %d\n"), addressFamily);
			break;
		}
		if (addressFamily == AF_INET) {
			OutputDebugStr(_T("IPv4 address:  %s\n"), string_to_tstring(std::string(inet_ntoa(row->Address.Ipv4.sin_addr))).c_str());
		}
		//如果IP地址状态不是正常
		if (row->DadState != IpDadStatePreferred) {

		}
		break;
	case MibDeleteInstance:
		break;
	case MibInitialNotification:

		break;
	case MibParameterNotification:
		break;
	default:
		break;
	}
	return;
}
VOID NETIOAPI_API_ CNetLoaderService::IpInterfaceChangeEvent(_In_ PVOID CallerContext, _In_ PMIB_IPINTERFACE_ROW Row OPTIONAL, _In_ MIB_NOTIFICATION_TYPE NotificationType)
{
	CNetLoaderService * service;

	service = static_cast<CNetLoaderService *>(CallerContext);

	//在locker生存周期中锁定
	CCSLocker locker = service->m_Config.LockinLifeCycle();
	switch (NotificationType) {
	case MibAddInstance:
	{
		service->UpdateModuleList();

		PCONFIG_MODULE module = service->m_Config.FindModuleByLuid(Row->InterfaceLuid);
		if (module == NULL || !module->isexist) {
			break;
		}
		//设置模块信息
		service->SetModuleInfo(module);
			
	}
		break;
	case MibDeleteInstance:
		service->UpdateModuleList();

		break;
	case MibInitialNotification:
	{
		
	}
		break;
	case MibParameterNotification:
		break;
	default:
		break;
	}
}

unsigned CALLBACK CNetLoaderService::RouterVipCheckThread(void * context)
{
	PVIP_CHECK_CONTEXT vip_check;
	OutputDebugStr(_T("CNetLoaderService::RouterVipCheckThread start\n"));
	vip_check = static_cast<PVIP_CHECK_CONTEXT>(context);
	do {
		LCXL_ADDR_INFO virtual_addr;
		IF_INDEX if_index;
		{
			//在locker生存周期中锁定
			CCSLocker locker = vip_check->service->m_Config.LockinLifeCycle();
			PCONFIG_MODULE module = vip_check->service->m_Config.FindModuleByLuid(vip_check->miniport_net_luid);
			if (module == NULL || !module->isexist) {
				continue;
			}
			virtual_addr = module->module.virtual_addr;
			if_index = module->module.miniport_ifindex;
		}
		if (virtual_addr.status & SA_ENABLE_IPV6) {
			SOCKADDR_INET Address = { 0 };

			Address.Ipv6.sin6_family = AF_INET6;
			Address.Ipv6.sin6_addr = virtual_addr.ipv6;

			CheckAndSetVip(vip_check->miniport_net_luid, if_index, &Address, virtual_addr.ipv6_onlink_prefix_length);
		}
		if (virtual_addr.status & SA_ENABLE_IPV4) {
			SOCKADDR_INET Address = { 0 };

			Address.Ipv4.sin_family = AF_INET;
			Address.Ipv4.sin_addr = virtual_addr.ipv4;
			CheckAndSetVip(vip_check->miniport_net_luid, if_index, &Address, virtual_addr.ipv4_onlink_prefix_length);
		}
	} while (WaitForSingleObject(vip_check->exit_event, 1000) == WAIT_TIMEOUT);
	//删除
	delete context;
	OutputDebugStr(_T("CNetLoaderService::RouterVipCheckThread end\n"));
	return 0;
}

void CNetLoaderService::CheckAndSetVip(NET_LUID miniport_net_luid, IF_INDEX ifindex, PSOCKADDR_INET Address, UINT8 onlink_prefix_length)
{
	DWORD resu;
	MIB_UNICASTIPADDRESS_ROW Row = { 0 };
	
	Row.InterfaceLuid = miniport_net_luid;
	Row.Address = *Address;

	//查看本机是否有虚拟IP地址
	resu = GetUnicastIpAddressEntry(&Row);
	if (resu == NO_ERROR) {
		//如果虚拟IP状态不正确，如ip重复等
		if (Row.DadState != IpDadStatePreferred && Row.DadState != IpDadStateTentative) {
			OutputDebugStr(_T("CNetLoaderService::CheckAndSetVip:GetUnicastIpAddressEntry DadState failed, delete VIP\n"));
			//删除虚拟IP
			DeleteUnicastIpAddressEntry(&Row);
		}
	} else {

		//本机不具有虚拟IP地址，则查看局域网中是否有主机拥有了虚拟IP地址
		MIB_IPNET_ROW2 NetRow = { 0 };
		
		NetRow.InterfaceLuid = Row.InterfaceLuid;
		NetRow.Address = Row.Address;

		OutputDebugStr(_T("GetUnicastIpAddressEntry failed(0x%08x)\n"), resu);
		//检查是否虚拟IP地址已经存在于网络中
		resu = GetIpNetEntry2(&NetRow);
		if (resu == NO_ERROR) {
			switch (NetRow.State)
			{
			case NlnsUnreachable://链路无法搜索到
				OutputDebugStr(_T("CNetLoaderService::CheckAndSetVip:GetIpNetEntry2 NlnsUnreachable\n"));
				//虚拟IP地址不存在，抢占虚拟IP地址
				SetIpAddress(NetRow.InterfaceLuid, &NetRow.Address, onlink_prefix_length);
				break;
			case NlnsReachable:
				OutputDebugStr(_T("CNetLoaderService::CheckAndSetVip:GetIpNetEntry2 NlnsReachable\n"));
				//清空邻居缓存，以供下次更快的发现
				FlushIpNetTable2(NetRow.Address.si_family, ifindex);
				break;
			default:
				OutputDebugStr(_T("CNetLoaderService::CheckAndSetVip:GetIpNetEntry2 NetRow.State=%d\n"), NetRow.State);
				break;
			}
		} else {
			OutputDebugStr(_T("GetIpNetEntry2 failed(0x%08x)\n"), resu);
			if (resu == ERROR_NOT_FOUND) {
				OutputDebugStr(_T("CNetLoaderService::CheckAndSetVip:GetIpNetEntry2 ERROR_NOT_FOUND\n"));
				//虚拟IP地址不存在，抢占虚拟IP地址
				SetIpAddress(NetRow.InterfaceLuid, &NetRow.Address, onlink_prefix_length);
			}
		}

	}
}

void CNetLoaderService::SetModuleInfo(PCONFIG_MODULE module)
{
#ifdef _DEBUG
	OutputDebugStr(_T("APP:SetModuleInfo-----------------------------\n"));
	OutputDebugStr(_T("mac_addr=%02x-%02x-%02x-%02x-%02x-%02x\nfilter_module_name=%ws\nminiport_friendly_name=%ws\nminiport_name=%ws\nminiport_net_luid=%I64x\n"),
		module->module.mac_addr.Address[0], module->module.mac_addr.Address[1], module->module.mac_addr.Address[2], module->module.mac_addr.Address[3], module->module.mac_addr.Address[4], module->module.mac_addr.Address[5],
		module->module.filter_module_name,
		module->module.miniport_friendly_name,
		module->module.miniport_name,
		module->module.miniport_net_luid
		);
	OutputDebugStr(_T("APP:-----------------------------\n"));
#endif
	//设置虚拟IP地址
	if (!lnlSetVirtualAddr(module->module.miniport_net_luid, &module->module.virtual_addr)) {
		OutputDebugStr(_T("lnlSetVirtualAddr(%I64d) failed:error code=%d\n"), module->module.miniport_net_luid, GetLastError());
	}
	switch (m_Config.GetRole()) {
	case LCXL_ROLE_ROUTER:
	{
		std::vector<CONFIG_SERVER>::iterator sit;
		for (sit = module->server_list.begin(); sit != module->server_list.end(); sit++) {
			//添加主机
			if (!lnlAddServer(module->module.miniport_net_luid, &(*sit).server)) {
				OutputDebugStr(_T("lnlAddServer(%I64d) failed:error code=%d\n"), module->module.miniport_net_luid, GetLastError());
			}
		}

		if (module->thread_handle == NULL) {
			PVIP_CHECK_CONTEXT context = new VIP_CHECK_CONTEXT();

			context->exit_event = CreateEvent(NULL, TRUE, FALSE, NULL);
			context->service = this;
			context->miniport_net_luid = module->module.miniport_net_luid;

			module->thread_handle = (HANDLE)_beginthreadex(NULL, 0, &RouterVipCheckThread, context, 0, NULL);
			if (module->thread_handle == NULL) {
				CloseHandle(context->exit_event);
				delete context;
			} else {
				module->exit_event = context->exit_event;
			}
		}
	}
		break;
	default:
		break;
	}
}


