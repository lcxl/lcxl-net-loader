#include "lcxl_iocp_base.h"
#include <stdlib.h>
#include <mstcpip.h>

#include <ws2tcpip.h>
#include <mswsock.h>
#include <process.h> 

#pragma comment(lib, "Ws2_32.lib")

static LPFN_ACCEPTEX g_AcceptEx;
static LPFN_GETACCEPTEXSOCKADDRS g_GetAcceptExSockaddrs;

#ifdef _DEBUG
void OutputDebugStr(const TCHAR fmt[], ...)
{
	va_list argptr;
	PTCHAR buf;
	va_start(argptr, fmt);
	int bufsize = _vsntprintf(NULL, 0, fmt, argptr) + 2;
	buf = (PTCHAR)malloc(bufsize*sizeof(TCHAR));
	_vsntprintf(buf, bufsize, fmt, argptr);
	OutputDebugString(buf);
	free(buf);
}
#endif // _DEBUG


/************************************************************************/
/* IOCP工作线程                                                         */
/************************************************************************/
static unsigned __stdcall IocpWorkThread(void *CompletionPortID)
{
	HANDLE CompletionPort = (HANDLE)CompletionPortID;

	while (TRUE) {
		BOOL FIsSuc;
		PIOCPOverlapped FIocpOverlapped;
		CSocketBase *SockBase;
		CSocketObj *SockObj = NULL;
		CSocketLst *SockLst = NULL;
		BOOL _IsSockObj = FALSE;
		DWORD BytesTransferred;
		INT resuInt;

		FIsSuc = GetQueuedCompletionStatus(CompletionPort, &BytesTransferred,
			(PULONG_PTR)&SockBase, (LPOVERLAPPED*)&FIocpOverlapped, INFINITE);
		if (SockBase != NULL) {
			assert(SockBase == FIocpOverlapped->AssignedSockObj);
			_IsSockObj = SockBase->mSocketType == STObj;
			if (_IsSockObj) {
				SockObj = static_cast<CSocketObj*>(SockBase);
			} else {
				SockLst = static_cast<CSocketLst*>(SockBase);
			}
		} else {
			// IOCP 线程退出指令
			// 退出
			OutputDebugStr(_T("获得退出命令，退出并命令下一线程退出。\n"));
			// 通知下一个工作线程退出
			PostQueuedCompletionStatus(CompletionPort, 0, 0, NULL);
			break;
		}
		if (FIsSuc) {
			// 如果是退出线程消息，则退出
			BOOL _NeedContinue = FALSE;
			SOCKET tmpSock;
			PCSocketObj _NewSockObj;

			switch (FIocpOverlapped->OverlappedType) {
			case otRecv:case otSend:
				if (BytesTransferred == 0) {
					assert(FIocpOverlapped == SockObj->GetAssignedOverlapped());
					OutputDebugStr(_T("socket(%d)已关闭:%d\n"), SockObj->GetSocket(), WSAGetLastError());
					// 减少引用
					SockObj->InternalDecRefCount();
					// 继续
					_NeedContinue = TRUE;
					break;
				}
				// socket事件
				switch (FIocpOverlapped->OverlappedType){
				case otRecv:
					assert(FIocpOverlapped == SockObj->GetAssignedOverlapped());
					// 移动当前接受的指针
					FIocpOverlapped->RecvDataLen = BytesTransferred;
					FIocpOverlapped->RecvData = SockObj->GetRecvBuf();
					// 获取事件指针
					// 发送结果
					// 产生事件
					try{
						SockObj->GetOwner()->OnIOCPEvent(ieRecvAll, SockObj, FIocpOverlapped);
					}
					catch (...) {
						OutputDebugStr(_T("SockObj->GetOwner()->OnIOCPEvent ieRecvAll throw an exception\n"));
					}
					// 投递下一个WSARecv
					if (!SockObj->WSARecv()) {
						// 如果出错
						OutputDebugStr(_T("WSARecv函数出错socket=%d:%d\n"), SockObj->GetSocket(), WSAGetLastError());
						// 减少引用
						SockObj->InternalDecRefCount();
					}
					break;
				case otSend:
					// 获取事件指针
					// 数据指针后移
					FIocpOverlapped->CurSendData = (PVOID)((PBYTE)FIocpOverlapped->CurSendData + BytesTransferred);
					// 如果已全部发送完成，释放内存
					if ((ULONG_PTR)FIocpOverlapped->CurSendData -
						(ULONG_PTR)FIocpOverlapped->SendData == (ULONG_PTR)FIocpOverlapped->SendDataLen) {
						BOOL _NeedDecSockObj;

						// 触发事件
						try{
							SockObj->GetOwner()->OnIOCPEvent(ieSendAll, SockObj, FIocpOverlapped);
						}
						catch (...) {
							OutputDebugStr(_T("SockObj->GetOwner()->OnIOCPEvent ieSendAll throw an exception\n"));
						}
						SockObj->GetOwner()->GetOwner()->DelOverlapped(FIocpOverlapped);
						// 获取待发送的数据
						FIocpOverlapped = NULL;
						SockObj->GetOwner()->Lock();
						assert(SockObj->GetIsSending());
						if (SockObj->GetSendDataQueue().size() > 0) {
							FIocpOverlapped = SockObj->GetSendDataQueue().front();
							SockObj->GetSendDataQueue().pop();
							OutputDebugStr(_T("Socket(%d)取出待发送数据\n"), SockObj->GetSocket());
						} else {
							SockObj->SetIsSending(FALSE);
						}
						SockObj->GetOwner()->Unlock();
						// 默认减少Socket引用
						_NeedDecSockObj = TRUE;
						if (FIocpOverlapped != NULL) {
							if (!SockObj->WSASend(FIocpOverlapped)) {
								// 如果有错误
								OutputDebugStr(_T("IocpWorkThread:WSASend函数失败(socket=%d):%d\n"), SockObj->GetSocket(), WSAGetLastError());
								// 触发事件
								try{
									SockObj->GetOwner()->OnIOCPEvent(ieSendFailed, SockObj, FIocpOverlapped);
								}
								catch (...) {
									OutputDebugStr(_T("SockObj->GetOwner()->OnIOCPEvent ieSendFailed throw an exception\n"));
								}
								SockObj->GetOwner()->GetOwner()->DelOverlapped(FIocpOverlapped);
							} else {
								// 发送成功，不减少引用
								_NeedDecSockObj = FALSE;
							}
						}
						if (_NeedDecSockObj) {
							// 减少引用
							SockObj->InternalDecRefCount();
						}
					} else {
						// 没有全部发送完成
						FIocpOverlapped->DataBuf.len = FIocpOverlapped->SendDataLen +
							(ULONG)((ULONG_PTR)FIocpOverlapped->SendData -
							(ULONG_PTR)FIocpOverlapped->CurSendData);
						FIocpOverlapped->DataBuf.buf = (CHAR *)FIocpOverlapped->CurSendData;
						try{
							SockObj->GetOwner()->OnIOCPEvent(ieSendPart, SockObj, FIocpOverlapped);
						}
						catch (...) {
							OutputDebugStr(_T("SockObj->GetOwner()->OnIOCPEvent ieSendAll throw an exception\n"));
						}
						// 继续投递WSASend
						if (!SockObj->WSASend(FIocpOverlapped)) {
							// 如果有错误
							OutputDebugStr(_T("IocpWorkThread:WSASend函数失败(socket=%d):%d\n"), SockObj->GetSocket(), WSAGetLastError());
							// 触发事件
							try{
								SockObj->GetOwner()->OnIOCPEvent(ieSendFailed, SockObj, FIocpOverlapped);
							}
							catch (...) {
								OutputDebugStr(_T("SockObj->GetOwner()->OnIOCPEvent ieSendFailed throw an exception\n"));
							}
							SockObj->GetOwner()->GetOwner()->DelOverlapped(FIocpOverlapped);
							// 减少引用
							SockObj->InternalDecRefCount();
						}
					}
					break;


				default:
					break;
				}
				break;
			case otListen:
				assert(FIocpOverlapped == SockLst->GetAssignedOverlapped());
				/*
				GetAcceptExSockaddrs(SockLst->mLstBuf, 0, sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16, local, localLen, remote, remoteLen);
				*/
				tmpSock = SockLst->GetSocket();
				// 更新上下文
				resuInt = setsockopt(FIocpOverlapped->AcceptSocket, SOL_SOCKET,
					SO_UPDATE_ACCEPT_CONTEXT, (char *)&tmpSock, sizeof(tmpSock));
				if (resuInt != 0) {
					OutputDebugStr(_T("socket(%d)设置setsockopt失败:%d\n"),
						FIocpOverlapped->AcceptSocket, WSAGetLastError());
				}
				// 监听
				// 产生事件，添加SockObj，如果失败，则close之
				_NewSockObj = NULL;
				// 创建新的SocketObj类
				SockLst->CreateSockObj(_NewSockObj);
				// 填充Socket句柄
				_NewSockObj->mSock = FIocpOverlapped->AcceptSocket;
				// 设置为服务socket
				_NewSockObj->mIsSerSocket = TRUE;
				// 添加到Socket列表中
				SockLst->GetOwner()->AddSockBase(_NewSockObj);
				// 投递下一个Accept端口
				if (!SockLst->Accept()){
					OutputDebugStr(_T("AcceptEx函数失败: %d\n"), WSAGetLastError());
					SockLst->InternalDecRefCount();
				}
				break;
			}
			if (_NeedContinue) {
				continue;
			}
		} else {
			if (FIocpOverlapped != NULL) {
				OutputDebugStr(_T("GetQueuedCompletionStatus函数失败(socket=%d): %d\n"),
					SockBase->GetSocket(), GetLastError());
				// 关闭
				if (FIocpOverlapped != SockBase->GetAssignedOverlapped()) {
					// 只有otSend的FIocpOverlapped
					assert(FIocpOverlapped->OverlappedType == otSend);
					SockBase->GetOwner()->GetOwner()->DelOverlapped(FIocpOverlapped);
				}
				// 减少引用
				SockBase->InternalDecRefCount();
			} else {
				OutputDebugStr(_T("GetQueuedCompletionStatus函数失败: %d\n"), GetLastError());
			}
		}
	}
	_endthreadex(0);
	return 0;
}


