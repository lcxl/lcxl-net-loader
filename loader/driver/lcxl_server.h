#ifndef _LCXL_SERVER_H_
#define _LCXL_SERVER_H_
/*
author:
LCX
abstract:
服务器相关头文件
*/
#include "../../common/driver/ref_lock_list.h"
#include "../../common/kalman.h"
#define TAG_SERVER      'SERV'


typedef struct _SERVER_INFO_LIST_ENTRY
{
	//列表项
	REF_LIST_ENTRY			list_entry;
	//服务器锁
	KSPIN_LOCK				lock;
	//服务器信息
	LCXL_SERVER				info;
	//服务器性能信息
	LCXL_SERVER_PERFORMANCE	performance;
	//时间戳，服务器可用性检测，使用KeQueryPerformanceCounter
	LARGE_INTEGER			timestamp;	
	//当检测服务器可用性失败时，当前重试次数
	INT						current_retry_number;
} SERVER_INFO_LIST_ENTRY, *PSERVER_INFO_LIST_ENTRY;//服务器信息列表项

//负载均衡算法用到的数据结构
typedef struct _ROUTING_ALGORITHM_DATA{
	
	struct {
		INT		current_server_index;//了目前轮询到的服务器的序号
	} ra_poll;//启用RA_POLL负载均衡算法时所用到的数据
	struct {

		KALMAN_STATE kalman_state;//kalman状态
	} ra_fast_respnse; //启用RA_FAST_RESPONSE负载均衡算法时所用到的数据
} ROUTING_ALGORITHM_DATA, *PROUTING_ALGORITHM_DATA;

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
		resu->current_retry_number = 0;
		resu->timestamp = KeQueryPerformanceCounter(NULL);
		
	}
	return resu;
}
//锁定服务器
//参数:
//server:PSERVER_INFO_LIST_ENTRY，服务器数据结构
//lock_handle_in_stack:PKLOCK_QUEUE_HANDLE，锁结构，此结构必须处于栈中 
__inline VOID LockServer(IN PSERVER_INFO_LIST_ENTRY server, OUT PKLOCK_QUEUE_HANDLE lock_handle_in_stack)
{
	ASSERT(server != NULL && lock_handle_in_stack != NULL); 
	KeAcquireInStackQueuedSpinLock(&server->lock, lock_handle_in_stack);
}

//解锁服务器
//参数:
//__LockHandleInStack:PKLOCK_QUEUE_HANDLE ，锁结构，此结构必须处于栈中 
__inline VOID  UnLockServer(IN PKLOCK_QUEUE_HANDLE lock_handle_in_stack)
{	
	ASSERT(lock_handle_in_stack != NULL); 
	KeReleaseInStackQueuedSpinLock(lock_handle_in_stack);
}
//释放服务器
__inline VOID FreeServer(PSERVER_INFO_LIST_ENTRY server)
{
	ASSERT(server->list_entry.ref_count == 0);
	ExFreeToNPagedLookasideList(&g_server_mem_mgr, server);
}
#define DelServerMemMgr() ExDeleteNPagedLookasideList(&g_server_mem_mgr)

//************************************
// 简介: 删除配置信息回调函数
// 返回: VOID
// 参数: PLIST_ENTRY server
//************************************
VOID DelServerCallBack(PLIST_ENTRY server);

//************************************
// 简介: 通过列表项获取服务器信息
// 返回: PSERVER_INFO_LIST_ENTRY
// 参数: PLIST_ENTRY server_list_entry
//************************************
__inline PSERVER_INFO_LIST_ENTRY GetServerbyListEntry(PLIST_ENTRY server_list_entry)
{
	return CONTAINING_RECORD(GetRefListEntry(server_list_entry), SERVER_INFO_LIST_ENTRY, list_entry);
}

//************************************
// 简介: 根据MAC地址寻找服务器，注意：使用FindServer之前需要锁定列表
// 返回: PSERVER_INFO_LIST_ENTRY
// 参数: IN PLCXL_LOCK_LIST server_list
// 参数: IN PIF_PHYSICAL_ADDRESS mac_addr
//************************************
PSERVER_INFO_LIST_ENTRY FindServer(IN PLCXL_LIST server_list, IN PIF_PHYSICAL_ADDRESS mac_addr);

//************************************
// 简介: 服务是否可用
// 返回: BOOLEAN
// 参数: IN PSERVER_INFO_LIST_ENTRY server_info
// 参数: IN INT ip_mode
//************************************
__inline BOOLEAN ServerIsAvaliable(IN PSERVER_INFO_LIST_ENTRY server_info, IN INT ip_mode)
{
	return (server_info->info.status&SS_ONLINE) != 0 &&
		(
		(server_info->info.ip_status&SA_ENABLE_IPV6) && (ip_mode == IM_IPV6) ||
		(server_info->info.ip_status&SA_ENABLE_IPV4) && (ip_mode == IM_IPV4)
		) &&
		(server_info->info.status&SS_DELETED) == 0;
}


//************************************
// 简介: 从当前服务器开始寻找可用的服务器
// 返回: PSERVER_INFO_LIST_ENTRY
// 参数: IN PLCXL_LOCK_LIST server_list
// 参数: IN PSERVER_INFO_LIST_ENTRY server
// 参数: IN INT ip_mode
// 参数: IN BOOLEAN checking_last 如果此值为TRUE，则优先选择状态不为checking的服务器，否则checking服务器和正常的服务器具有一样的优先权
//************************************
PSERVER_INFO_LIST_ENTRY FindAvaliableServerFormCurrentServer(IN PLCXL_LIST server_list, IN PSERVER_INFO_LIST_ENTRY server, IN INT ip_mode, IN BOOLEAN checking_last);

//************************************
// 简介: 选择一台最适合的服务器
// 返回: PSERVER_INFO_LIST_ENTRY
// 参数: IN PLCXL_LOCK_LIST server_list
// 参数: IN INT ip_mode
// 参数: IN PVOID ip_header
// 参数: IN PTCP_HDR tcp_header
//************************************
PSERVER_INFO_LIST_ENTRY SelectBestServer(IN PLCXL_LIST server_list, IN INT ip_mode, IN PVOID ip_header, IN PTCP_HDR tcp_header, IN INT	routing_algorithm, IN PROUTING_ALGORITHM_DATA ra_data);

#endif