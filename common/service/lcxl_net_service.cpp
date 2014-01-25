#include "lcxl_net_service.h"

void CNetServiceBase::SerHandler(DWORD dwControl)
{
	// Handle the requested control code.
	switch (dwControl){
	case SERVICE_CONTROL_STOP:case SERVICE_CONTROL_SHUTDOWN:
		// 关闭服务
		OutputDebugStr(_T("服务端接收到关闭命令\n"));
		SetExitEvent();
		break;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	case SERVICE_CONTROL_PAUSE:
		break;
	case SERVICE_CONTROL_CONTINUE:
		break;
		// invalid control code
	default:
		// update the service status.
		SetCurrentState(GetCurrentState());
		break;
	}
}

void CNetServiceBase::SerRun()
{
	mIOCPMgr = new CIOCPManager();
	mSerList = new CIOCPBaseList(mIOCPMgr);

	DOnIOCPBaseEvent iocp_event(this, reinterpret_cast<EOnIOCPBaseEvent>(&CNetServiceBase::IOCPEvent));
	mSerList->SetIOCPEvent(iocp_event);

	mExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	mSockLst = new CSocketLst();
	// 启动监听
	if (mSockLst->StartListen(mSerList, 9999)) {
		//
		SetCurrentState(SERVICE_RUNNING);
		// 等待退出
		WaitForSingleObject(mExitEvent, INFINITE);
		mSockLst->Close();
	} else {
		OutputDebugStr(_T("启动监听失败！\n"));
		delete mSockLst;
	}

	CloseHandle(mExitEvent);
	delete mSerList;
	delete mIOCPMgr;
}

void CNetServiceBase::SetExitEvent()
{
	SetEvent(mExitEvent);
}
