#ifndef _LCXL_IOCP_BASE_H_
#define _LCXL_IOCP_BASE_H_

#include <WinSock2.h>
#include <windows.h>
#include <vector>
#include <queue>
#include <assert.h>
#include "lcxl_string.h"

#include "lcxl_func_delegate.h"

using namespace std;
//定义IP地址字符串最大长度
#define ADDR_STRING_MAX_LEN 1024

#ifdef _DEBUG
#define RELEASE_INLINE
#else
#define RELEASE_INLINE inline
#endif // DEBUG

#ifdef _DEBUG
void OutputDebugStr(const TCHAR fmt[], ...);
#else
#define OutputDebugStr(__fmt, ...)
#endif // _DEBUG



typedef enum _IocpEventEnum {ieAddSocket,

	/// <summary>
	/// socket从IOCP管理器移除事件
	/// </summary>
	ieDelSocket,

	/// <summary>
	/// socket被系统关闭事件，当触发这事件时，用户必须释放此socket的引用，以便iocp管理器清除此socket，当用户引用释放之后，会触发ieD
	/// elSocket事件
	/// </summary>
	ieCloseSocket,

	ieError,

	/// <summary>
	/// ieRecvPart 在本单元中没有实现，扩展用
	/// </summary>
	ieRecvPart,

	ieRecvAll,

	ieRecvFailed,

	ieSendPart,

	ieSendAll,

	ieSendFailed} IocpEventEnum, *PIocpEventEnum;
typedef enum _ListenEventEnum {leAddSockLst, leDelSockLst, leCloseSockLst, leListenFailed} ListenEventEnum, *PListenEventEnum;
//前置申明
//Socket类
class CSocketBase;
// 监听socket类，要实现不同的功能，需要继承并实现其子类
class CSocketLst;
//typedef IOCPOverlapped *PIOCPOverlapped;
class CSocketObj;
class CCustomIOCPBaseList;
class CIOCPBaseList;
class CIOCPManager;

typedef CSocketObj *PCSocketObj;

typedef enum _OverlappedTypeEnum {otRecv, otSend, otListen} OverlappedTypeEnum;
/// <summary>
/// socket类的状态
/// </summary>
typedef enum _SocketInitStatus {
	/// <summary>
	/// socket类正在初始化
	/// </summary>
	sisInitializing,

	/// <summary>
	/// socket类初始化完成
	/// </summary>
	sisInitialized,

	/// <summary>
	/// socket类正在析构
	/// </summary>
	sisDestroying} SocketInitStatus;

typedef struct _IOCPOverlapped {
	OVERLAPPED lpOverlapped;
	WSABUF DataBuf;
	BOOL IsUsed;
	OverlappedTypeEnum OverlappedType;
	CSocketBase *AssignedSockObj;
	LPVOID GetRecvData() {
		assert(OverlappedType == otRecv);
		return RecvData;
	}
	DWORD GetRecvDataLen() {
		assert(OverlappedType == otRecv);
		return RecvDataLen;
	}
	DWORD GetCurSendDataLen() {
		assert(OverlappedType == otSend);
		return (DWORD)((DWORD_PTR)CurSendData - (DWORD_PTR)SendData);
	}
	LPVOID GetSendData() {
		assert(OverlappedType == otSend);
		return SendData;
	}
	DWORD GetTotalSendDataLen() {
		assert(OverlappedType == otSend);
		return SendDataLen;
	}
	union
	{
		struct {
			LPVOID RecvData;
			DWORD RecvDataLen;
		};
		struct 
		{
			LPVOID SendData;
			LPVOID CurSendData;
			DWORD SendDataLen;
		};
		struct  
		{
			SOCKET AcceptSocket;
		};
	};
} IOCPOverlapped, *PIOCPOverlapped;

/// <summary>
/// Socket类型
/// </summary>
typedef enum _SocketType{ STObj, STLst } SocketType;

