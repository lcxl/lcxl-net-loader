#ifndef _DRV_INTERFACE_TYPE_H_
#define _DRV_INTERFACE_TYPE_H_

#include "../../common/lcxl_type.h"
#pragma warning(disable:4200)

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_INSTANCE_NAME_LENGTH     256

	//获取所有的网卡接口序号
	//IOCTL_LOADER_ALL_MODULE
	//output

	typedef struct _APP_MODULE_INFO {
#define AMS_NONE		0x00
#define AMS_NORMAL		0x01
#define AMS_NO_FILTER	0x02
		//模块模式
		INT					app_module_status;
		//小端口驱动ifindex
		NET_IFINDEX			miniport_if_index;
		//网卡本地唯一ID
		NET_LUID			miniport_net_luid;
		//真实地址
		LCXL_ADDR_INFO		real_addr;
		//虚拟IPv4
		IN_ADDR				virtual_ipv4;
		//虚拟IPv6
		IN6_ADDR			virtual_ipv6;
		//模块名称
		USHORT				filter_module_name_len;
		WCHAR				filter_module_name[MAX_INSTANCE_NAME_LENGTH];
		//小端口驱动友好名称
		USHORT				miniport_friendly_name_len;
		WCHAR				miniport_friendly_name[MAX_INSTANCE_NAME_LENGTH];
		//小端口驱动名称
		USHORT				miniport_name_len;
		WCHAR				miniport_name[MAX_INSTANCE_NAME_LENGTH];
		union _ROLE_UNION {
			
			struct _ROLE_LOADER {
				//服务器数量
				INT			server_count;
			} loader;
			struct _ROLE_SERVER {
				INT			reserve;
			} server;
		} role;
	} APP_MODULE_INFO, *PAPP_MODULE_INFO;

	//IOCTL_LOADER_SET_VIRTUAL_IP
	typedef struct _APP_IP {
		NET_LUID	miniport_net_luid;
		LCXL_IP		ip;
	} APP_IP, *PAPP_IP;
	//服务器列表
	//IOCTL_LOADER_GET_SERVER_LIST
	//input NET_LUID                     miniport_net_luid;
	//output
	typedef struct _APP_SERVER_LIST {
		int					server_count;//服务器数量
		LCXL_ADDR_INFO	server_list[0];//服务器地址
	} APP_SERVER_LIST, *PAPP_SERVER_LIST;

	//添加服务器
	typedef struct _APP_ADD_SERVER {
		NET_LUID			miniport_net_luid;
		LCXL_ADDR_INFO	server;
	} APP_ADD_SERVER, *PAPP_ADD_SERVER;
	//删除服务器
	typedef struct _APP_DEL_SERVER {
		NET_LUID			miniport_net_luid;
		//MAC地址
		IF_PHYSICAL_ADDRESS	mac_addr;
	} APP_DEL_SERVER, *PAPP_DEL_SERVER;
#ifdef __cplusplus
}
#endif

#endif