#ifndef _LCXL_SETTING_H_
#define _LCXL_SETTING_H_
/*
author:
LCX
abstract:
配置相关头文件
*/
#include "lcxl_route.h"
#include "../../common/driver/lcxl_lock_list.h"

#define TAG_MODULE					'MODU'

typedef struct _LCXL_MODULE_SETTING_INFO {
	
	//ifindex
	NET_IFINDEX				miniport_ifindex;
	//网卡本地唯一ID
	NET_LUID				miniport_net_luid;
	//小端口驱动友好名称
	PNDIS_STRING			miniport_friendly_name;
	//小端口驱动名称
	PNDIS_STRING			miniport_name;
	//模块名称
	PNDIS_STRING			filter_module_name;
	//MAC地址
	IF_PHYSICAL_ADDRESS		mac_addr;
	//驱动当前角色，有LCXL_ROLE_ROUTER和LCXL_ROLE_SERVER两种角色
	INT						lcxl_role;
	//虚拟地址IPv4/IPv6(非多线程安全)
	LCXL_ADDR_INFO			virtual_addr;
	//------------------------LCXL_ROUTER角色------------------------
	//服务器列表，SERVER_INFO_LIST_ENTRY
	LCXL_LOCK_LIST			server_list;
	//路由表超时时间，以秒为单位
	INT						route_timeout;
	//服务器检测信息
	LCXL_SERVER_CHECK		server_check;

	//路由算法，有RA_....
	INT						routing_algorithm;
	//------------------------LCXL_SERVER角色------------------------

} LCXL_MODULE_SETTING_INFO, *PLCXL_MODULE_SETTING_INFO;//配置模块数据结构

typedef struct _LCXL_SETTING{
	LARGE_INTEGER		frequency;//KeQueryPerformanceCounter的每秒时钟频率，用于计算路由表的生存周期
} LCXL_SETTING, *PLCXL_SETTING;//驱动设置

//删除路由信息
//route_info:路由信息
//server_list:路由信息所在的服务器
__inline VOID DeleteRouteListEntry(IN OUT PLCXL_ROUTE_LIST_ENTRY route_info, IN PLCXL_LOCK_LIST server_list)
{
	PSERVER_INFO_LIST_ENTRY server;

	server = route_info->dst_server;
	RemoveEntryList(&route_info->list_entry);
	FreeRoute(route_info);
	//将路由所在的服务器的引用减1
	DecRefListEntry(server_list, &server->list_entry);
}
VOID InitModuleSetting(IN OUT PLCXL_MODULE_SETTING_INFO module, IN PNDIS_FILTER_ATTACH_PARAMETERS attach_paramters);

extern LCXL_SETTING g_setting;
#endif


