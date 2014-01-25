#ifndef _LCXL_NET_SERVICE_H_
#define _LCXL_NET_SERVICE_H_


#include "../../component/lcxl_iocp/lcxl_iocp_cmd.h"
#include "lcxl_service.h"

class CNetServiceBase : public CServiceBase{
protected:
	//ÍË³öÊÂ¼þ
	HANDLE mExitEvent;
	CIOCPBaseList *mSerList;
	CIOCPManager *mIOCPMgr;
	CSocketLst *mSockLst;
	virtual void IOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped) = 0;
protected:
	virtual void SerHandler(DWORD dwControl);
	virtual void SerRun();
public:
	void SetExitEvent();
};

#endif