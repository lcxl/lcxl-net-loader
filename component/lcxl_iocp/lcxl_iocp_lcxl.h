#ifndef _LCXL_IOCP_LCXL_H_
#define _LCXL_IOCP_LCXL_H_

#include "lcxl_iocp_base.h"

typedef struct _SendDataRec {
private:
	ULONG mTotalLen;
	PVOID mTotalData;
	PVOID mData;
	ULONG mDataLen;
public:
	PVOID GetData() {
		return mData;
	};
	ULONG GetDataLen() {
		return mDataLen;
	};
	/// <summary>
	/// 将给定的数据转化为本记录的数据结构
	/// </summary>
	BOOL Assign(PVOID _TotalData, ULONG _TotalLen);
public:
	friend class CLLSockObj;
} SendDataRec, *PSendDataRec;

class CLLSockLst :public CSocketLst {
protected:
	virtual void CreateSockObj(CSocketObj* &SockObj);// 覆盖
public:
	virtual ~CLLSockLst() {

	}
};

///	<summary>
///	  LCXL协议的socket类
///	</summary>
class CLLSockObj : public CSocketObj {
private:
	PVOID mBuf;
	ULONG mCurDataLen;
	ULONG mBufLen;
	/// <summary>
	/// 接收到的数据
	/// </summary>
	PVOID mRecvData;
	ULONG mRecvDataLen;
	/// <summary>
	/// 是否接收一个完整的数据
	/// </summary>
	BOOL mIsRecvAll;
protected:
	// 初始化
	virtual BOOL Init();
public:
	virtual PVOID GetRecvData();
	virtual long GetRecvDataLen();
protected:
	PVOID GetRecvBuf() {
		return mBuf;
	};
public:
	// 销毁
	virtual ~CLLSockObj();
	// SendData之前锁定
	BOOL SendData(const SendDataRec &ASendDataRec);
	BOOL SendData(PVOID Data, ULONG DataLen);
	// 获取发送数据的指针
	void GetSendData(ULONG DataLen, SendDataRec &ASendDataRec);
	// 只有没有调用SendData的时候才可以释放，调用SendData之后将会自动释放。
	void FreeSendData(const SendDataRec &ASendDataRec);
	BOOL GetIsRecvAll() {
		return mIsRecvAll;
	}
public:
	friend class CCustomIOCPLCXLList;
};

class CCustomIOCPLCXLList : public CCustomIOCPBaseList {
private:
	virtual void OnIOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped);
protected:
	virtual void OnIOCPEvent(IocpEventEnum EventType, CLLSockObj *SockObj, PIOCPOverlapped Overlapped);
};

class CIOCPLCXLList;

// IOCP事件
typedef void (CIOCPLCXLList::*EOnIOCPLCXLEvent)(IocpEventEnum EventType, CLLSockObj *SockObj, PIOCPOverlapped Overlapped);
//定义事件触发函数类型
typedef _LCXLFunctionDelegate<CIOCPLCXLList, EOnIOCPLCXLEvent> DOnIOCPLCXLEvent;
// IOCP监听事件
typedef void (CIOCPLCXLList::*EOnListenLCXLEvent)(ListenEventEnum EventType, CLLSockLst *SockLst);
//定义事件触发函数类型
typedef _LCXLFunctionDelegate<CIOCPLCXLList, EOnListenLCXLEvent> DOnListenLCXLEvent;


// LCXL协议实现类
class CIOCPLCXLList : public CCustomIOCPBaseList{
private:
	DOnIOCPLCXLEvent mIOCPEvent;
	DOnListenLCXLEvent mListenEvent;
protected:
	virtual void OnIOCPEvent(IocpEventEnum EventType, CLLSockObj *SockObj, PIOCPOverlapped Overlapped);
	// 监听事件
	virtual void OnListenEvent(ListenEventEnum EventType, CSocketLst *SockLst);
public:
	DOnIOCPLCXLEvent GetIOCPEvent() {
		return mIOCPEvent;
	};
	void SetIOCPEvent(const DOnIOCPLCXLEvent &Value) {
		mIOCPEvent = Value;
	};
	DOnListenLCXLEvent GetListenEvent() {
		return mListenEvent;
	};
	void SetListenEvent(const DOnListenLCXLEvent &Value) {
		mListenEvent = Value;
	};
};

#endif // !_LCXL_IOCP_LCXL_H_
