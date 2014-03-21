﻿#ifndef _LCXL_NET_LOADER_SERVICE_H_
#define _LCXL_NET_LOADER_SERVICE_H_

#include "../../common/service/lcxl_net_service.h"
#include "lcxl_config.h"
class CNetLoaderService : public CNetServiceBase {
private:
	CLCXLConfig m_Config;
protected:
	virtual void IOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped);
	virtual bool PreSerRun();
public:
	bool LoadConfigFile(std::string XmlFilePath);
};

#endif