class CSocketBase {
protected:
	SocketType mSocketType;
	int mRefCount;
	int mUserRefCount;
	SocketInitStatus mIniteStatus;
	SOCKET mSock;
	CCustomIOCPBaseList *mOwner;
	HANDLE mIOComp;
	PIOCPOverlapped mAssignedOverlapped;
	UINT_PTR mTag;
	virtual BOOL Init() = 0;
	int InternalIncRefCount(int Count=1, BOOL UserMode=FALSE);
	int InternalDecRefCount(int Count=1, BOOL UserMode=FALSE);
public:
	CSocketBase();
	virtual ~CSocketBase();
	virtual void Close();
	//Property
	CCustomIOCPBaseList *GetOwner() {
		return mOwner;
	}
	SOCKET GetSocket() {
		return mSock;
	}
	SocketInitStatus GetIniteStatus() {
		return mIniteStatus;
	}
	UINT_PTR GetTag() {
		return mTag;
	}
	void SetTag(UINT_PTR Value) {
		mTag = Value;
	}
	PIOCPOverlapped GetAssignedOverlapped() {
		return mAssignedOverlapped;
	}
	int IncRefCount(int Count=1);
	int DecRefCount(int Count=1);
	/// <summary>
	/// 引用计数
	/// </summary>
	int GetRefCount(){
		return mRefCount;
	}
	friend class CCustomIOCPBaseList;
	friend unsigned __stdcall IocpWorkThread(void *CompletionPortID);
};

class CSocketLst: public CSocketBase {
private:
	int mPort;
	PVOID mLstBuf;
	DWORD mLstBufLen;
	int mSocketPoolSize;
protected:
	BOOL Accept();
	virtual BOOL Init();
	virtual void CreateSockObj(CSocketObj* &SockObj);
public:
	CSocketLst();
	virtual ~CSocketLst();
	//Property
	int GetPort() {
		return mPort;
	}
	int GetSocketPoolSize() {
		return mSocketPoolSize;
	}
	void SetSocketPoolSize(int Value);
	BOOL StartListen(CCustomIOCPBaseList *IOCPList, int Port, u_long InAddr = INADDR_ANY);
	friend class CCustomIOCPBaseList;
	friend unsigned __stdcall IocpWorkThread(void *CompletionPortID);
};

/// <summary>
/// Socket类，一个类管一个套接字
/// </summary>
class CSocketObj: public CSocketBase {
private:
	DWORD mRecvBufLen;
	LPVOID mRecvBuf;
	BOOL mIsSerSocket;
	BOOL mIsSending;
	queue<PIOCPOverlapped> mSendDataQueue;
	RELEASE_INLINE BOOL WSARecv();
	RELEASE_INLINE BOOL WSASend(PIOCPOverlapped Overlapped);
	void SetIsSending(BOOL value) {
		mIsSending = value;
	}
protected:
	virtual BOOL Init();
public:
	CSocketObj();
	virtual ~CSocketObj();
	/// <summary>
	/// 连接指定的网络地址，支持IPv6
	/// </summary>
	/// <param name="IOCPList">
	/// Socket列表
	/// </param>
	/// <param name="SerAddr">
	/// 要连接的地址
	/// </param>
	/// <param name="Port">
	/// 要连接的端口号
	/// </param>
	/// <param name="IncRefNumber">如果成功，则增加多少引用计数，引用计数需要程序员自己释放，不然会一直占用</param>
	/// <returns>
	/// 返回是否连接成功
	/// </returns>
	BOOL ConnectSer(CCustomIOCPBaseList &IOCPList, LPCTSTR SerAddr, int Port, int IncRefNumber);
	//Windows平台下使用WSAAddressToString 
	tstring GetRemoteIP() {
		tstring Address;
		WORD Port;
		GetRemoteAddr(Address, Port);
		return Address;
	}
	WORD GetRemotePort() {
		tstring Address;
		WORD Port;
		GetRemoteAddr(Address, Port);
		return Port;
	}
	BOOL GetRemoteAddr(tstring &Address, WORD &Port);
	tstring GetLocalIP() {
		tstring Address;
		WORD Port;
		GetLocalAddr(Address, Port);
		return Address;
	}
	WORD GetLocalPort() {
		tstring Address;
		WORD Port;
		GetLocalAddr(Address, Port);
		return Port;
	}
	BOOL GetLocalAddr(tstring &Address, WORD &Port);
	LPVOID GetRecvBuf() {
		return mRecvBuf;
	}
	RELEASE_INLINE void SetRecvBufLenBeforeInit(DWORD NewRecvBufLen);
	BOOL SendData(LPVOID Data, DWORD DataLen, BOOL UseGetSendDataFunc = FALSE);
	RELEASE_INLINE LPVOID GetSendData(DWORD DataLen);
	RELEASE_INLINE void FreeSendData(LPVOID Data);
	BOOL SetKeepAlive(BOOL IsOn, int KeepAliveTime = 50000, int KeepAliveInterval = 30000);

