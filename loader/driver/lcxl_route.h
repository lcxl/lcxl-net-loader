#ifndef _LCXL_ROUTE_H_
#define _LCXL_ROUTE_H_

/*
author:
LCX
abstract:
路由相关头文件
*/

#include "lcxl_server.h"

#define TAG_ROUTE       'ROUT'
//路由信息
typedef struct _LCXL_ROUTE_LIST_ENTRY
{
	LIST_ENTRY		        list_entry;		//列表项
#define RS_NONE     0x00
#define RS_NORMAL   0x01					//正常
#define RS_LAST_ACK 0x02					//正在等待最后一个ACK包
#define RS_CLOSED   0x03					//连接已关闭
	int                     status;         //连接状态
	LCXL_IP					src_ip;
	//TCP
	unsigned short	        src_port;		//源端口号
	unsigned short	        dst_port;		//目的端口号
	PSERVER_INFO_LIST_ENTRY dst_server;	    //目标服务器
} LCXL_ROUTE_LIST_ENTRY, *PLCXL_ROUTE_LIST_ENTRY;

extern NPAGED_LOOKASIDE_LIST  g_route_mem_mgr;

#define INIT_ROUTE_MEM_MGR() ExInitializeNPagedLookasideList(&g_route_mem_mgr, NULL, NULL, 0, sizeof(LCXL_ROUTE_LIST_ENTRY), TAG_ROUTE, 0)
#define ALLOC_ROUTE() (PLCXL_ROUTE_LIST_ENTRY)ExAllocateFromNPagedLookasideList(&g_route_mem_mgr)
#define FREE_ROUTE(__buf) ExFreeToNPagedLookasideList(&g_route_mem_mgr, __buf)
#define DEL_ROUTE_MEM_MGR() ExDeleteNPagedLookasideList(&g_route_mem_mgr)


///<summary>
///创建路由信息表项
///</summary>
PLCXL_ROUTE_LIST_ENTRY CreateRouteListEntry(IN PLIST_ENTRY route_list);
///<summary>
///始化路由信息表项
///</summary>
void InitRouteListEntry(IN OUT PLCXL_ROUTE_LIST_ENTRY route_info, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader, IN PSERVER_INFO_LIST_ENTRY server_info);

///<summary>
///获取路由信息项
///</summary>
PLCXL_ROUTE_LIST_ENTRY GetRouteListEntry(IN PLIST_ENTRY route_list, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader);

///<summary>
///获取路由信息项IPv4
///</summary>
__inline  PLCXL_ROUTE_LIST_ENTRY GetRouteListEntry4(IN PLIST_ENTRY route_list, IN PIPV4_HEADER pIPHeader, IN PTCP_HDR pTcpHeader)
{
	return GetRouteListEntry(route_list, IM_IPV4, pIPHeader, pTcpHeader);
}
///<summary>
///获取路由信息项IPv6
///</summary>
__inline PLCXL_ROUTE_LIST_ENTRY GetRouteListEntry6(IN PLIST_ENTRY route_list, IN PIPV6_HEADER pIPHeader, IN PTCP_HDR pTcpHeader)
{
	return GetRouteListEntry(route_list, IM_IPV6, pIPHeader, pTcpHeader);
}

#endif