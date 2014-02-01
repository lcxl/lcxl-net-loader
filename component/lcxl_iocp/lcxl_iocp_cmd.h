#ifndef _LCXL_IOCP_CMD_H_
#define _LCXL_IOCP_CMD_H_

#include "lcxl_iocp_lcxl.h"

typedef struct _CMDDataRec {
private:
	ULONG mTotalLen;
	PVOID mTotalData;
	PVOID mData;
	ULONG mDataLen;
public:
	WORD GetCMD();
	void SetCMD(const WORD Value);
	PVOID GetData() {
		return mData;
	}
	ULONG GetDataLen() {
		return mDataLen;
	}
	BOOL Assign(PVOID _TotalData, ULONG _TotalLen);
public:
	friend class CCmdSockObj;
} CMDDataRec, *PCMDDataRec;

class CCmdSockLst :public CLLSockLst {
protected:
	virtual void CreateSockObj(CSocketObj* &SockObj);// 覆盖
public:
	virtual ~CCmdSockLst() {

	}
};

///	<summary>
///	  基于命令的通讯协议Socket类实现
///	</summary>
class CCmdSockObj :public CLLSockObj {
public:
	///	<remarks>
	///	  SendData之前要锁定
	///	</remarks>
	BOOL SendData(const CMDDataRec ASendDataRec);

	///	<remarks>
	///	  SendData之前要锁定
	///	</remarks>
	BOOL SendData(WORD CMD, PVOID Data, ULONG DataLen);

	///	<remarks>
	///	  SendData之前要锁定
	///	</remarks>
	BOOL SendData(WORD CMD, PVOID Data[], ULONG DataLen[], INT DataCount);

	///	<summary>
	///	  获取发送数据的指针
	///	</summary>
	void GetSendData(ULONG DataLen, CMDDataRec &ASendDataRec);

	///	<summary>
	///	  只有没有调用SendData的时候才可以释放，调用SendData之后将会自动释放。
	///	</summary>
	///	<param name="SendDataRec">
	///	  要释放的数据
	///	</param>
	void FreeSendData(const CMDDataRec &ASendDataRec);
	inline static void GetSendDataFromOverlapped(PIOCPOverlapped Overlapped, CMDDataRec &ASendDataRec);
};

class CIOCPCMDList;

// IOCP事件
typedef void (CIOCPCMDList::*EOnIOCPCMDEvent)(IocpEventEnum EventType, CCmdSockObj *SockObj, PIOCPOverlapped Overlapped);
//定义事件触发函数类型
typedef _LCXLFunctionDelegate<CIOCPCMDList, EOnIOCPCMDEvent> DOnIOCPCMDEvent;
// IOCP监听事件
typedef void (CIOCPCMDList::*EOnListenCMDEvent)(ListenEventEnum EventType, CCmdSockLst *SockLst);
//定义事件触发函数类型
typedef _LCXLFunctionDelegate<CIOCPCMDList, EOnListenCMDEvent> DOnListenCMDEvent;

///	<summary>
///	  基于命令的通讯协议Socket类列表的实现
///	</summary>
class CIOCPCMDList :public CCustomIOCPLCXLList{
private:
	DOnIOCPCMDEvent mIOCPEvent;
	DOnListenCMDEvent mListenEvent;
protected:
	/// <summary>
	/// 基类的事件
	/// </summary>
	virtual void OnIOCPEvent(IocpEventEnum EventType, CLLSockObj *SockObj, PIOCPOverlapped Overlapped);
	// 监听事件
	virtual void OnListenEvent(ListenEventEnum EventType, CSocketLst *SockLst);
public:
	// 外部接口
	DOnIOCPCMDEvent GetIOCPEvent() {
		return mIOCPEvent;
	}
	void SetIOCPEvent(const DOnIOCPCMDEvent &Value) {
		mIOCPEvent = Value;
	}
	DOnListenCMDEvent GetListenEvent() {
		return mListenEvent;
	}
	void SetListenEvent(const DOnListenCMDEvent &Value) {
		mListenEvent = Value;
	}
};

#endif