int CSocketBase::InternalIncRefCount(int Count/*=1*/, BOOL UserMode/*=FALSE*/)
{
	int resu;
	mOwner->Lock();
	mRefCount += Count;
	if (UserMode) {
		mUserRefCount += Count;
		resu = mUserRefCount;
	} else {
		resu = mRefCount;
	}
	mOwner->Unlock();
	assert(resu > 0);
	return resu;
}

int CSocketBase::InternalDecRefCount(int Count/*=1*/, BOOL UserMode/*=FALSE*/)
{
	// socket是否关闭
	BOOL _IsSocketClosed1;
	BOOL _IsSocketClosed2;
	BOOL _CanFree;
	int resu;
	mOwner->Lock();
	_IsSocketClosed1 = mRefCount == mUserRefCount;
	mRefCount -= Count;
	if (UserMode) {
		mUserRefCount -= Count;
		resu = mUserRefCount;
	} else
	{
		resu = mRefCount;
	}
	_IsSocketClosed2 = mRefCount == mUserRefCount;
	_CanFree = 0 == mRefCount;
	mOwner->Unlock();
	// socket已经关闭
	if (!_IsSocketClosed1 && _IsSocketClosed2) {
		// 触发close事件
		if (this->mSocketType == STObj) {
			mOwner->OnIOCPEvent(ieCloseSocket, static_cast<CSocketObj*>(this), NULL);
		} else {
			mOwner->OnListenEvent(leCloseSockLst, static_cast<CSocketLst*>(this));
		}
	}
	if (_CanFree){
		// 移除自身，并且释放
		mOwner->RemoveSockBase(this);
	}
	return resu;
}

CSocketBase::CSocketBase()
{
	mSock = INVALID_SOCKET;
	// 引用计数默认为1
	mRefCount = 0;
	// 用户计数默认为0
	mUserRefCount = 0;

	mIniteStatus = sisInitializing;
	mOwner = NULL;
	mIOComp = INVALID_HANDLE_VALUE;
	mAssignedOverlapped = NULL;
	mTag = 0;
}

CSocketBase::~CSocketBase()
{
	if (mAssignedOverlapped != NULL) {
		assert(mOwner != NULL);
		assert(mOwner->GetOwner() != NULL);
		mOwner->GetOwner()->DelOverlapped(mAssignedOverlapped);
	}
}

void CSocketBase::Close()
{
	shutdown(mSock, SD_BOTH);
	if (closesocket(mSock) != ERROR_SUCCESS) {
		OutputDebugStr(_T("closesocket failed:%d\n"), WSAGetLastError());
	}
	mSock = INVALID_SOCKET;
}

int CSocketBase::IncRefCount(int Count/*=1*/)
{
	assert(Count > 0);
	return InternalIncRefCount(Count, TRUE);
}

