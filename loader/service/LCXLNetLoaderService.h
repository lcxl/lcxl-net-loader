#ifndef _LCXL_NET_LOADER_SERVICE_H_
#define _LCXL_NET_LOADER_SERVICE_H_

#include "../../common/service/lcxl_net_service.h"

class CNetLoaderSer : public CNetServiceBase {
protected:
	virtual void IOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped);
};

#endif