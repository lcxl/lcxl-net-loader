#include "lcxl_config.h"
#include <tchar.h>
#include <Ws2tcpip.h>

#include "../../component/lcxl_iocp/lcxl_string.h"
#ifdef _DEBUG
void OutputDebugStr(const TCHAR fmt[], ...);
#else
#define OutputDebugStr(__fmt, ...)
#endif // _DEBUG

bool CLCXLConfig::LoadXMLFile(std::string XmlFilePath)
{
	CCSLocker locker = LockinLifeCycle();
	tinyxml2::XMLElement *root_element;
	//加载文件
	if (m_XmlDoc.LoadFile(XmlFilePath.c_str()) == 0) {
		root_element = m_XmlDoc.RootElement();
		if (root_element == NULL) {
			OutputDebugStr(_T("Invalid XML file\n"));
			return false;
		}
		//验证根元素名称是否正确
		std::string root_ele_name = m_XmlDoc.RootElement()->Name();
		if (string_tolower(root_ele_name) != ELEMENT_ROOT) {
			OutputDebugStr(_T("The root element of the xml file is not '%s'\n"), wstring_to_tstring(std::string(ELEMENT_ROOT)).c_str());
			return false;
		}
	} else {
		return true;
	}
	tinyxml2::XMLElement *element;
	//读取监听端口
	element = root_element->FirstChildElement(ELEMENT_PORT);
	if (element != NULL) {
		m_Port = element->IntAttribute(ATTRIBUTE_VALUE);
		if (m_Port <= 0 || m_Port >= 65535) {
			OutputDebugStr(_T("port=%d is invalid.\n"), m_Port);
			return false;
		}
	}
	//读取模块列表
	element = root_element->FirstChildElement(ELEMENT_MODULE_LIST);
	if (element != NULL) {
		ReadModuleList(element, m_ModuleList);
	}
	return true;
}

bool CLCXLConfig::SaveXMLFile(std::string XmlFilePath)
{
	CCSLocker locker = LockinLifeCycle();
	m_XmlDoc.Clear();
	m_XmlDoc.InsertEndChild(m_XmlDoc.NewDeclaration());
	m_XmlDoc.SetBOM(true);

	tinyxml2::XMLNode *root_element;
	
	//创建根元素
	root_element = m_XmlDoc.InsertEndChild(m_XmlDoc.NewElement(ELEMENT_ROOT));

	tinyxml2::XMLElement *element;
	//写入端口
	element = m_XmlDoc.NewElement(ELEMENT_PORT);
	element->SetAttribute(ATTRIBUTE_VALUE, m_Port);
	root_element->InsertEndChild(element);
	//创建模块列表
	root_element->InsertEndChild(WriteModuleList(m_XmlDoc.NewElement(ELEMENT_MODULE_LIST), m_ModuleList));
	return m_XmlDoc.SaveFile(XmlFilePath.c_str()) == tinyxml2::XML_NO_ERROR;
}

