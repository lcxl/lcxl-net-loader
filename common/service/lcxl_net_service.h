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
	int m_ListenPort;
	virtual void IOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped) = 0;
protected:
	virtual DWORD SerHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData);
	virtual void SerRun();
public:
	CNetServiceBase();
	virtual ~CNetServiceBase();
	void SetExitEvent();
	int GetListenPort();
	void SetListenPort(int Port);

};

#endif