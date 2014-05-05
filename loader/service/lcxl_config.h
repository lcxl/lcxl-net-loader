#ifndef _LCXL_CONFIG_H_
#define _LCXL_CONFIG_H_
#include <WinSock2.h>
#include <Ws2ipdef.h>
#include <string>
#include "../../component/tinyxml2/tinyxml2.h"
#include "../common/drv_interface_type.h"
#include <vector>
#include "lcxl_func.h"

#define ELEMENT_ROOT "netloader"
#define ATTRIBUTE_VALUE "value"

#define ELEMENT_PORT "port"
#define VALUE_PORT_DEFAULT 32112

#define ELEMENT_LCXL_ROLE "lcxl_role"

#define ELEMENT_ROLENAME "rolename"
#define VALUE_ROLENAME_UNKNOWN "unknown"
#define VALUE_ROLENAME_ROUTE "router"
#define VALUE_ROLENAME_SERVER "server"
#define VALUE_ROLENAME_DEFAULT VALUE_ROLENAME_UNKNOWN
//CONFIG_MODULE_LIST
#define ELEMENT_MODULE_LIST "module_list"
//CONFIG_MODULE
#define ELEMENT_MODULE "module"
#define ELEMENT_MINIPORT_NET_LUID "miniport_net_luid"
#define ELEMENT_MAC_ADDR "mac_addr"
#define ELEMENT_VIRTUAL_ADDR "virtual_addr"//LCXL_ADDR_INFO
#define ELEMENT_FILTER_MODULE_NAME "filter_module_name"
#define ELEMENT_MINIPORT_FRIENDLY_NAME "miniport_friendly_name"
#define ELEMENT_MINIPORT_IFINDEX "miniport_ifindex"
#define ELEMENT_MINIPORT_NAME "miniport_name"
#define ELEMENT_SERVER_LIST "server_list"
#define ELEMENT_SERVER_COUNT "server_count"
#define ELEMENT_ROUTE_TIMEOUT "route_timeout"
#define ELEMENT_SERVER_CHECK "server_check"//LCXL_SERVER_CHECK
#define ELEMENT_ROUTING_ALGORITHM "routing_algorithm"

//LCXL_ADDR_INFO
#define ELEMENT_STATUS "status"
#define ELEMENT_IPV4 "ipv4"
#define ELEMENT_IPV4_ONLINK_PREFIX_LENGTH "ipv4_onlink_prefix_length"
#define ELEMENT_IPV6 "ipv6"
#define ELEMENT_IPV6_ONLINK_PREFIX_LENGTH "ipv6_onlink_prefix_length"
//LCXL_SERVER_CHECK
#define ELEMENT_INTERVAL "interval"
#define ELEMENT_TIMEOUT "timeout"
#define ELEMENT_RETRY_NUMBER "retry_number"
//LCXL_SERVER
#define ELEMENT_SERVER "server"
// #define ELEMENT_STATUS "status"
#define ELEMENT_IP_STATUS "ip_status"
#define ELEMENT_COMMENT "comment"
//#define ELEMENT_MAC_ADDR "mac_addr"

typedef struct _CONFIG_SERVER {
	//服务器信息
	LCXL_SERVER server;
	//备注名
	WCHAR		comment[256];
} CONFIG_SERVER, *PCONFIG_SERVER;

typedef struct _CONFIG_MODULE{
	//网卡是否存在，如果不存在，则表明可能被禁用或者从系统中移除
	bool						isexist;
	//驱动模块列表
	APP_MODULE					module;
	//------------------------LCXL_ROUTER角色------------------------
	//服务器列表
	std::vector<CONFIG_SERVER>	server_list;
	HANDLE						thread_handle;
	HANDLE						exit_event;
	//------------------------LCXL_SERVER角色------------------------
} CONFIG_MODULE, *PCONFIG_MODULE;


class CLCXLConfig {
private:
	tinyxml2::XMLDocument			m_XmlDoc;
	int								m_Port;
	std::vector<CONFIG_MODULE>		m_ModuleList;
	CRITICAL_SECTION				m_CriticalSection;
public:
	//************************************
	// 简介: 将字符串转换为角色类型
	// 返回: int
	// 参数: const std::string val
	//************************************
	static int StrToRole(const std::string val);

	//************************************
	// 简介: 将角色类型转换为字符串
	// 返回: std::string
	// 参数: int role
	//************************************
	static std::string RoleToStr(int role);
public:
	static tinyxml2::XMLElement *WriteModuleList(tinyxml2::XMLElement *owner_element, const std::vector<CONFIG_MODULE> &module_list);
	static tinyxml2::XMLElement *WriteModule(tinyxml2::XMLElement *owner_element, const CONFIG_MODULE &module);
	static tinyxml2::XMLElement *WriteAddrInfo(tinyxml2::XMLElement *owner_element, const LCXL_ADDR_INFO &addr);
	static tinyxml2::XMLElement *WriteServerCheck(tinyxml2::XMLElement *owner_element, const LCXL_SERVER_CHECK &server_check);
	static tinyxml2::XMLElement *WriteServerList(tinyxml2::XMLElement *owner_element, const std::vector<CONFIG_SERVER> &server_list);
	static tinyxml2::XMLElement *WriteServer(tinyxml2::XMLElement *owner_element, const CONFIG_SERVER &server);
public:
	
	static std::vector<CONFIG_MODULE> & ReadModuleList(tinyxml2::XMLElement *owner_element, std::vector<CONFIG_MODULE> &module_list);
	static CONFIG_MODULE & ReadModule(tinyxml2::XMLElement *owner_element, CONFIG_MODULE &module);
	static LCXL_ADDR_INFO & ReadAddrInfo(tinyxml2::XMLElement *owner_element, LCXL_ADDR_INFO &addr);
	static LCXL_SERVER_CHECK &ReadServerCheck(tinyxml2::XMLElement *owner_element, LCXL_SERVER_CHECK &server_check);
	static std::vector<CONFIG_SERVER> & ReadServerList(tinyxml2::XMLElement *owner_element, std::vector<CONFIG_SERVER> &server_list);
	static CONFIG_SERVER & ReadServer(tinyxml2::XMLElement *owner_element, CONFIG_SERVER &server);
public:
	CLCXLConfig();
	~CLCXLConfig();

	//************************************
	// 简介: 在返回值的生存周期期间加锁，返回值销毁时自动解锁
	// 返回: CCSLocker
	//************************************
	CCSLocker LockinLifeCycle();

	bool SaveXMLFile(std::string XmlFilePath);
	bool LoadXMLFile(std::string XmlFilePath);

	int GetPort() const { return m_Port; }

	///将驱动的module同步到配置文件中
	void UpdateModuleList(const std::vector<APP_MODULE> &module_list);
	std::vector<CONFIG_MODULE> &ModuleList() { return m_ModuleList; }
	CONFIG_MODULE *FindModuleByLuid(NET_LUID miniport_net_luid);
};

#endif