
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#include <vector>
using namespace std;
#include "lcxl_iocp_base.h"

#pragma comment(lib, "Ws2_32.lib")

LPFN_ACCEPTEX g_AcceptEx;
LPFN_GETACCEPTEXSOCKADDRS g_GetAcceptExSockaddrs ;

BOOL SocketObj::Init()
{
	throw std::exception("The method or operation is not implemented.");
}