void CLCXLConfig::UpdateModuleList(const std::vector<APP_MODULE> &module_list)
{
	//在locker生存周期中锁定
	CCSLocker locker = LockinLifeCycle();

	std::vector<CONFIG_MODULE>::iterator cit;
	std::vector<APP_MODULE>::const_iterator ait;

	for (cit = m_ModuleList.begin(); cit != m_ModuleList.end(); cit++) {
		(*cit).isexist = false;
		for (ait = module_list.cbegin(); ait != module_list.cend(); ait++) {
			if ((*ait).miniport_net_luid.Value == (*cit).module.miniport_net_luid.Value) {
				//更新CONFIG_MODULE中的数据，以下数据以系统为准
				(*cit).module.miniport_ifindex = (*ait).miniport_ifindex;
				if ((*ait).lcxl_role != LCXL_ROLE_UNKNOWN) {
					(*cit).module.lcxl_role = (*ait).lcxl_role;
				}
				CopyMemory(&(*cit).module.mac_addr, &(*ait).mac_addr, sizeof((*cit).module.mac_addr));
				wcscpy_s((*cit).module.miniport_friendly_name, sizeof((*cit).module.miniport_friendly_name) / sizeof(WCHAR), (*ait).miniport_friendly_name);
				wcscpy_s((*cit).module.miniport_name, sizeof((*cit).module.miniport_name) / sizeof(WCHAR), (*ait).miniport_name);
				wcscpy_s((*cit).module.filter_module_name, sizeof((*cit).module.filter_module_name) / sizeof(WCHAR), (*ait).filter_module_name);
				(*cit).isexist = true;
				break;
			}
		}
	}

	for (ait = module_list.cbegin(); ait != module_list.cend(); ait++) {
		bool isexist = false;
		for (cit = m_ModuleList.begin(); cit != m_ModuleList.end(); cit++) {
			if ((*ait).miniport_net_luid.Value == (*cit).module.miniport_net_luid.Value) {
				isexist = true;
				break;
			}
		}
		if (!isexist) {
			CONFIG_MODULE module;

			module.module = (*ait);
			module.isexist = true;
			module.exit_event = NULL;
			module.thread_handle = NULL;
			module.ipv4_router_active = false;
			module.ipv6_router_active = false;
			m_ModuleList.push_back(module);
		}
	}
}

tinyxml2::XMLElement * CLCXLConfig::WriteModuleList(tinyxml2::XMLElement *owner_element, const std::vector<CONFIG_MODULE> &module_list)
{
	std::vector<CONFIG_MODULE>::const_iterator it;
	for (it = module_list.begin(); it != module_list.end(); it++) {
		//插入模块
		owner_element->InsertEndChild(WriteModule(owner_element->GetDocument()->NewElement(ELEMENT_MODULE), *it));
	}
	return owner_element;
}

tinyxml2::XMLElement * CLCXLConfig::WriteModule(tinyxml2::XMLElement *owner_element, const CONFIG_MODULE &module)
{
	
	tinyxml2::XMLElement *element;

	//插入角色
	element = owner_element->GetDocument()->NewElement(ELEMENT_ROLENAME);
	element->SetAttribute(ATTRIBUTE_VALUE, RoleToStr(module.module.lcxl_role).c_str());
	owner_element->InsertEndChild(element);

	//写入miniport_net_luid
	element = owner_element->GetDocument()->NewElement(ELEMENT_MINIPORT_NET_LUID);
	element->SetAttribute(ATTRIBUTE_VALUE, std::to_string(module.module.miniport_net_luid.Value).c_str());
	owner_element->InsertEndChild(element);

	//写入mac_addr
	element = owner_element->GetDocument()->NewElement(ELEMENT_MAC_ADDR);
	element->SetAttribute(ATTRIBUTE_VALUE, string_format(
		"%02x-%02x-%02x-%02x-%02x-%02x",
		module.module.mac_addr.Address[0],
		module.module.mac_addr.Address[1],
		module.module.mac_addr.Address[2],
		module.module.mac_addr.Address[3],
		module.module.mac_addr.Address[4],
		module.module.mac_addr.Address[5]).c_str());
	owner_element->InsertEndChild(element);

	//插入virtual_addr
	owner_element->InsertEndChild(
		WriteAddrInfo(
		owner_element->GetDocument()->NewElement(ELEMENT_VIRTUAL_ADDR),
		module.module.virtual_addr));
	//插入服务器列表
	owner_element->InsertEndChild(WriteServerList(owner_element->GetDocument()->NewElement(ELEMENT_SERVER_LIST), module.server_list));

	//插入路由表超时时间，以秒为单位
	element = owner_element->GetDocument()->NewElement(ELEMENT_ROUTE_TIMEOUT);
	element->SetAttribute(ATTRIBUTE_VALUE, module.module.route_timeout);
	owner_element->InsertEndChild(element);

	//插入服务器检测信息
	owner_element->InsertEndChild(
		WriteServerCheck(
		owner_element->GetDocument()->NewElement(ELEMENT_SERVER_CHECK),
		module.module.server_check));

	//插入路由算法，有RA_....
	element = owner_element->GetDocument()->NewElement(ELEMENT_ROUTING_ALGORITHM);
	element->SetAttribute(ATTRIBUTE_VALUE, module.module.routing_algorithm);
	owner_element->InsertEndChild(element);

	//写入额外的一些信息
	element = owner_element->GetDocument()->NewElement(ELEMENT_MINIPORT_FRIENDLY_NAME);
	element->SetAttribute(ATTRIBUTE_VALUE, wstring_to_utf8string(std::wstring(module.module.miniport_friendly_name)).c_str());
	owner_element->InsertEndChild(element);

	return owner_element;
}

