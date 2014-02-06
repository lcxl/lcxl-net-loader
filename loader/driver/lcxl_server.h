#ifndef _LCXL_SERVER_H_
#define _LCXL_SERVER_H_
/*
author:
LCX
abstract:
服务器相关头文件
*/
#include "../../common/driver/ref_lock_list.h"
#define TAG_SERVER      'SERV'

//服务器信息列表项
typedef struct _SERVER_INFO_LIST_ENTRY
{
	//列表项
	REF_LIST_ENTRY			list_entry;
	//锁
	KSPIN_LOCK				lock;
	//服务器状态
	LCXL_SERVER_INFO		info;
	//服务器性能状态
	LCXL_SERVER_PERFORMANCE	performance;
} SERVER_INFO_LIST_ENTRY, *PSERVER_INFO_LIST_ENTRY;

extern NPAGED_LOOKASIDE_LIST  g_server_mem_mgr;

#define InitServerMemMgr() ExInitializeNPagedLookasideList(&g_server_mem_mgr, NULL, NULL, 0, sizeof(SERVER_INFO_LIST_ENTRY), TAG_SERVER, 0)
__inline PSERVER_INFO_LIST_ENTRY AllocServer()
{
	PSERVER_INFO_LIST_ENTRY resu;
	resu = (PSERVER_INFO_LIST_ENTRY)ExAllocateFromNPagedLookasideList(&g_server_mem_mgr);
	if (resu != NULL) {
		RtlZeroMemory(resu, sizeof(SERVER_INFO_LIST_ENTRY));
		InitializeListHead(&resu->list_entry.list_entry);
		KeInitializeSpinLock(&resu->lock);
		resu->list_entry.ref_count = 1;
	}
	return resu;
}
//锁定服务器
//参数:
//__SER:PSERVER_INFO_LIST_ENTRY，服务器数据结构
//__LockHandleInStack:PKLOCK_QUEUE_HANDLE，锁结构，此结构必须处于栈中 
__inline VOID LockServer(IN PSERVER_INFO_LIST_ENTRY __SER, OUT PKLOCK_QUEUE_HANDLE __LockHandleInStack)
{
	ASSERT(__SER != NULL && __LockHandleInStack != NULL); 
	KeAcquireInStackQueuedSpinLock(&__SER->lock, __LockHandleInStack);
}

//解锁服务器
//参数:
//__LockHandleInStack:PKLOCK_QUEUE_HANDLE ，锁结构，此结构必须处于栈中 
__inline VOID  UnLockServer(IN PKLOCK_QUEUE_HANDLE __LockHandleInStack)
{	
	ASSERT(__LockHandleInStack != NULL); 
	KeReleaseInStackQueuedSpinLock(__LockHandleInStack);
}
//释放服务器
__inline VOID FreeServer(PSERVER_INFO_LIST_ENTRY server)
{
	ASSERT(server->list_entry.ref_count == 0);
	ExFreeToNPagedLookasideList(&g_server_mem_mgr, server);
}
#define DelServerMemMgr() ExDeleteNPagedLookasideList(&g_server_mem_mgr)

//删除配置信息回调函数
VOID DelServerCallBack(PLIST_ENTRY server);
///<summary>
//选择服务器
///</summary>
PSERVER_INFO_LIST_ENTRY SelectBestServer(IN PLCXL_LOCK_LIST server_list, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader);

#endif