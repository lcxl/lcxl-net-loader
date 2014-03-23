#include "precomp.h"
#include "lcxl_setting.h"

//配置信息
LCXL_SETTING		g_setting;

VOID LoadModuleSetting(IN OUT PLCXL_MODULE_SETTING_INFO module, IN PNDIS_FILTER_ATTACH_PARAMETERS attach_paramters)
{
	//更新module中的信息
	module->miniport_if_index = attach_paramters->BaseMiniportIfIndex;
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

}