tinyxml2::XMLElement * CLCXLConfig::WriteAddrInfo(tinyxml2::XMLElement *owner_element, const LCXL_ADDR_INFO &addr)
{
	//CONFIG_MODULE_VIRTUAL_ADDR
	tinyxml2::XMLElement *element;
	//写入状态
	element = owner_element->GetDocument()->NewElement(ELEMENT_STATUS);
	element->SetAttribute(ATTRIBUTE_VALUE, addr.status);
	owner_element->InsertEndChild(element);
	//写入ipv4
	element = owner_element->GetDocument()->NewElement(ELEMENT_IPV4);
	char ipv4[16];
	inet_ntop(AF_INET, const_cast<IN_ADDR*>(&addr.ipv4), ipv4, sizeof(ipv4) / sizeof(ipv4[0]));
	element->SetAttribute(ATTRIBUTE_VALUE, ipv4);
	owner_element->InsertEndChild(element);

	//写入IPv4前缀
	element = owner_element->GetDocument()->NewElement(ELEMENT_IPV4_ONLINK_PREFIX_LENGTH);
	element->SetAttribute(ATTRIBUTE_VALUE, addr.ipv4_onlink_prefix_length);
	owner_element->InsertEndChild(element);

	//写入ipv6
	element = owner_element->GetDocument()->NewElement(ELEMENT_IPV6);
	char ipv6[100];
	inet_ntop(AF_INET6, const_cast<IN6_ADDR*>(&addr.ipv6), ipv6, sizeof(ipv6) / sizeof(ipv6[0]));
	element->SetAttribute(ATTRIBUTE_VALUE, ipv6);
	owner_element->InsertEndChild(element);

	//写入IPv6前缀
	element = owner_element->GetDocument()->NewElement(ELEMENT_IPV6_ONLINK_PREFIX_LENGTH);
	element->SetAttribute(ATTRIBUTE_VALUE, addr.ipv6_onlink_prefix_length);
	owner_element->InsertEndChild(element);

	return owner_element;
}

tinyxml2::XMLElement * CLCXLConfig::WriteServerCheck(tinyxml2::XMLElement *owner_element, const LCXL_SERVER_CHECK &server_check)
{
	tinyxml2::XMLElement *element;

	//插入服务器检测间隔，以秒为单位
	element = owner_element->GetDocument()->NewElement(ELEMENT_INTERVAL);
	element->SetAttribute(ATTRIBUTE_VALUE, server_check.interval);
	owner_element->InsertEndChild(element);

	//插入服务器检测超时时间，以秒为单位
	element = owner_element->GetDocument()->NewElement(ELEMENT_TIMEOUT);
	element->SetAttribute(ATTRIBUTE_VALUE, server_check.timeout);
	owner_element->InsertEndChild(element);

	//插入服务器检测失败时的重试次数
	element = owner_element->GetDocument()->NewElement(ELEMENT_RETRY_NUMBER);
	element->SetAttribute(ATTRIBUTE_VALUE, server_check.retry_number);
	owner_element->InsertEndChild(element);
	return owner_element;
}

tinyxml2::XMLElement * CLCXLConfig::WriteServerList(tinyxml2::XMLElement *owner_element, const std::vector<CONFIG_SERVER> &server_list)
{
	std::vector<CONFIG_SERVER>::const_iterator it;
	for (it = server_list.begin(); it != server_list.end(); it++) {
		//插入服务器
		owner_element->InsertEndChild(WriteServer(owner_element->GetDocument()->NewElement(ELEMENT_SERVER), *it));
	}
	return owner_element;
}

