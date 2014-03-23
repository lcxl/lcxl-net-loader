#ifndef _LCXL_CONFIG_H_
#define _LCXL_CONFIG_H_
#include <WinSock2.h>
#include <Ws2ipdef.h>
#include <string>
#include "../../component/tinyxml2/tinyxml2.h"
#include "../common/drv_interface_type.h"
#include <vector>

#define CONFIG_ROOT_ELEMENT "lcxlnetloader"
#define CONFIG_VALUE "value"

#define CONFIG_PORT "port"
#define CONFIG_PORT_DEFAULT 32112

#define CONFIG_ROLENAME "rolename"
#define CONFIG_ROLENAME_UNKNOWN "unknown"
#define CONFIG_ROLENAME_LOADER "loader"
#define CONFIG_ROLENAME_SERVER "server"
#define CONFIG_ROLENAME_DEFAULT CONFIG_ROLENAME_UNKNOWN

typedef struct _CONFIG_MODULE_INFO {
	//是否启用
	BOOL					enabled;
	//网卡本地唯一ID
	NET_LUID				miniport_net_luid;
	//虚拟IPv4
	IN_ADDR					virtual_ipv4;
	//虚拟IPv6
	IN6_ADDR				virtual_ipv6;
	//服务器列表
	std::vector<LCXL_ADDR_INFO>	server_list;
} CONFIG_MODULE_INFO, *PCONFIG_MODULE_INFO;

class CLCXLConfig {
private:
	tinyxml2::XMLDocument		m_XmlDoc;
	int							m_Port;
	std::string					m_RoleName;
	int							m_Role;
	std::vector<CONFIG_MODULE_INFO>	m_ModuleList;
	
public:
	bool SaveXMLFile(std::string XmlFilePath);
	bool LoadXMLFile(std::string XmlFilePath);

	int GetPort() const { return m_Port; }

	std::string GetRoleName() const { return m_RoleName; }
	void SetRoleName(const std::string val);
	
	int GetRole() const { return m_Role; }
	void SetRole(int val);

	std::vector<CONFIG_MODULE_INFO> &ModuleList() { return m_ModuleList; }
};

#endif