int CSocketBase::DecRefCount(int Count/*=1*/)
{
	assert(Count > 0);
	if (mUserRefCount == 0) {
		throw invalid_argument("IncRefCount function must be called before call this function!");
	}
	return InternalDecRefCount(Count, TRUE);
}


BOOL CSocketLst::Accept()
{
	DWORD BytesReceived;
	BOOL resu;

	assert(mAssignedOverlapped != NULL);
	assert(mAssignedOverlapped->OverlappedType == otListen);
	ZeroMemory(&mAssignedOverlapped->lpOverlapped, sizeof(mAssignedOverlapped->lpOverlapped));
	mAssignedOverlapped->AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
		WSA_FLAG_OVERLAPPED);

	resu = (g_AcceptEx(mSock, mAssignedOverlapped->AcceptSocket, mLstBuf, 0,
		sizeof(sockaddr_storage)+16, sizeof(sockaddr_storage)+16, &BytesReceived,
		&mAssignedOverlapped->lpOverlapped) == TRUE) || (WSAGetLastError() == WSA_IO_PENDING);
	// 投递AcceptEx
	if (!resu) {
		OutputDebugStr(_T("AcceptEx函数失败: %d\n"), WSAGetLastError());
		closesocket(mAssignedOverlapped->AcceptSocket);
		mAssignedOverlapped->AcceptSocket = INVALID_SOCKET;
	}
	return resu;
}

BOOL CSocketLst::Init()
{
	// 分配接受数据的内存
	mLstBuf = malloc(mLstBufLen);
	return TRUE;
}

void CSocketLst::CreateSockObj(CSocketObj* &SockObj)
{
	assert(SockObj == NULL);
	SockObj = new CSocketObj;
	SockObj->SetTag(GetTag());
}

CSocketLst::CSocketLst()
{
	mSocketType = STLst;
	mSocketPoolSize = 10;
	mLstBufLen = (sizeof(sockaddr_storage)+16) * 2;

	mPort = 0;
	mLstBuf = NULL;

}

CSocketLst::~CSocketLst()
{
	if (mLstBuf != NULL) {
		free(mLstBuf);
	}
}

void CSocketLst::SetSocketPoolSize(int Value)
{
	if (mIniteStatus == sisInitializing) {
		if (Value > 0) {
			mSocketPoolSize = Value;
		}
	} else {
		throw invalid_argument("SocketPoolSize can't be set after StartListen");
	}
}

BOOL CSocketLst::StartListen(CCustomIOCPBaseList *IOCPList, int Port, u_long InAddr /*= INADDR_ANY*/)
{
	SOCKADDR_IN InternetAddr;
	PSOCKADDR sockaddr;
	int ErrorCode;
	BOOL resu = FALSE;

	mPort = Port;
	mSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (mSock == INVALID_SOCKET) {
		ErrorCode = WSAGetLastError();
		OutputDebugStr(_T("WSASocket 函数失败：%d\n"), ErrorCode);
		return resu;
	}
	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.s_addr = htonl(InAddr);
	InternetAddr.sin_port = htons(Port);
	sockaddr = (PSOCKADDR)&InternetAddr;
	// 绑定端口号
	if (bind(mSock, sockaddr, sizeof(InternetAddr)) == SOCKET_ERROR) {
		ErrorCode = WSAGetLastError();
		OutputDebugStr(_T("bind 函数失败：%d\n"), ErrorCode);
		closesocket(mSock);
		WSASetLastError(ErrorCode);
		mSock = INVALID_SOCKET;
		return resu;
	}
	// 开始监听
	if (listen(mSock, SOMAXCONN) == SOCKET_ERROR) {
		ErrorCode = WSAGetLastError();
		OutputDebugStr(_T("listen 函数失败：%d\n"), ErrorCode);
		closesocket(mSock);
		WSASetLastError(ErrorCode);
		mSock = INVALID_SOCKET;
		return resu;
	}
	mOwner = IOCPList;
	// 添加到SockLst
	resu = IOCPList->AddSockBase(this);
	if (!resu) {
		ErrorCode = WSAGetLastError();
		OutputDebugStr(_T("AddSockBase 函数失败：%d\n"), ErrorCode);
		closesocket(mSock);
		WSASetLastError(ErrorCode);
		mSock = INVALID_SOCKET;
	}
	return resu;
}



RELEASE_INLINE BOOL CSocketObj::WSARecv()
{
	DWORD Flags;

	// 清空Overlapped
	ZeroMemory(&mAssignedOverlapped->lpOverlapped, sizeof(mAssignedOverlapped->lpOverlapped));
	// 设置OverLap
	mAssignedOverlapped->DataBuf.len = mRecvBufLen;
	mAssignedOverlapped->DataBuf.buf = (CHAR *)mRecvBuf;
	Flags = 0;
	return (::WSARecv(mSock, &mAssignedOverlapped->DataBuf, 1, NULL, &Flags,
		&mAssignedOverlapped->lpOverlapped, NULL) == 0) || (WSAGetLastError() == WSA_IO_PENDING);
}

RELEASE_INLINE BOOL CSocketObj::WSASend(PIOCPOverlapped Overlapped)
{
	// OutputDebugStr(Format('WSASend:Overlapped=%p,Overlapped=%d\n',[Overlapped, Integer(Overlapped.OverlappedType)]));
	// 清空Overlapped
	ZeroMemory(&Overlapped->lpOverlapped, sizeof(Overlapped->lpOverlapped));
	assert(Overlapped->OverlappedType == otSend);
	assert((Overlapped->DataBuf.buf != NULL) && (Overlapped->DataBuf.len > 0));
	return (::WSASend(mSock, &Overlapped->DataBuf, 1, NULL, 0,
		&Overlapped->lpOverlapped, NULL) == 0) || (WSAGetLastError() == WSA_IO_PENDING);
}

BOOL CSocketObj::Init()
{
	assert(mRecvBufLen > 0);
	// 分配接受数据的内存
	mRecvBuf = malloc(mRecvBufLen);
	if (mRecvBuf == NULL) {
		return FALSE;
	}
	// 初始化
	//FSendDataQueue: = TList.Create;
	return TRUE;
}

CSocketObj::CSocketObj()
{
	mSocketType = STObj;
	// 设置初始缓冲区为4096
	mRecvBufLen = 4096;

	mRecvBuf = NULL;
	mIsSerSocket = FALSE;
	mIsSending = FALSE;
}

CSocketObj::~CSocketObj()
{
	while (!mSendDataQueue.empty()) {
		PIOCPOverlapped _IOCPOverlapped;

		_IOCPOverlapped = mSendDataQueue.front();
		mSendDataQueue.pop();
		mOwner->GetOwner()->DelOverlapped(_IOCPOverlapped);
	}
	if (mRecvBuf != NULL) {
		free(mRecvBuf);
	}
}

