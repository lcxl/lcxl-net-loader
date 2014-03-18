#ifndef _LCXL_NET_LOADER_SERVICE_H_
#define _LCXL_NET_LOADER_SERVICE_H_

#include "../../common/service/lcxl_net_service.h"
#include "../../component/tinyxml2/tinyxml2.h"
class CNetLoaderService : public CNetServiceBase {
private:
	tinyxml2::XMLDocument m_doc;
protected:
	virtual void IOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped);
public:
	bool LoadXMLFile(std::string XmlFilePath);
};

#endif