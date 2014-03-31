#ifndef _LCXL_CONFIG_H_
#define _LCXL_CONFIG_H_
#include <WinSock2.h>
#include <Ws2ipdef.h>
#include <string>
#include "../../component/tinyxml2/tinyxml2.h"
#include "../common/drv_interface_type.h"
#include <vector>

#define CONFIG_ROOT_ELEMENT "netloader"
#define CONFIG_VALUE "value"

#define CONFIG_PORT "port"
#define CONFIG_PORT_DEFAULT 32112

#define CONFIG_ROLENAME "rolename"
#define CONFIG_ROLENAME_UNKNOWN "unknown"
#define CONFIG_ROLENAME_ROUTE "router"
#define CONFIG_ROLENAME_SERVER "server"
#define CONFIG_ROLENAME_DEFAULT CONFIG_ROLENAME_UNKNOWN
//CONFIG_MODULE_LIST
#define ELEMENT_MODULE_LIST "module_list"
//CONFIG_MODULE
#define ELEMENT_MODULE "module"
#define ELEMENT_MINIPORT_NET_LUID "miniport_net_luid"
#define ELEMENT_MAC_ADDR "mac_addr"
#define ELEMENT_VIRTUAL_ADDR "virtual_addr"//LCXL_ADDR_INFO
#define ELEMENT_MINIPORT_FRIENDLY_NAME "miniport_friendly_name"
#define ELEMENT_MINIPORT_NAME "miniport_name"
#define ELEMENT_SERVER_LIST "server_list"
#define ELEMENT_ROUTER_MAC_ADDR "router_mac_addr"
//LCXL_ADDR_INFO
#define ELEMENT_STATUS "status"
#define ELEMENT_IPV4 "ipv4"
#define ELEMENT_IPV6 "ipv6"
//LCXL_SERVER
#define ELEMENT_SERVER "server"
// #define ELEMENT_STATUS "status"
#define ELEMENT_IP_STATUS "ip_status"
#define ELEMENT_COMMENT "comment"
//#define ELEMENT_MAC_ADDR "mac_addr"

typedef struct _CONFIG_SERVER {
	LCXL_SERVER server;
	WCHAR		comment[256];//备注名
} CONFIG_SERVER, *PCONFIG_SERVER;

typedef struct _CONFIG_MODULE{
	//驱动模块列表
	APP_MODULE					module;
	//------------------------LCXL_ROUTER角色------------------------
	//服务器列表
	std::vector<CONFIG_SERVER>	server_list;
	//------------------------LCXL_SERVER角色------------------------
	//负载均衡器mac地址
	IF_PHYSICAL_ADDRESS			router_mac_addr;
} CONFIG_MODULE, *PCONFIG_MODULE_INFO;

class CLCXLConfig {
private:
	tinyxml2::XMLDocument			m_XmlDoc;
	int								m_Port;
	std::string						m_Rolename;
	int								m_Role;
	std::vector<CONFIG_MODULE>		m_ModuleList;
public:
	static tinyxml2::XMLElement *WriteModuleList(tinyxml2::XMLElement *owner_element, const std::vector<CONFIG_MODULE> &module_list);
	static tinyxml2::XMLElement *WriteModule(tinyxml2::XMLElement *owner_element, const CONFIG_MODULE &module);
	static tinyxml2::XMLElement *WriteAddrInfo(tinyxml2::XMLElement *owner_element, const LCXL_ADDR_INFO &addr);
	static tinyxml2::XMLElement *WriteServerList(tinyxml2::XMLElement *owner_element, const std::vector<CONFIG_SERVER> &server_list);
	static tinyxml2::XMLElement *WriteServer(tinyxml2::XMLElement *owner_element, const CONFIG_SERVER &server);
public:
	static std::vector<CONFIG_MODULE> & ReadModuleList(tinyxml2::XMLElement *owner_element, std::vector<CONFIG_MODULE> &module_list);
	static CONFIG_MODULE & ReadModule(tinyxml2::XMLElement *owner_element, CONFIG_MODULE &module);
	static LCXL_ADDR_INFO & ReadAddrInfo(tinyxml2::XMLElement *owner_element, LCXL_ADDR_INFO &addr);
	static std::vector<CONFIG_SERVER> & ReadServerList(tinyxml2::XMLElement *owner_element, std::vector<CONFIG_SERVER> &server_list);
	static CONFIG_SERVER & ReadServer(tinyxml2::XMLElement *owner_element, CONFIG_SERVER &server);
public:
	CLCXLConfig();
	bool SaveXMLFile(std::string XmlFilePath);
	bool LoadXMLFile(std::string XmlFilePath);

	int GetPort() const { return m_Port; }

	std::string GetRolename() const { return m_Rolename; }
	void SetRolename(const std::string val);
	
	int GetRole() const { return m_Role; }
	void SetRole(int val);
	///将驱动的module同步到配置文件中
	void UpdateModuleList(const std::vector<APP_MODULE> &module_list);
	std::vector<CONFIG_MODULE> &ModuleList() { return m_ModuleList; }
};

#endif