	BOOL GetIsSerSocket() {
		return mIsSerSocket;
	}

	BOOL GetIsSending() {
		return mIsSending;
	}
	queue<PIOCPOverlapped> &GetSendDataQueue() {
		return mSendDataQueue;
	}
	friend class CCustomIOCPBaseList;
	friend unsigned __stdcall IocpWorkThread(void *CompletionPortID);
};

/// <summary>
/// 存储Socket列表的类，前身为的TSocketMgr类
/// </summary>
class CCustomIOCPBaseList{
private:
	HANDLE mCanDestroyEvent;
	BOOL mIsFreeing;
	CIOCPManager *mOwner;
	int mLockRefNum;
	RTL_CRITICAL_SECTION mSockBaseCS;
	vector<CSocketBase*> mSockBaseList;
	queue<CSocketBase*> mSockBaseAddList;
	queue<CSocketBase*> mSockBaseDelList;
	vector<CSocketObj*> mSockObjList;
	vector<CSocketLst*> mSockLstList;
	
protected:
	/// <summary>
	/// 这个只是单纯的临界区锁，要更加有效的锁定列表，使用 LockSockList
	/// </summary>
	RELEASE_INLINE void Lock();
	/// <summary>
	/// 添加sockobj到列表中，返回True表示成功，返回False表示失败，注意这里要处理IsFreeing为True的情况
	/// </summary>
	RELEASE_INLINE void Unlock();
	/// <summary>
	/// 添加sockobj到列表中，返回True表示成功，返回False表示失败，注意这里要处理IsFreeing为True的情况
	/// </summary>
	BOOL AddSockBase(CSocketBase *SockBase);
	/// <summary>
	/// 移除sockbase，如果 列表被锁定，则将socket类放入待删除队列中
	/// </summary>
	BOOL RemoveSockBase(CSocketBase *SockBase);
	/// <summary>
	/// 初始化SockBase
	/// </summary>
	BOOL InitSockBase(CSocketBase *SockBase);
	/// <summary>
	/// 释放sockbase，并触发事件，此时sockbase必须已经从列表中移除
	/// </summary>
	BOOL FreeSockBase(CSocketBase *SockBase);
	/// <summary>
	/// 在IOCP管理器中注册SockBase
	/// </summary>
	RELEASE_INLINE BOOL IOCPRegSockBase(CSocketBase *SockBase);
	void WaitForDestroyEvent();
	/// <summary>
	/// 检查是否可以释放
	/// </summary>
	void CheckCanDestroy();
	/// <summary>
	/// IOCP事件
	/// </summary>
	virtual void OnIOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped);
	virtual void OnListenEvent(ListenEventEnum EventType, CSocketLst *SockLst);