tinyxml2::XMLElement * CLCXLConfig::WriteServer(tinyxml2::XMLElement *owner_element, const CONFIG_SERVER &server)
{
	tinyxml2::XMLElement *element;

	element = owner_element->GetDocument()->NewElement(ELEMENT_STATUS);
	element->SetAttribute(ATTRIBUTE_VALUE, server.server.status);
	owner_element->InsertEndChild(element);

	element = owner_element->GetDocument()->NewElement(ELEMENT_IP_STATUS);
	element->SetAttribute(ATTRIBUTE_VALUE, server.server.ip_status);
	owner_element->InsertEndChild(element);

	element = owner_element->GetDocument()->NewElement(ELEMENT_COMMENT);
	std::wstring comment = server.comment;
	element->SetAttribute(ATTRIBUTE_VALUE, wstring_to_utf8string(comment).c_str());
	owner_element->InsertEndChild(element);

	element = owner_element->GetDocument()->NewElement(ELEMENT_MAC_ADDR);
	element->SetAttribute(ATTRIBUTE_VALUE, string_format(
		"%02x-%02x-%02x-%02x-%02x-%02x",
		server.server.mac_addr.Address[0],
		server.server.mac_addr.Address[1],
		server.server.mac_addr.Address[2],
		server.server.mac_addr.Address[3],
		server.server.mac_addr.Address[4],
		server.server.mac_addr.Address[5]).c_str());
	owner_element->InsertEndChild(element);

	return owner_element;
}

std::vector<CONFIG_MODULE> & CLCXLConfig::ReadModuleList(tinyxml2::XMLElement *owner_element, std::vector<CONFIG_MODULE> &module_list)
{
	tinyxml2::XMLElement * element;

	element = owner_element->FirstChildElement(ELEMENT_MODULE);
	while (element != NULL) {
		CONFIG_MODULE moudule;

		module_list.push_back(ReadModule(element, moudule));

		element = element->NextSiblingElement(ELEMENT_MODULE);
	}
	return module_list;
}

