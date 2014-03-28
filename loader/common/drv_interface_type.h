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

	typedef struct _APP_MODULE {
		//小端口驱动ifindex
		NET_IFINDEX			miniport_ifindex;
		//网卡本地唯一ID
		NET_LUID			miniport_net_luid;
		//虚拟地址IPv4/IPv6
		LCXL_ADDR_INFO		virtual_addr;
		//服务器MAC地址
		IF_PHYSICAL_ADDRESS mac_addr;
		//模块名称
		WCHAR				filter_module_name[MAX_INSTANCE_NAME_LENGTH];
		//小端口驱动友好名称
		WCHAR				miniport_friendly_name[MAX_INSTANCE_NAME_LENGTH];
		//小端口驱动名称
		WCHAR				miniport_name[MAX_INSTANCE_NAME_LENGTH];
		union {
			struct {
				//服务器数量
				INT			server_count;
			} loader;
			struct  {
				INT			reserve;
			} server;
		} role;
	} APP_MODULE, *PAPP_MODULE;

	//IOCTL_LOADER_SET_VIRTUAL_IP
	typedef struct _APP_VIRTUAL_IP {
		NET_LUID			miniport_net_luid;
		LCXL_ADDR_INFO		addr;
	} APP_VIRTUAL_IP, *PAPP_VIRTUAL_IP;
	//服务器列表
	//IOCTL_LOADER_GET_SERVER_LIST
	//input NET_LUID                     miniport_net_luid;
	//output array of LCXL_SERVER
	//添加服务器
	typedef struct _APP_ADD_SERVER {
		NET_LUID			miniport_net_luid;
		LCXL_SERVER			server;
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