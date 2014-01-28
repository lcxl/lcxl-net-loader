#ifndef _LCXL_SETTING_H_
#define _LCXL_SETTING_H_
/*
author:
LCX
abstract:
配置相关头文件
*/
#include "lcxl_route.h"
//配置文件
#define LOADER_SETTING_FILE_PATH    L"\\SystemRoot\\System32\\drivers\\etc\\lcxl_loader"
#define TAG_MODULE					'MODU'
//配置文件数据结构
typedef struct _LCXL_MODULE_SETTING_LIST_ENTRY {
	LIST_ENTRY				list_entry;		//列表项
	//ref_count = 0，没有filter和配置文件关联
	//ref_count = 1，有filter和配置文件关联
	LONG					ref_count;
	//重启后删除此设置
#define ML_DELETE_AFTER_RESTART	0x1
	//此配置处于启用状态
#define ML_ENABLED				0x2
	
	//网卡本地唯一ID
	NET_LUID				miniport_net_luid;
	//标识
	INT						flag;
	//小端口驱动友好名称
	PNDIS_STRING			miniport_friendly_name;
	//小端口驱动名称
	PNDIS_STRING			miniport_name;
	//模块名称
	PNDIS_STRING			filter_module_name;
	//真实地址
	LCXL_SERVER_ADDR		real_addr;
	//虚拟IPv4
	IN_ADDR					virtual_ipv4;
	//虚拟IPv6
	IN6_ADDR				virtual_ipv6;
	//服务器数量
	INT						server_count;
	//服务器列表
	SERVER_INFO_LIST_ENTRY	server_list;
	// 服务器列表锁
	FILTER_LOCK             module_setting_lock;
} LCXL_MODULE_SETTING_LIST_ENTRY, *PLCXL_MODULE_SETTING_LIST_ENTRY;

typedef struct _LCXL_SETTING{
	//模块列表锁
	FILTER_LOCK				module_list_lock;
	INT						module_count;
	LCXL_MODULE_SETTING_LIST_ENTRY	module_list;
} LCXL_SETTING, *PLCXL_SETTING;
//!添加代码!

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

extern LCXL_SETTING g_Setting;
#endif


