#ifndef _LCXL_NET_LOADER_SERVICE_H_
#define _LCXL_NET_LOADER_SERVICE_H_

#include "../../common/service/lcxl_net_service.h"
#include "lcxl_config.h"
#include "../../component/jsoncpp/include/json/json.h"

#define CONFIG_FILE_NAME "lcxlnetloader.xml"
class CNetLoaderService : public CNetServiceBase {
private:
	HANDLE m_NotifyEvent;
	CLCXLConfig m_Config;
	bool LoadXMLFile();
	bool SaveXMLFile();
private:
	unsigned long SetIpAddress(NET_LUID miniport_net_luid, PLCXL_ADDR_INFO virtual_addr);
protected:
	virtual void IOCPEvent(IocpEventEnum EventType, CLLSockObj *SockObj, PIOCPOverlapped Overlapped);
	virtual void RecvEvent(CLLSockObj *SockObj, PIOCPOverlapped Overlapped);
	virtual bool ProcessJsonData(const Json::Value &root, Json::Value &ret);
	virtual bool PreSerRun();
	virtual void PostSerRun();
public:
};

void CALLBACK CallCompleted(PVOID callerContext, PMIB_UNICASTIPADDRESS_ROW row, MIB_NOTIFICATION_TYPE notificationType);

#endif