BOOL CSocketObj::ConnectSer(CCustomIOCPBaseList &IOCPList, LPCTSTR SerAddr, int Port, int IncRefNumber)
{
	BOOL resu = FALSE;
	ADDRINFOT _Hints;
	int _Retval;
	PADDRINFOT _ResultAddInfo;
	PADDRINFOT _NextAddInfo;
	DWORD _AddrStringLen;
	LPTSTR _AddrString;

	assert(mIsSerSocket == FALSE);
	ZeroMemory(&_Hints, sizeof(_Hints));
	_Hints.ai_family = AF_UNSPEC;
	_Hints.ai_socktype = SOCK_STREAM;
	_Hints.ai_protocol = IPPROTO_TCP;
	_Retval = GetAddrInfo(SerAddr, to_tstring(Port).c_str(), &_Hints, &_ResultAddInfo);
	if (_Retval != 0) {
		return FALSE;
	}
	_NextAddInfo = _ResultAddInfo;

	// 申请缓冲区
	_AddrString = new TCHAR[ADDR_STRING_MAX_LEN];

	while (_NextAddInfo != NULL) {
		_AddrStringLen = ADDR_STRING_MAX_LEN;
		// 获取
#ifdef _DEBUG
		if (WSAAddressToString(_NextAddInfo->ai_addr, (DWORD)_NextAddInfo->ai_addrlen, NULL,
			_AddrString, &_AddrStringLen) == 0) {
			// 改为真实长度,这里的_AddrStringLen包含了末尾的字符#0，所以要减去这个#0的长度
			_AddrStringLen--;
			OutputDebugStr(_T("ai_addr:%s,ai_flags:%d,ai_canonname=%s\n"),
				_AddrString, _NextAddInfo->ai_flags, _NextAddInfo->ai_canonname);
		} else {
			OutputDebugStr(_T("WSAAddressToString Error:%d\n"), WSAGetLastError());
		}
#endif
		mSock = WSASocket(_NextAddInfo->ai_family, _NextAddInfo->ai_socktype,
			_NextAddInfo->ai_protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (mSock != INVALID_SOCKET) {
			if (connect(mSock, _NextAddInfo->ai_addr, (INT)_NextAddInfo->ai_addrlen) == SOCKET_ERROR) {
				DWORD LastError = WSAGetLastError();

				OutputDebugStr(_T("连接%s失败：%d\n"), _AddrString, LastError);

				closesocket(mSock);
				WSASetLastError(LastError);
				mSock = INVALID_SOCKET;
			} else {
				mOwner = &IOCPList;
				// 增加引用
				IncRefCount(IncRefNumber);
				resu = IOCPList.AddSockBase(this);
				if (!resu) {
					DWORD LastError = WSAGetLastError();
					OutputDebugStr(_T("添加%s到列表中失败：%d\n"), _AddrString, LastError);
					closesocket(mSock);
					WSASetLastError(LastError);
					mSock = INVALID_SOCKET;
					// 减少引用
					DecRefCount(IncRefNumber);
				}
				break;
			}
		}
		_NextAddInfo = _NextAddInfo->ai_next;
	}
	delete[] _AddrString;
	FreeAddrInfo(_ResultAddInfo);
	return resu;
}

BOOL CSocketObj::GetRemoteAddr(tstring &Address, WORD &Port)
{
	SOCKADDR_STORAGE name;
	int namelen;
	char addrbuf[NI_MAXHOST];
	char portbuf[NI_MAXSERV];

	Address.clear();
	Port = 0;

	namelen = sizeof(name);
	if (getpeername(mSock, (PSOCKADDR)&name, &namelen) == 0) {
		if (getnameinfo((PSOCKADDR)&name, namelen, addrbuf, NI_MAXHOST, portbuf, NI_MAXSERV, NI_NUMERICHOST || NI_NUMERICSERV) == 0) {
			Address = string_to_tstring(string(addrbuf));
			Port = stoi(portbuf);
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CSocketObj::GetLocalAddr(tstring &Address, WORD &Port)
{
	SOCKADDR_STORAGE name;
	int namelen;
	char addrbuf[NI_MAXHOST];
	char portbuf[NI_MAXSERV];

	Address.clear();
	Port = 0;

	namelen = sizeof(name);
	if (getsockname(mSock, (PSOCKADDR)&name, &namelen) == 0) {
		if (getnameinfo((PSOCKADDR)&name, namelen, addrbuf, NI_MAXHOST, portbuf, NI_MAXSERV, NI_NUMERICHOST || NI_NUMERICSERV) == 0) {
			Address = string_to_tstring(string(addrbuf));
			Port = stoi(portbuf);
		}
		return TRUE;
	}
	return FALSE;
}

RELEASE_INLINE void CSocketObj::SetRecvBufLenBeforeInit(DWORD NewRecvBufLen)
{
	assert(this->mIniteStatus == sisInitializing && NewRecvBufLen > 0);
	if (mRecvBufLen != NewRecvBufLen) {
		mRecvBufLen = NewRecvBufLen;
	}
}

BOOL CSocketObj::SendData(LPVOID Data, DWORD DataLen, BOOL UseGetSendDataFunc /*= FALSE*/)
{
	PIOCPOverlapped FIocpOverlapped;
	PVOID _NewData;
	BOOL _PauseSend;
	BOOL resu;

	if (DataLen == 0) {
		return TRUE;
	}
	// 先增加引用
	InternalIncRefCount();
	_NewData = NULL;
	assert(Data != NULL);
	resu = FALSE;
	FIocpOverlapped = mOwner->GetOwner()->NewOverlapped(this, otSend);
	if (FIocpOverlapped != NULL) {
		// 填充发送数据有关的信息
		if (UseGetSendDataFunc) {
			FIocpOverlapped->SendData = Data;
		} else {
			_NewData = malloc(DataLen);
			CopyMemory(_NewData, Data, DataLen);
			FIocpOverlapped->SendData = _NewData;
		}
		FIocpOverlapped->CurSendData = FIocpOverlapped->SendData;
		FIocpOverlapped->SendDataLen = DataLen;
		FIocpOverlapped->DataBuf.buf = (CHAR *)FIocpOverlapped->CurSendData;
		FIocpOverlapped->DataBuf.len = FIocpOverlapped->SendDataLen;
		mOwner->Lock();
		_PauseSend = mIsSending || (mIniteStatus == sisInitializing);
		// 如果里面有正在发送的
		if (_PauseSend) {
			mSendDataQueue.push(FIocpOverlapped);
			OutputDebugStr(_T("Socket(%d)中的发送数据加入到待发送对列\n"), mSock);
		} else {
			mIsSending = TRUE;
		}
		mOwner->Unlock();
		if (!(_PauseSend)) {
			// OutputDebugStr(_T("SendData:Overlapped=%p,Overlapped=%d\n"),FIocpOverlapped, Integer(FIocpOverlapped.OverlappedType)));
			// 投递WSASend
			if (!WSASend(FIocpOverlapped)){
				// 如果有错误
				OutputDebugStr(_T("SendData:WSASend函数失败(socket=%d):%d\n"),
					mSock, WSAGetLastError());
				// 删除此Overlapped
				mOwner->GetOwner()->DelOverlapped(FIocpOverlapped);
				mOwner->Lock();
				mIsSending = FALSE;
				mOwner->Unlock();
			} else {
				resu = TRUE;
			}
		} else {
			// 添加到待发送对列的数据不会增加引用，因此需要取消先前的预引用
			InternalDecRefCount();
			resu = TRUE;
		}
	}
	if (!resu) {
		if (!UseGetSendDataFunc) {
			if (_NewData != NULL) {
				free(_NewData);
			}
		}
		// 减少引用
		InternalDecRefCount();
	}
	return resu;
}

RELEASE_INLINE LPVOID CSocketObj::GetSendData(DWORD DataLen)
{
	return malloc(DataLen);
}

RELEASE_INLINE void CSocketObj::FreeSendData(LPVOID Data)
{
	free(Data);
}

BOOL CSocketObj::SetKeepAlive(BOOL IsOn, int KeepAliveTime /*= 50000*/, int KeepAliveInterval /*= 30000*/)
{
	struct tcp_keepalive alive_in;
	struct tcp_keepalive alive_out;
	DWORD ulBytesReturn;

	alive_in.keepalivetime = KeepAliveTime; // 开始首次KeepAlive探测前的TCP空闭时间
	alive_in.keepaliveinterval = KeepAliveInterval; // 两次KeepAlive探测间的时间间隔
	alive_in.onoff = (ULONG)IsOn;
	return WSAIoctl(mSock, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out,
		sizeof(alive_out), &ulBytesReturn, NULL, NULL) == 0;
}


RELEASE_INLINE void CCustomIOCPBaseList::Lock()
{
	EnterCriticalSection(&mSockBaseCS);
}

RELEASE_INLINE void CCustomIOCPBaseList::Unlock()
{
	LeaveCriticalSection(&mSockBaseCS);
}

BOOL CCustomIOCPBaseList::AddSockBase(CSocketBase *SockBase)
{
	BOOL _IsLocked;
	BOOL resu;

	assert(SockBase->GetSocket() != INVALID_SOCKET);
	assert(SockBase->GetRefCount() >= 0);
	SockBase->mOwner = this;
	// 增加引用计数+1，此引用计数代表Recv的引用
	SockBase->InternalIncRefCount();
	// 开始初始化Socket
	if (!SockBase->Init()) {
		// ieCloseSocket，在没有加入到IOCP之前，都得触发
		SockBase->Close();
		SockBase->InternalDecRefCount();
		return FALSE;
	}
	Lock();
	// List是否被锁住
	_IsLocked = mLockRefNum > 0;
	if (_IsLocked) {
		// 被锁住，不能对Socket列表进行添加或删除操作，先加到Socket待添加List中。
		mSockBaseAddList.push(SockBase);
		OutputDebugStr(_T("列表被锁定，Socket(%d)进入待添加队列\n"), SockBase->GetSocket());
	} else {
		// 没有被锁住，直接添加到Socket列表中
		mSockBaseList.push_back(SockBase);
		// 添加到影子List
		if (SockBase->mSocketType == STObj) {
			mSockObjList.push_back(static_cast<CSocketObj*>(SockBase));
		} else {
			mSockLstList.push_back(static_cast<CSocketLst*>(SockBase));
		}
	}
	Unlock();
	if (!_IsLocked) {
		// 如果没有被锁住，则初始化Socket
		resu = InitSockBase(SockBase);
		if (resu) {

		} else {
			// 初始化出错，
			assert(SockBase->GetRefCount() > 0);
		}
	} else {
		// 如果被锁住，那返回值永远是True
		resu = TRUE;
	}
	return resu;
}

BOOL CCustomIOCPBaseList::RemoveSockBase(CSocketBase *SockBase)
{
	BOOL _IsLocked;

	assert(SockBase->GetRefCount() == 0);
	Lock();
	_IsLocked = mLockRefNum > 0;
	if (!_IsLocked) {
		vector<CSocketBase*>::iterator it;
		for (it = mSockBaseList.begin(); it != mSockBaseList.end(); it++) {
			if ((*it) == SockBase) {
				it = mSockBaseList.erase(it);
				break;
			}
		}
		if (SockBase->mSocketType == STObj) {
			vector<CSocketObj*>::iterator it;
			for (it = mSockObjList.begin(); it != mSockObjList.end(); it++) {
				if ((*it) == SockBase) {
					it = mSockObjList.erase(it);
					break;
				}
			}
		} else {
			vector<CSocketLst*>::iterator it;
			for (it = mSockLstList.begin(); it != mSockLstList.end(); it++) {
				if ((*it) == SockBase) {
					it = mSockLstList.erase(it);
					break;
				}
			}
		}
	} else {
		mSockBaseDelList.push(SockBase);
	}
	Unlock();
	if (!_IsLocked) {
		FreeSockBase(SockBase);
	}
	return TRUE;
}

BOOL CCustomIOCPBaseList::InitSockBase(CSocketBase *SockBase)
{
	// 进入到这里，就说明已经添加到socket列表中了，所以要触发
	BOOL _IsSockObj;

	_IsSockObj = SockBase->mSocketType == STObj;
	try {
		if (_IsSockObj) {
			OnIOCPEvent(ieAddSocket, static_cast<CSocketObj*>(SockBase), NULL);
		} else {
			OnListenEvent(leAddSockLst, static_cast<CSocketLst*>(SockBase));
		}
	}
	catch (...){

	}
	// 锁定
	assert(SockBase->GetRefCount() > 0);
	// 添加到Mgr
	if (!IOCPRegSockBase(SockBase)) {
		// 失败？
		// ieCloseSocket，自己手动发动
		SockBase->Close();
		SockBase->InternalDecRefCount();
		//?
		return TRUE;
	}
	// Result := True;
	// 注册到系统的IOCP中才算初始化完成
	Lock();
	SockBase->mIniteStatus = sisInitialized;
	Unlock();
	if (_IsSockObj) {
		CSocketObj *_SockObj = static_cast<CSocketObj*>(SockBase);
		// 获得Recv的Overlapped
		_SockObj->mAssignedOverlapped = mOwner->NewOverlapped(_SockObj, otRecv);
		// 投递WSARecv
		if (!_SockObj->WSARecv()) {
			// 如果出错
			OutputDebugStr(_T("InitSockObj:WSARecv函数出错socket=%d:%d\n"), _SockObj->GetSocket(), WSAGetLastError());
			try{
				OnIOCPEvent(ieRecvFailed, _SockObj, _SockObj->mAssignedOverlapped);
			}
			catch (...){

			}
			// 减少引用
			SockBase->InternalDecRefCount();
		}
	} else {
		CSocketLst *_SockLst = static_cast<CSocketLst*>(SockBase);
		// 获得Listen的Overlapped
		_SockLst->mAssignedOverlapped = mOwner->NewOverlapped(_SockLst, otListen);
		// 投递AcceptEx
		if (!_SockLst->Accept()) {
			// 减少引用
			SockBase->InternalDecRefCount();
		}
	}
	return TRUE;
}

BOOL CCustomIOCPBaseList::FreeSockBase(CSocketBase *SockBase)
{
	BOOL _IsSockObj;

	assert(SockBase->GetRefCount() == 0);
	_IsSockObj = SockBase->mSocketType == STObj;
	if (_IsSockObj) {
		try {
			OnIOCPEvent(ieDelSocket, static_cast<CSocketObj*>(SockBase), NULL);
		}
		catch (...) {

		}
	} else {
		try {
			OnListenEvent(leDelSockLst, static_cast<CSocketLst*>(SockBase));
		}
		catch (...) {

		}
	}
	delete SockBase;
	if (mIsFreeing) {
		CheckCanDestroy();
	}
	return TRUE;
}

RELEASE_INLINE BOOL CCustomIOCPBaseList::IOCPRegSockBase(CSocketBase *SockBase)
{
	BOOL resu;
	// 在IOCP中注册此Socket
	SockBase->mIOComp = CreateIoCompletionPort((HANDLE)SockBase->GetSocket(), mOwner->mCompletionPort,
		(ULONG_PTR)SockBase, 0);
	resu = SockBase->mIOComp != 0;
	if (!resu) {
		OutputDebugStr(_T("Socket(%d)IOCP注册失败！Error:%d\n"), SockBase->GetSocket(), WSAGetLastError());
	}
	return resu;
}

void CCustomIOCPBaseList::WaitForDestroyEvent()
{
#define EVENT_NUMBER 1
	BOOL _IsEnd;
	HANDLE EventArray[EVENT_NUMBER];

	EventArray[0] = mCanDestroyEvent;
	_IsEnd = FALSE;
	// 等待释放类的事件
	while (!_IsEnd) {
		switch (MsgWaitForMultipleObjects(EVENT_NUMBER, EventArray, FALSE, INFINITE, QS_ALLINPUT)) {
		case WAIT_OBJECT_0:
			// 可以释放了
			_IsEnd = TRUE;
			break;
		case WAIT_OBJECT_0 + EVENT_NUMBER:
			// 有GUI消息，先处理GUI消息
			OutputDebugStr(_T("TIOCPBaseList.Destroy:Process GUI Event\n"));
			ProcessMsgEvent();
			break;
		default:
			//其他事件？
			_IsEnd = TRUE;
		}
	}
}

void CCustomIOCPBaseList::CheckCanDestroy()
{
	BOOL _CanDestroy;

	Lock();
	_CanDestroy = (mSockBaseList.size() == 0) && (mSockBaseAddList.size() == 0) &&
		(mSockBaseDelList.size() == 0);
	Unlock();
	if (_CanDestroy) {
		SetEvent(mCanDestroyEvent);
	}
}

void CCustomIOCPBaseList::OnIOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped)
{

}

void CCustomIOCPBaseList::OnListenEvent(ListenEventEnum EventType, CSocketLst *SockLst)
{

}

CCustomIOCPBaseList::CCustomIOCPBaseList(CIOCPManager *AIOCPMgr)
{
	mOwner = AIOCPMgr;
	mLockRefNum = 0;
	mIsFreeing = FALSE;
	mCanDestroyEvent = NULL;

	assert(AIOCPMgr != NULL);
	InitializeCriticalSection(&mSockBaseCS);
	// 添加自身
	mOwner->AddSockList(this);
}

CCustomIOCPBaseList::~CCustomIOCPBaseList()
{
	mCanDestroyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	mIsFreeing = TRUE;
	CloseAllSockBase();
	CheckCanDestroy();
	WaitForDestroyEvent();

	mOwner->RemoveSockList(this);
	CloseHandle(mCanDestroyEvent);
}

void CCustomIOCPBaseList::LockSockList()
{
	Lock();
	assert(mLockRefNum >= 0);
	mLockRefNum++;
	Unlock();
}

void CCustomIOCPBaseList::UnlockSockList()
{
	BOOL _IsEnd;

	do {
		CSocketBase *_SockBase = NULL;
		BOOL isAdd = FALSE;

		Lock();
		assert(mLockRefNum >= 1);
		// 判断是不是只有本线程锁定了列表，只要判断FLockRefNum是不是大于1
		_IsEnd = mLockRefNum > 1;
		if (!_IsEnd) {
			// 只有本线程锁住了socket，然后查看socket删除列表是否为空
			if (mSockBaseDelList.size() > 0) {
				vector<CSocketBase*>::iterator it;
				BOOL _IsSockObj;

				// 不为空，从第一个开始删
				_SockBase = mSockBaseDelList.front();
				mSockBaseDelList.pop();

				for (it = mSockBaseList.begin(); it != mSockBaseList.end(); it++) {
					if ((*it) == _SockBase) {
						mSockBaseList.erase(it);
						break;
					}
				}
				_IsSockObj = _SockBase->mSocketType == STObj;
				if (_IsSockObj) {
					vector<CSocketObj*>::iterator it;
					for (it = mSockObjList.begin(); it != mSockObjList.end(); it++) {
						if ((*it) == _SockBase) {
							mSockObjList.erase(it);
							break;
						}
					}
				} else {
					vector<CSocketLst*>::iterator it;
					for (it = mSockLstList.begin(); it != mSockLstList.end(); it++) {
						if ((*it) == _SockBase) {
							mSockLstList.erase(it);
							break;
						}
					}
				}
				isAdd = FALSE;
			} else {
				// 查看socket添加列表是否为空
				if (mSockBaseAddList.size() > 0) {
					BOOL _IsSockObj;


					isAdd = TRUE;
					// 如果不为空，则pop一个sockobj添加到列表中
					_SockBase = mSockBaseAddList.front();
					mSockBaseAddList.pop();
					mSockBaseList.push_back(_SockBase);
					_IsSockObj = _SockBase->mSocketType == STObj;
					if (_IsSockObj) {
						mSockObjList.push_back(static_cast<CSocketObj*>(_SockBase));
					} else {
						mSockLstList.push_back(static_cast<CSocketLst*>(_SockBase));
					}
				} else {
					// 都为空，则表示已经结束了
					_IsEnd = TRUE;
				}
			}
		}
		// 如果没什么想要处理的了，锁定列表数减1
		if (_IsEnd) {
			mLockRefNum--;
		}
		Unlock();
		// 查看sockobj是否为空，不为空则表示在锁List期间有删除sock或者添加sock操作
		if (_SockBase != NULL) {
			if (isAdd) {
				// 有添加sock操作，初始化sockobj，如果失败，会自动被Free掉，无需获取返回值
				InitSockBase(_SockBase);
			} else {
				// 有删除sock操作，删除sockobk
				// InitSockBase(_SockBase);
				// RemoveSockBase(_SockBase);
				assert(_SockBase->GetRefCount() == 0);
				// _SockBase.Free;
				FreeSockBase(_SockBase);
			}
		}
	} while (!_IsEnd);
}

void CCustomIOCPBaseList::ProcessMsgEvent()
{
	MSG Msg;

	while (PeekMessage(&Msg, 0, 0, 0, PM_NOREMOVE)) {
		BOOL Unicode;
		BOOL MsgExists;

		Unicode = (Msg.hwnd == 0) || IsWindowUnicode(Msg.hwnd);
		if (Unicode) {
			MsgExists = PeekMessageW(&Msg, 0, 0, 0, PM_REMOVE);
		} else {
			MsgExists = PeekMessageA(&Msg, 0, 0, 0, PM_REMOVE);
		}
		if (MsgExists) {
			TranslateMessage(&Msg);
			if (Unicode) {
				DispatchMessageW(&Msg);
			} else {
				DispatchMessageA(&Msg);
			}
		}
	}
}

void CCustomIOCPBaseList::CloseAllSockObj()
{
	vector<CSocketObj*>::iterator it;

	LockSockList();
	for (it = mSockObjList.begin(); it != mSockObjList.end(); it++) {
		(*it)->Close();
	}
	UnlockSockList();
}

void CCustomIOCPBaseList::CloseAllSockLst()
{
	vector<CSocketLst*>::iterator it;

	LockSockList();
	for (it = mSockLstList.begin(); it != mSockLstList.end(); it++) {
		(*it)->Close();
	}
	UnlockSockList();
}

void CCustomIOCPBaseList::CloseAllSockBase()
{
	vector<CSocketBase*>::iterator it;

	LockSockList();
	for (it = mSockBaseList.begin(); it != mSockBaseList.end(); it++) {
		(*it)->Close();
	}
	UnlockSockList();
}

#ifdef _UNICODE
#define GetHostName GetHostNameW
#else
#define GetHostName gethostname
#endif // _UNICODE

void CCustomIOCPBaseList::GetLocalAddrs(vector<tstring> &Addrs)
{
	LPTSTR sHostName;
	ADDRINFOT _Hints;
	int _Retval;
	PADDRINFOT _ResultAddInfo;
	PADDRINFOT _NextAddInfo;

	Addrs.clear();
	sHostName = new TCHAR[MAX_PATH];
	if (GetHostName(sHostName, MAX_PATH) == SOCKET_ERROR) {
		return;
	}

	ZeroMemory(&_Hints, sizeof(_Hints));
	_Hints.ai_family = AF_UNSPEC;
	_Hints.ai_socktype = SOCK_STREAM;
	_Hints.ai_protocol = IPPROTO_TCP;
	_Retval = GetAddrInfo(sHostName, NULL, &_Hints, &_ResultAddInfo);
	if (_Retval == 0) {
		DWORD _AddrStringLen;
		LPTSTR _AddrString;

		_NextAddInfo = _ResultAddInfo;
		// 申请缓冲区
		_AddrString = new TCHAR[ADDR_STRING_MAX_LEN];

		while (_NextAddInfo != NULL) {
			_AddrStringLen = ADDR_STRING_MAX_LEN;
			// 获取

			if (WSAAddressToString(_NextAddInfo->ai_addr, (DWORD)_NextAddInfo->ai_addrlen, NULL,
				_AddrString, &_AddrStringLen) == 0) {
				// 改为真实长度,这里的_AddrStringLen包含了末尾的字符#0，所以要减去这个#0的长度
				_AddrStringLen--;
				Addrs.push_back(tstring(_AddrString));
				OutputDebugStr(_T("ai_addr:%s,ai_flags:%d,ai_canonname=%s\n"),
					_AddrString, _NextAddInfo->ai_flags, _NextAddInfo->ai_canonname);
			} else {
				OutputDebugStr(_T("WSAAddressToString Error:%d\n"), WSAGetLastError());
			}

			_NextAddInfo = _NextAddInfo->ai_next;
		}
		delete[] _AddrString;
		FreeAddrInfo(_ResultAddInfo);
	}
	delete[] sHostName;
}

void CIOCPManager::AddSockList(CCustomIOCPBaseList *SockList)
{
	LockSockList();
	mSockList.push_back(SockList);
	UnlockSockList();
}

void CIOCPManager::RemoveSockList(CCustomIOCPBaseList *SockList)
{
	vector<CCustomIOCPBaseList*>::iterator it;
	LockSockList();
	for (it = mSockList.begin(); it != mSockList.end(); it++) {
		if ((*it) == SockList) {
			mSockList.erase(it);
			break;
		}
	}
	UnlockSockList();
}

void CIOCPManager::FreeOverLappedList()
{
	vector<PIOCPOverlapped>::iterator it;
	LockOverLappedList();
	for (it = mOverLappedList.begin(); it != mOverLappedList.end(); it++) {
		assert((*it)->IsUsed == FALSE);
		free(*it);
	}
	mOverLappedList.clear();
	UnlockOverLappedList();
}

void CIOCPManager::DelOverlapped(PIOCPOverlapped UsedOverlapped)
{
	assert(UsedOverlapped != NULL);
	// 正在使用设置为False
	assert(UsedOverlapped->IsUsed == TRUE);
	switch (UsedOverlapped->OverlappedType) {
	case otSend:
		assert(UsedOverlapped->SendData != NULL);
		if (UsedOverlapped->SendData != NULL) {
			free(UsedOverlapped->SendData);
			UsedOverlapped->SendData = NULL;
		}
		break;
	case otListen:
		if (UsedOverlapped->AcceptSocket != INVALID_SOCKET) {
			closesocket(UsedOverlapped->AcceptSocket);
			UsedOverlapped->AcceptSocket = INVALID_SOCKET;
		}
		break;
	}
	LockOverLappedList();
	// 正在使用设置为False
	UsedOverlapped->IsUsed = FALSE;
	mOverLappedList.push_back(UsedOverlapped);
	UnlockOverLappedList();
}

PIOCPOverlapped CIOCPManager::NewOverlapped(CSocketBase *SockObj, OverlappedTypeEnum OverlappedType)
{
	PIOCPOverlapped _NewOverLapped;
	LockOverLappedList();

	if (mOverLappedList.size() > 0) {
		_NewOverLapped = mOverLappedList[0];
		mOverLappedList.erase(mOverLappedList.begin());
	} else {
		_NewOverLapped = (PIOCPOverlapped)malloc(sizeof(IOCPOverlapped));
	}
	_NewOverLapped->IsUsed = TRUE;

	UnlockOverLappedList();

	// 已经使用
	_NewOverLapped->AssignedSockObj = SockObj;
	_NewOverLapped->OverlappedType = OverlappedType;
	// 清零
	switch (OverlappedType) {
	case otSend:
		_NewOverLapped->SendData = NULL;
		_NewOverLapped->CurSendData = NULL;
		_NewOverLapped->SendDataLen = 0;
	case otRecv:
		_NewOverLapped->RecvData = NULL;
		_NewOverLapped->RecvDataLen = 0;
	case otListen:
		_NewOverLapped->AcceptSocket = INVALID_SOCKET;
	default:
		break;
	}
	return _NewOverLapped;
}

BOOL CIOCPManager::PostExitStatus()
{
	OutputDebugStr(_T("发送线程退出命令。\n"));
	return PostQueuedCompletionStatus(mCompletionPort, 0, 0, NULL);
}

CIOCPManager::CIOCPManager(int IOCPThreadCount /*= 0*/)
{
	SOCKET TmpSock;
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD dwBytes;
	INT i;

	OutputDebugStr(_T("IOCPManager::IOCPManager\n"));
	// 使用 2.2版的WS2_32.DLL
	if (WSAStartup(0x0202, &mwsaData) != 0) {
		throw exception("WSAStartup Fails");
	}
	// 获取AcceptEx和GetAcceptExSockaddrs的函数指针
	TmpSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (TmpSock == INVALID_SOCKET) {
		throw exception("WSASocket Fails");
	}
	if (SOCKET_ERROR == WSAIoctl(TmpSock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(guidAcceptEx), &g_AcceptEx, sizeof(g_AcceptEx), &dwBytes, NULL, NULL)) {
		throw exception("WSAIoctl WSAID_ACCEPTEX Fails");
	}
	if (SOCKET_ERROR == WSAIoctl(TmpSock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptExSockaddrs, sizeof(guidGetAcceptExSockaddrs), &g_GetAcceptExSockaddrs, sizeof(g_GetAcceptExSockaddrs), &dwBytes, NULL, NULL)) {
		throw exception("WSAIoctl WSAID_GETACCEPTEXSOCKADDRS Fails");
	}
	closesocket(TmpSock);
	// 初始化临界区
	InitializeCriticalSection(&mSockListCS);
	InitializeCriticalSection(&mOverLappedListCS);
	// 初始化IOCP完成端口
	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	if (IOCPThreadCount <= 0) {
		SYSTEM_INFO SysInfo;
		GetSystemInfo(&SysInfo);
		IOCPThreadCount = SysInfo.dwNumberOfProcessors + 2;
	}
	mIocpWorkThreadCount = IOCPThreadCount;
	mIocpWorkThreads = new HANDLE[mIocpWorkThreadCount];
	// 创建IOCP工作线程
	for (i = 0; i < mIocpWorkThreadCount; i++) {
		mIocpWorkThreads[i] = (HANDLE)_beginthreadex(NULL, 0, IocpWorkThread, (PVOID)mCompletionPort, 0, NULL);
		if (mIocpWorkThreads[i] == NULL) {
			throw exception("CreateThread FIocpWorkThreads Fails");
		}
	}
}

CIOCPManager::~CIOCPManager()
{
	BOOL Resu;
	LockSockList();
	try {
		if (mSockList.size() > 0) {
			throw exception("SockList必须全部释放");
		}
	}
	catch (...) {
		UnlockSockList();
		throw;
	}
	UnlockSockList();
	Resu = PostExitStatus();
	assert(Resu == TRUE);
	OutputDebugStr(_T("等待完成端口工作线程退出。\n"));
	// 等待工作线程退出
	WaitForMultipleObjects(mIocpWorkThreadCount, mIocpWorkThreads, TRUE, INFINITE);
	delete[] mIocpWorkThreads;
	OutputDebugStr(_T("等待完成端口句柄。\n"));
	// 关闭IOCP句柄
	CloseHandle(mCompletionPort);
	// 等待SockLst释放，这个比较特殊
	// WaitSockLstFree;
	// 释放
	FreeOverLappedList();
	DeleteCriticalSection(&mOverLappedListCS);
	assert(mSockList.size() == 0);
	DeleteCriticalSection(&mSockListCS);
	// 关闭Socket
	WSACleanup();
}

RELEASE_INLINE void CIOCPManager::LockSockList()
{
	EnterCriticalSection(&mSockListCS);
}

RELEASE_INLINE void CIOCPManager::UnlockSockList()
{
	LeaveCriticalSection(&mSockListCS);
}

RELEASE_INLINE void CIOCPManager::LockOverLappedList()
{
	EnterCriticalSection(&mOverLappedListCS);
}

RELEASE_INLINE void CIOCPManager::UnlockOverLappedList()
{
	LeaveCriticalSection(&mOverLappedListCS);
}

void CIOCPBaseList::OnIOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped)
{
	if (mIOCPEvent.IsAvaliable()) {
		TRIGGER_DELEGATE(mIOCPEvent)(EventType, SockObj, Overlapped);
	}
}

void CIOCPBaseList::OnListenEvent(ListenEventEnum EventType, CSocketLst *SockLst)
{
	if (mListenEvent.IsAvaliable()) {
		TRIGGER_DELEGATE(mListenEvent)(EventType, SockLst);
	}
}

CIOCPBaseList::CIOCPBaseList(CIOCPManager *AIOCPMgr) :CCustomIOCPBaseList(AIOCPMgr)
{

}
