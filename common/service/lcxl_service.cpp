#include "lcxl_service.h"
#include <assert.h>

BOOL CServiceBase::Run()
{
	if (m_ServiceTableEntry[0].lpServiceName == NULL) {
		throw std::exception("服务名未指定");
	}
	m_IsCallRunFunc = TRUE;
	return StartServiceCtrlDispatcher(m_ServiceTableEntry);
}

void CServiceBase::SerMain(DWORD dwNumServicesArgs, LPTSTR lpServiceArgVectors[])
{
	// 注册控制
	m_ServiceStatusHandle = RegisterServiceCtrlHandlerEx(m_SerivceName.c_str(), &CServiceBase::HandlerEx, this);
	if (m_ServiceStatusHandle == NULL) {
#ifdef _DEBUG
		OutputDebugString(_T("CServiceBase::SerMain RegisterServiceCtrlHandlerEx failed\n"));
#endif
		return;
	}
	// 报告正在启动
	SetCurrentState(SERVICE_START_PENDING);
	// 报告启动成功
	//SetCurrentState(SERVICE_RUNNING);

	SerRun();
	// 报告服务当前的状态给服务控制管理器
	SetCurrentState(SERVICE_STOP_PENDING);
	SetCurrentState(SERVICE_STOPPED);
}

CServiceBase::CServiceBase() :m_IsCallRunFunc(FALSE)
{
	m_ServiceTableEntry[0].lpServiceName = NULL;
	m_ServiceTableEntry[0].lpServiceProc = &CServiceBase::ServiceMain;
	m_ServiceTableEntry[1].lpServiceName = NULL;
	m_ServiceTableEntry[1].lpServiceProc = NULL;

	m_SerStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	m_SerStatus.dwServiceSpecificExitCode = 0;
	m_SerStatus.dwCheckPoint = 1;
	m_SerStatus.dwWaitHint = 0;
	m_SerStatus.dwWin32ExitCode = 0;

	if (m_GlobalService != NULL) {
		m_GlobalService = this;
	} else {
		throw std::exception("服务类只能实例化一个");
	}
}

CServiceBase::~CServiceBase()
{

}

DWORD CServiceBase::GetCurrentState()
{
	return m_SerStatus.dwCurrentState;
}

void CServiceBase::SetCurrentState(DWORD dwCurrentState)
{
	if (m_SerStatus.dwCurrentState != dwCurrentState) {
		m_SerStatus.dwCurrentState = dwCurrentState;
		if (m_SerStatus.dwCurrentState == SERVICE_START_PENDING || m_SerStatus.dwCurrentState == SERVICE_STOP_PENDING || m_SerStatus.dwCurrentState == SERVICE_CONTINUE_PENDING || m_SerStatus.dwCurrentState == SERVICE_PAUSE_PENDING){
			m_SerStatus.dwControlsAccepted = 0;
			m_SerStatus.dwCheckPoint++;
		} else {
			m_SerStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
			m_SerStatus.dwCheckPoint = 0;
		}
		SetServiceStatus(m_ServiceStatusHandle, &m_SerStatus);
	}
}

DWORD CServiceBase::GetServiceType()
{
	return m_SerStatus.dwServiceType;
}

void CServiceBase::SetServiceType(DWORD dwServiceType)
{
	m_SerStatus.dwServiceType = dwServiceType;
}

DWORD CServiceBase::GetServiceSpecificExitCode()
{
	return m_SerStatus.dwServiceSpecificExitCode;
}

void CServiceBase::SetServiceSpecificExitCode(DWORD dwServiceSpecificExitCode)
{
	m_SerStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;
}

DWORD CServiceBase::GetWaitHint()
{
	return m_SerStatus.dwWaitHint;
}

void CServiceBase::SetWaitHint(DWORD dwWaitHint)
{
	m_SerStatus.dwWaitHint = dwWaitHint;
}

DWORD CServiceBase::GetWin32ExitCode()
{
	return m_SerStatus.dwWin32ExitCode;
}

void CServiceBase::SetWin32ExitCode(DWORD dwWin32ExitCode)
{
	m_SerStatus.dwWin32ExitCode = dwWin32ExitCode;
}

std::tstring CServiceBase::GetSerivceName()
{
	return m_SerivceName;
}

void CServiceBase::SetServiceName(std::tstring szServiceName)
{
	if (!m_IsCallRunFunc) {
		m_SerivceName = szServiceName;
		m_ServiceTableEntry[0].lpServiceName = const_cast<TCHAR*>(szServiceName.c_str());
	} else {
		assert(FALSE);
	}
}

DWORD WINAPI CServiceBase::HandlerEx(_In_ DWORD dwControl, _In_ DWORD dwEventType, _In_ LPVOID lpEventData, _In_ LPVOID lpContext)
{
	reinterpret_cast<CServiceBase *>(lpContext)->SerHandler(dwControl, dwEventType, lpEventData);
	return NO_ERROR;
}

void WINAPI CServiceBase::ServiceMain(DWORD argc, LPTSTR *argv)
{
	m_GlobalService->SerMain(argc, argv);
}

CServiceBase* CServiceBase::m_GlobalService = NULL;
