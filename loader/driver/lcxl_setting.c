#include "precomp.h"
#include "lcxl_setting.h"

//配置信息
LCXL_SETTING		g_setting;

VOID InitModuleSetting(IN OUT PLCXL_MODULE_SETTING_INFO module, IN PNDIS_FILTER_ATTACH_PARAMETERS attach_paramters)
{
	//更新module中的信息
	module->miniport_ifindex = attach_paramters->BaseMiniportIfIndex;
	module->miniport_net_luid = attach_paramters->BaseMiniportNetLuid;
	//保存MAC地址（一个问题，当用户手动修改了MAC地址，会怎样- -）
	module->mac_addr.Length = sizeof(module->mac_addr.Address) < attach_paramters->MacAddressLength ? sizeof(module->mac_addr.Address) : attach_paramters->MacAddressLength;
	NdisMoveMemory(module->mac_addr.Address, attach_paramters->CurrentMacAddress, module->mac_addr.Length);

	//更新小端口驱动相关信息
	LCXLFreeString(module->miniport_friendly_name);
	module->miniport_friendly_name = LCXLNewString(attach_paramters->BaseMiniportInstanceName);
	LCXLFreeString(module->miniport_name);
	module->miniport_name = LCXLNewString(attach_paramters->BaseMiniportName);
	LCXLFreeString(module->filter_module_name);
	module->filter_module_name = LCXLNewString(attach_paramters->FilterModuleGuidName);
	//设置路由项超时时间，默认20分钟
	module->route_timeout = 1200;
	//设置服务器检测间隔，默认3秒
	module->server_check_interval = 3;
	//服务器检测超时时间，默认5秒
	module->server_check_timeout = 5;
	//服务器检测失败时的重试次数，默认1次
	module->server_check_retry_number = 1;
	//设置负载均衡算法为最小连接数
	module->routing_algorithm = RA_LEAST_CONNECTION;
}
