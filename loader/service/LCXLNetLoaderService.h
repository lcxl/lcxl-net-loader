#ifndef _LCXL_NET_LOADER_SERVICE_H_
#define _LCXL_NET_LOADER_SERVICE_H_

#include "resource.h"
#include "../common/dll_interface.h"
#include "../../component/lcxl_iocp/lcxl_iocp_base.h"

TCHAR LCXLSHADOW_SER_NAME[] = _T("LCXLNetLoaderService");

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);  //服务主函数  
void WINAPI ControlHandler(DWORD dwMsg);            //服务控制函数

/// <summary>
/// 服务管理父类
/// </summary>
class SerMgrBase {
private:
	SERVICE_TABLE_ENTRY mServiceTableEntry[2];
	// 服务句柄
	SERVICE_STATUS_HANDLE mServiceStatusHandle;
protected:
	SERVICE_STATUS mSerStatus;
	/// <summary>
	/// 报告服务状态给服务管理器
	/// </summary>
	/// <param name="SerStatus">
	/// 服务状态
	/// </param>
	/// <returns>
	/// 是否成功
	/// </returns>
	BOOL ReportStatusToSCMgr();
	void SerMain(DWORD dwNumServicesArgs, LPTSTR lpServiceArgVectors[]);
	virtual void SerHandler(DWORD dwControl) = 0;
	virtual void SerRun() = 0;
public:
	SerMgrBase();
	virtual ~SerMgrBase();
	/// <summary>
	/// 初始化服务
	/// </summary>
	/// <returns>
	/// 是否成功
	/// </returns>
	virtual BOOL Run();
};

class SerCore : public SerMgrBase{
private:
	//退出事件
	HANDLE mExitEvent;
	CIOCPBaseList *mSerList;
	CIOCPManager *mIOCPMgr;
	CSocketLst *mSockLst;
	void IOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped);
protected:
	virtual void SerHandler(DWORD dwControl);
	virtual void SerRun();
public:
	friend void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
	friend void WINAPI ServiceHandler(DWORD dwControl);
};

#endif