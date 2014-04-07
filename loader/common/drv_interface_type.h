#ifndef _DRV_INTERFACE_TYPE_H_
#define _DRV_INTERFACE_TYPE_H_

#include "../../common/lcxl_type.h"
#pragma warning(disable:4200)

#ifdef __cplusplus
extern "C" {
#endif
#pragma pack(push,1)
#define MAX_INSTANCE_NAME_LENGTH     256

	//获取所有的网卡接口序号
	//IOCTL_LOADER_ALL_MODULE
	//output

	typedef struct _APP_MODULE {
		//0:小端口驱动ifindex
		NET_IFINDEX			miniport_ifindex;
		//4:网卡本地唯一ID
		NET_LUID			miniport_net_luid;
		//12:虚拟地址IPv4/IPv6
		LCXL_ADDR_INFO		virtual_addr;
		//36:服务器MAC地址
		IF_PHYSICAL_ADDRESS mac_addr;
		//70:模块名称
		WCHAR				filter_module_name[MAX_INSTANCE_NAME_LENGTH];
		//582:小端口驱动友好名称
		WCHAR				miniport_friendly_name[MAX_INSTANCE_NAME_LENGTH];
		//1094:小端口驱动名称
		WCHAR				miniport_name[MAX_INSTANCE_NAME_LENGTH];
		//驱动当前角色，有LCXL_ROLE_ROUTER和LCXL_ROLE_SERVER两种角色
		INT					lcxl_role;
		//1606
		//------------------------LCXL_ROUTER角色------------------------
		//服务器数量
		INT					server_count;
		//------------------------LCXL_SERVER角色------------------------	
		//负载均衡器的mac地址
		IF_PHYSICAL_ADDRESS	router_mac_addr;
	} APP_MODULE, *PAPP_MODULE;

	//IOCTL_SET_VIRTUAL_ADDR
	typedef struct _APP_SET_VIRTUAL_ADDR {
		NET_LUID			miniport_net_luid;
		LCXL_ADDR_INFO		addr;
	} APP_SET_VIRTUAL_ADDR, *PAPP_SET_VIRTUAL_ADDR;
	//服务器列表
	//IOCTL_ROUTER_GET_SERVER_LIST
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

	//在服务器中设置负载均衡器的mac地址
	typedef struct _APP_SET_ROUTER_MAC_ADDR {
		NET_LUID			miniport_net_luid;
		//MAC地址
		IF_PHYSICAL_ADDRESS	mac_addr;
	} APP_SET_ROUTER_MAC_ADDR, *PAPP_SET_ROUTER_MAC_ADDR;
#pragma pack(pop)
#ifdef __cplusplus
}
#endif

#endif