public:
	CCustomIOCPBaseList(CIOCPManager *AIOCPMgr);
	virtual ~CCustomIOCPBaseList();
	/// <summary>
	/// 锁定列表，注意的锁定后不能对列表进行增加，删除操作，一切都由SocketMgr类维护
	/// </summary>
	void LockSockList();

	void UnlockSockList();
	/// <summary>
	/// 处理消息函数，在有窗口的程序下使用
	/// </summary>
	void ProcessMsgEvent();
	/// <summary>
	/// 关闭所有的Socket
	/// </summary>
	void CloseAllSockObj();
	/// <summary>
	/// 关闭所有的Socklst
	/// </summary>
	void CloseAllSockLst();
	/// <summary>
	/// 关闭所有的Socket，包括监听socket和非监听socket
	/// </summary>
	void CloseAllSockBase();
	/// <summary>
	/// 此类的拥有者
	/// </summary>
	CIOCPManager *GetOwner() {
		return mOwner;
	}
	vector<CSocketBase*> *GetSockBaseList() {
		return &mSockBaseList;
	}
	vector<CSocketLst*> *GetSockLstList() {
		return &mSockLstList;
	}
	vector<CSocketObj*> *GetSockObjList() {
		return &mSockObjList;
	}
	/// <summary>
	/// 获取本机IP地址列表
	/// </summary>
	/// <param name="Addrs">
	/// 获取后的ip地址存入此列表中
	/// </param>
	static void GetLocalAddrs(vector<tstring> &Addrs);
	//友类的声明
	friend class CSocketBase;       
	friend class CSocketLst;
	friend class CSocketObj;
	friend unsigned __stdcall IocpWorkThread(void *CompletionPortID);
};

class CIOCPManager {
private:
	WSADATA mwsaData;
	vector<CCustomIOCPBaseList*> mSockList;
	RTL_CRITICAL_SECTION mSockListCS;
	vector<PIOCPOverlapped> mOverLappedList;
	RTL_CRITICAL_SECTION mOverLappedListCS;
	HANDLE mCompletionPort;
	INT mIocpWorkThreadCount;
	HANDLE *mIocpWorkThreads;
protected:
	void AddSockList(CCustomIOCPBaseList *SockList);
	void RemoveSockList(CCustomIOCPBaseList *SockList);
	void FreeOverLappedList();
	void DelOverlapped(PIOCPOverlapped UsedOverlapped);
	PIOCPOverlapped NewOverlapped(CSocketBase *SockObj, OverlappedTypeEnum OverlappedType);
	BOOL PostExitStatus();
public:
	CIOCPManager(int IOCPThreadCount = 0);
	virtual ~CIOCPManager();
	RELEASE_INLINE void LockSockList();

	vector<CCustomIOCPBaseList*> &GetSockList() {
		return mSockList;
	}
	RELEASE_INLINE void UnlockSockList();
	RELEASE_INLINE void LockOverLappedList();
	vector<PIOCPOverlapped> &GetOverLappedList() {
		return mOverLappedList;
	}
	RELEASE_INLINE void UnlockOverLappedList();
	//友类
	friend class CSocketBase;
	//友类
	friend class CSocketObj;

	friend class CCustomIOCPBaseList;
	friend unsigned __stdcall IocpWorkThread(void *CompletionPortID);
};
//前置申明
class CIOCPBaseList;
// IOCP事件
typedef void (CIOCPBaseList::*EOnIOCPBaseEvent)(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped);
// 监听事件
typedef void (CIOCPBaseList::*EOnListenBaseEvent)(ListenEventEnum EventType, CSocketLst *SockLst);

//定义事件触发函数类型
typedef _LCXLFunctionDelegate<CIOCPBaseList, EOnIOCPBaseEvent> DOnIOCPBaseEvent;
typedef _LCXLFunctionDelegate<CIOCPBaseList, EOnListenBaseEvent> DOnListenBaseEvent;

class CIOCPBaseList :public CCustomIOCPBaseList {
private:
	DOnIOCPBaseEvent mIOCPEvent;
	DOnListenBaseEvent mListenEvent;
protected:
	/// <summary>
	/// IOCP事件
	/// </summary>
	virtual void OnIOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped);
	virtual void OnListenEvent(ListenEventEnum EventType, CSocketLst *SockLst);
public:
	CIOCPBaseList(CIOCPManager *AIOCPMgr);
	// 外部接口
	const DOnIOCPBaseEvent &GetIOCPEvent() {
		return mIOCPEvent;
	}
	void SetIOCPEvent(const DOnIOCPBaseEvent &Value) {
		mIOCPEvent = Value;
	}
	const DOnListenBaseEvent &GetListenEvent() {
		return mListenEvent;
	}
	void SetListenEvent(const DOnListenBaseEvent &Value) {
		mListenEvent = Value;
	}
};

#endif