#ifndef _LCXL_NET_LOADER_SERVICE_H_
#define _LCXL_NET_LOADER_SERVICE_H_

#include "../../common/service/lcxl_net_service.h"
#include "lcxl_config.h"
#include "../../component/jsoncpp/include/json/json.h"

#define CONFIG_FILE_NAME "lcxlnetloader.xml"
#define NETLOADER_DRIVER_COMPONENT_ID L"LL_NetLoader"
#define NETLOADER_SERVER_NAME _T("NetLoader")
#define NETLOADER_SERVER_DISPLAY_NAME _T("LCXL NetLoader Service")

typedef enum _NET_CFG_OPT {
	NC_INSTALL,
	NC_UNINSTALL
} NET_CFG_OPT, *PNET_CFG_OPT;

class CNetLoaderService : public CNetServiceBase {
private:
	HANDLE m_NotifyEvent;
	HANDLE m_IpInterfaceNotifyEvent;
	CLCXLConfig m_Config;
	bool LoadXMLFile();
	bool SaveXMLFile();	
	bool UpdateModuleList();
protected:
	virtual void IOCPEvent(IocpEventEnum EventType, CLLSockObj *SockObj, PIOCPOverlapped Overlapped);
	virtual void RecvEvent(CLLSockObj *SockObj, PIOCPOverlapped Overlapped);
	virtual bool ProcessJsonData(const Json::Value &root, Json::Value &ret);
	virtual bool PreSerRun();
	virtual void PostSerRun();
protected:
	static unsigned long SetIpAddress(NET_LUID miniport_net_luid, PSOCKADDR_INET address, UINT8 onlink_prefix_length);
	//************************************
	// 简介: 虚拟IP检查线程
	// 返回: unsigned CALLBACK
	// 参数: void * context
	//************************************
	static unsigned  CALLBACK RouterVipCheckThread(void *context);
	static bool CheckAndSetVip(NET_LUID miniport_net_luid, IF_INDEX ifindex, PSOCKADDR_INET Address, UINT8 onlink_prefix_length);
	static void CALLBACK IpAddressChangeEvent(PVOID callerContext, PMIB_UNICASTIPADDRESS_ROW row, MIB_NOTIFICATION_TYPE notificationType);
	static VOID
		NETIOAPI_API_ IpInterfaceChangeEvent(
		_In_ PVOID CallerContext,
		_In_ PMIB_IPINTERFACE_ROW Row OPTIONAL,
		_In_ MIB_NOTIFICATION_TYPE NotificationType
		);
	void SetModuleInfo(PCONFIG_MODULE module);
	//************************************
	// 简介: 
	// 返回: HRESULT
	// 参数: NET_CFG_OPT net_cfg_opt
	// 参数: LPCWSTR pszwInfId
	//************************************
	HRESULT NetCfgSetup(NET_CFG_OPT net_cfg_opt, LPCWSTR pszwInfId);
public:
	bool StartService();
	bool InstallService();
	bool InstallDriver();
	bool Install();
	bool UninstallService();
	bool UninstallDriver();
	bool Uninstall();
};

typedef struct _VIP_CHECK_CONTEXT {
	CNetLoaderService *service;
	NET_LUID miniport_net_luid;//网卡luid
	HANDLE exit_event;//退出事件
} VIP_CHECK_CONTEXT, *PVIP_CHECK_CONTEXT;

#endif