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
//配置文件
#define LOADER_SETTING_FILE_PATH    L"\\SystemRoot\\System32\\drivers\\etc\\lcxl_loader"
#define TAG_MODULE					'MODU'
//配置文件数据结构
typedef struct _LCXL_MODULE_SETTING_LIST_ENTRY {
	LIST_ENTRY			list_entry;		//列表项
	//ref_count = 0，没有filter和配置文件关联
	//ref_count = 1，有filter和配置文件关联
	LONG				ref_count;

	
	//网卡本地唯一ID
	NET_LUID			miniport_net_luid;
	//重启后删除此设置
#define MSF_DELETE_AFTER_RESTART	0x1
	//此配置处于启用状态
#define MSF_ENABLED					0x2
	//标识
	INT					flag;
	//小端口驱动友好名称
	PNDIS_STRING		miniport_friendly_name;
	//小端口驱动名称
	PNDIS_STRING		miniport_name;
	//模块名称
	PNDIS_STRING		filter_module_name;
	//真实地址
	LCXL_SERVER_ADDR	real_addr;
	//虚拟IPv4
	IN_ADDR				virtual_ipv4;
	//虚拟IPv6
	IN6_ADDR			virtual_ipv6;
	//服务器列表
	LCXL_LOCK_LIST		server_list;//SERVER_INFO_LIST_ENTRY
	// 服务器列表锁
	FILTER_LOCK			lock;
} LCXL_MODULE_SETTING_LIST_ENTRY, *PLCXL_MODULE_SETTING_LIST_ENTRY;

typedef struct _LCXL_SETTING{
	//模块列表锁
	FILTER_LOCK			lock;

	LCXL_LOCK_LIST		module_list;//LCXL_MODULE_SETTING_LIST_ENTRY
} LCXL_SETTING, *PLCXL_SETTING;



//删除配置信息回调函数
VOID DelModuleSettingCallBack(PLIST_ENTRY module_setting);

///<summary>
///加载配置文件
///</summary>
VOID LoadSetting();
//申请一个配置信息内存
PLCXL_MODULE_SETTING_LIST_ENTRY NewModuleSetting();
//释放配置信息内存
VOID DelModuleSetting(IN PLCXL_MODULE_SETTING_LIST_ENTRY module_setting);

PLCXL_MODULE_SETTING_LIST_ENTRY FindModuleSettingByLUID(IN NET_LUID miniport_net_luid);

//从设置模块中加载设置
PLCXL_MODULE_SETTING_LIST_ENTRY LoadModuleSetting(IN PNDIS_FILTER_ATTACH_PARAMETERS	attach_paramters);

///<summary>
///保存配置文件
///</summary>
VOID SaveSetting();

//减少引用并且检查是否可以释放SERVER结构
__inline LONG DecRefServerAndCheckIfCanDel(IN PLCXL_LOCK_LIST server_list, IN PSERVER_INFO_LIST_ENTRY server)
{
	LONG ref_count;

	ref_count = DecRefServer(server);
	if (ref_count <= 0) {
		ASSERT((server->info.status & SS_DELETED) != 0);
		DelFromLCXLLockList(server_list, &server->list_entry);
	}
	return ref_count;
}

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
	DecRefServerAndCheckIfCanDel(server_list, server);
}


extern LCXL_SETTING g_setting;
#endif