CONFIG_MODULE & CLCXLConfig::ReadModule(tinyxml2::XMLElement *owner_element, CONFIG_MODULE &module)
{
	tinyxml2::XMLElement * element;
	//设置module默认值
	ZeroMemory(&module.module, sizeof(module.module));
	module.isexist = false;
	module.exit_event = NULL;
	module.thread_handle = NULL;
	//角色未知
	module.module.lcxl_role = LCXL_ROLE_UNKNOWN;
	//设置路由项超时时间，默认30分钟
	module.module.route_timeout = 1800;
	//设置负载均衡算法为最小连接数
	module.module.routing_algorithm = RA_LEAST_CONNECTION;

	//读取角色
	element = owner_element->FirstChildElement(ELEMENT_ROLENAME);
	if (element != NULL) {
		module.module.lcxl_role = StrToRole(element->Attribute(ATTRIBUTE_VALUE));
	}

	//读取net_luid
	element = owner_element->FirstChildElement(ELEMENT_MINIPORT_NET_LUID);
	if (element != NULL) {
		module.module.miniport_net_luid.Value = _atoi64(element->Attribute(ATTRIBUTE_VALUE));
	}


	//读取mac_addr
	element = owner_element->FirstChildElement(ELEMENT_MAC_ADDR);
	if (element != NULL) {
		module.module.mac_addr.Length = string_to_mac(element->Attribute(ATTRIBUTE_VALUE), module.module.mac_addr.Address, sizeof(module.module.mac_addr.Address));
		/*
		//另一种读取方法
		module.module.mac_addr.Length = 6;
		sscanf_s(
		element->Attribute(CONFIG_VALUE),
		"%02x-%02x-%02x-%02x-%02x-%02x",
		&module.module.mac_addr.Address[0],
		&module.module.mac_addr.Address[1],
		&module.module.mac_addr.Address[2],
		&module.module.mac_addr.Address[3],
		&module.module.mac_addr.Address[4],
		&module.module.mac_addr.Address[5]
		);
		*/
	}
	//读取virtual_addr

	ZeroMemory(&module.module.virtual_addr, sizeof(module.module.virtual_addr));
	module.module.virtual_addr.ipv4_onlink_prefix_length = 24;
	module.module.virtual_addr.ipv6_onlink_prefix_length = 64;
	element = owner_element->FirstChildElement(ELEMENT_VIRTUAL_ADDR);
	if (element != NULL) {
		ReadAddrInfo(element, module.module.virtual_addr);
	}
	//读取服务器列表
	element = owner_element->FirstChildElement(ELEMENT_SERVER_LIST);
	if (element != NULL) {
		ReadServerList(element, module.server_list);
	}
	
	//读取路由表超时时间，以秒为单位
	element = owner_element->FirstChildElement(ELEMENT_ROUTE_TIMEOUT);
	if (element != NULL) {
		module.module.route_timeout = element->UnsignedAttribute(ATTRIBUTE_VALUE);
	}
	//读取服务器检测信息
	ZeroMemory(&module.module.server_check, sizeof(module.module.server_check));
	//设置服务器检测间隔，默认10秒
	module.module.server_check.interval = 10;
	//服务器检测超时时间，默认10秒
	module.module.server_check.timeout = 10;
	//服务器检测失败时的重试次数，默认2次
	module.module.server_check.retry_number = 2;
	element = owner_element->FirstChildElement(ELEMENT_SERVER_CHECK);
	if (element != NULL) {
		ReadServerCheck(element, module.module.server_check);
	}
	//路由算法，有RA_....
	element = owner_element->FirstChildElement(ELEMENT_ROUTING_ALGORITHM);
	if (element != NULL) {
		module.module.routing_algorithm = element->UnsignedAttribute(ATTRIBUTE_VALUE);
	}
	return module;
}

LCXL_ADDR_INFO & CLCXLConfig::ReadAddrInfo(tinyxml2::XMLElement *owner_element, LCXL_ADDR_INFO &addr)
{
	tinyxml2::XMLElement *element;
	
	
	//读取状态
	element = owner_element->FirstChildElement(ELEMENT_STATUS);
	if (element != NULL) {
		addr.status = element->UnsignedAttribute(ATTRIBUTE_VALUE);
	}
	//读取ipv4
	element = owner_element->FirstChildElement(ELEMENT_IPV4);
	if (element != NULL) {
		inet_pton(AF_INET, element->Attribute(ATTRIBUTE_VALUE), &addr.ipv4);
	}
	//读取ipv4前缀
	element = owner_element->FirstChildElement(ELEMENT_IPV4_ONLINK_PREFIX_LENGTH);
	if (element != NULL) {
		addr.ipv4_onlink_prefix_length = element->UnsignedAttribute(ATTRIBUTE_VALUE);
	}

	//读取ipv6
	element = owner_element->FirstChildElement(ELEMENT_IPV6);
	if (element != NULL) {
		inet_pton(AF_INET6, element->Attribute(ATTRIBUTE_VALUE), &addr.ipv6);
	}
	//读取ipv6前缀
	element = owner_element->FirstChildElement(ELEMENT_IPV6_ONLINK_PREFIX_LENGTH);
	if (element != NULL) {
		addr.ipv6_onlink_prefix_length = element->UnsignedAttribute(ATTRIBUTE_VALUE);
	}

	return addr;
}

