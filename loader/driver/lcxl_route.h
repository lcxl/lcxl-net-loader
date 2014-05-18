#ifndef _LCXL_ROUTE_H_
#define _LCXL_ROUTE_H_

/*
author:
LCXL
abstract:
路由相关头文件
*/

#include "lcxl_server.h"

#define TAG_ROUTE       'ROUT'
//路由信息
typedef struct _LCXL_ROUTE_LIST_ENTRY {
	LIST_ENTRY		        list_entry;	//列表项
#define RS_NONE     0x00				//初始状态
#define RS_NORMAL   0x01				//正常
#define RS_LAST_ACK 0x02				//正在等待最后一个ACK包
#define RS_CLOSED   0x03				//连接已关闭
	int                     status;		//路由状态
	LCXL_IP					src_ip;		//源IP地址
	unsigned short	        src_port;	//源端口号
	unsigned short	        dst_port;	//目的端口号
	PSERVER_INFO_LIST_ENTRY dst_server;	//目标服务器
	LARGE_INTEGER			recv_timestamp;	//时间戳，使用KeQueryPerformanceCounter
} LCXL_ROUTE_LIST_ENTRY, *PLCXL_ROUTE_LIST_ENTRY;

extern NPAGED_LOOKASIDE_LIST  g_route_mem_mgr;

#define InitRouteMemMgr() ExInitializeNPagedLookasideList(&g_route_mem_mgr, NULL, NULL, 0, sizeof(LCXL_ROUTE_LIST_ENTRY), TAG_ROUTE, 0)
__inline PLCXL_ROUTE_LIST_ENTRY AllocRoute()
{
	PLCXL_ROUTE_LIST_ENTRY resu;

	resu = (PLCXL_ROUTE_LIST_ENTRY)ExAllocateFromNPagedLookasideList(&g_route_mem_mgr);
	if (resu != NULL) {
		RtlZeroMemory(resu, sizeof(LCXL_ROUTE_LIST_ENTRY));
		resu->status = RS_NONE;
	}
	return resu;
}
#define FreeRoute(__buf) ExFreeToNPagedLookasideList(&g_route_mem_mgr, __buf)
#define DelRouteMemMgr() ExDeleteNPagedLookasideList(&g_route_mem_mgr)

///<summary>
///创建路由信息表项
///</summary>
PLCXL_ROUTE_LIST_ENTRY CreateRouteListEntry(IN PLIST_ENTRY route_list);

///<summary>
///始化路由信息表项
///</summary>
VOID InitRouteListEntry(IN OUT PLCXL_ROUTE_LIST_ENTRY route_info, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader, IN PSERVER_INFO_LIST_ENTRY server_info);

///<summary>
///获取路由信息项
///</summary>
PLCXL_ROUTE_LIST_ENTRY GetRouteListEntry(IN PLIST_ENTRY route_list, IN INT route_timeout, IN PLCXL_LOCK_LIST server_list, IN INT ip_mode, IN PVOID ip_header, IN PTCP_HDR tcp_header);

#endif