LCXL_SERVER_CHECK & CLCXLConfig::ReadServerCheck(tinyxml2::XMLElement *owner_element, LCXL_SERVER_CHECK &server_check)
{
	tinyxml2::XMLElement * element;

	//设置服务器检测间隔，以秒为单位
	element = owner_element->FirstChildElement(ELEMENT_INTERVAL);
	if (element != NULL) {
		server_check.interval = element->UnsignedAttribute(ATTRIBUTE_VALUE);
	}
	//服务器检测超时时间，以秒为单位
	element = owner_element->FirstChildElement(ELEMENT_TIMEOUT);
	if (element != NULL) {
		server_check.timeout = element->UnsignedAttribute(ATTRIBUTE_VALUE);
	}
	//服务器检测失败时的重试次数
	element = owner_element->FirstChildElement(ELEMENT_RETRY_NUMBER);
	if (element != NULL) {
		server_check.retry_number = element->UnsignedAttribute(ATTRIBUTE_VALUE);
	}
	return server_check;
}

std::vector<CONFIG_SERVER> & CLCXLConfig::ReadServerList(tinyxml2::XMLElement *owner_element, std::vector<CONFIG_SERVER> &server_list)
{
	tinyxml2::XMLElement * element;

	element = owner_element->FirstChildElement(ELEMENT_SERVER);
	while (element != NULL) {
		CONFIG_SERVER server;

		server_list.push_back(ReadServer(element, server));

		element = element->NextSiblingElement(ELEMENT_SERVER);
	}
	return server_list;
}

CONFIG_SERVER & CLCXLConfig::ReadServer(tinyxml2::XMLElement *owner_element, CONFIG_SERVER &server)
{
	tinyxml2::XMLElement *element;
	ZeroMemory(&server, sizeof(server));
	element = owner_element->FirstChildElement(ELEMENT_STATUS);
	if (element != NULL) {
		server.server.status = element->UnsignedAttribute(ATTRIBUTE_VALUE);
	}
	element = owner_element->FirstChildElement(ELEMENT_IP_STATUS);
	if (element != NULL) {
		server.server.ip_status = element->UnsignedAttribute(ATTRIBUTE_VALUE);
	}
	element = owner_element->FirstChildElement(ELEMENT_COMMENT);
	if (element != NULL) {
		wcscpy_s(server.comment, sizeof(server.comment) / sizeof(server.comment[0]), utf8string_to_wstring(std::string(element->Attribute(ATTRIBUTE_VALUE))).c_str());
	}
	//读取mac_addr
	element = owner_element->FirstChildElement(ELEMENT_MAC_ADDR);
	if (element != NULL) {
		server.server.mac_addr.Length = string_to_mac(element->Attribute(ATTRIBUTE_VALUE), server.server.mac_addr.Address, sizeof(server.server.mac_addr.Address));
	}
	return server;
}

CLCXLConfig::CLCXLConfig()
{
	m_Port = VALUE_PORT_DEFAULT;

	InitializeCriticalSection(&m_CriticalSection);

}

CONFIG_MODULE * CLCXLConfig::FindModuleByLuid(NET_LUID miniport_net_luid)
{
	std::vector<CONFIG_MODULE>::iterator it;
	for (it = m_ModuleList.begin(); it != m_ModuleList.end(); it++) {
		if ((*it).module.miniport_net_luid.Value == miniport_net_luid.Value) {
			return &(*it);
		}
	}
	return NULL;
}

CLCXLConfig::~CLCXLConfig()
{
	DeleteCriticalSection(&m_CriticalSection);
}

CCSLocker CLCXLConfig::LockinLifeCycle()
{
	return CCSLocker(&m_CriticalSection);
}

int CLCXLConfig::StrToRole(const std::string val)
{
	if (val == VALUE_ROLENAME_ROUTE) {
		return LCXL_ROLE_ROUTER;
	} else if (val == VALUE_ROLENAME_SERVER) {
		return LCXL_ROLE_SERVER;
	} else {
		return LCXL_ROLE_UNKNOWN;
	}
}

std::string CLCXLConfig::RoleToStr(int role)
{
	switch (role) {
	case LCXL_ROLE_ROUTER:
		return VALUE_ROLENAME_ROUTE;
	case LCXL_ROLE_SERVER:
		return VALUE_ROLENAME_SERVER;
	default:
		return VALUE_ROLENAME_UNKNOWN;
	}
}





