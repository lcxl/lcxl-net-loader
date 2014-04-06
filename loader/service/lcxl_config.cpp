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
		if (string_tolower(root_ele_name) != CONFIG_ROOT_ELEMENT) {
			OutputDebugStr(_T("The root element of the xml file is not '%s'\n"), wstring_to_tstring(std::string(CONFIG_ROOT_ELEMENT)).c_str());
			return false;
		}
	} else {
		return true;
	}
	tinyxml2::XMLElement *element;
	//读取监听端口
	element = root_element->FirstChildElement(CONFIG_PORT);
	if (element != NULL) {
		m_Port = element->IntAttribute(CONFIG_VALUE);
		if (m_Port <= 0 || m_Port >= 65535) {
			OutputDebugStr(_T("port=%d is invalid.\n"), m_Port);
			return false;
		}
	}
	//读取角色
	element = root_element->FirstChildElement(CONFIG_ROLENAME);
	if (element != NULL) {
		SetRolename(element->Attribute(CONFIG_VALUE));
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
	m_XmlDoc.Clear();
	m_XmlDoc.InsertEndChild(m_XmlDoc.NewDeclaration());
	m_XmlDoc.SetBOM(true);

	tinyxml2::XMLNode *root_element;
	
	//创建根元素
	root_element = m_XmlDoc.InsertEndChild(m_XmlDoc.NewElement(CONFIG_ROOT_ELEMENT));

	tinyxml2::XMLElement *element;
	//写入端口
	element = m_XmlDoc.NewElement(CONFIG_PORT);
	element->SetAttribute(CONFIG_VALUE, m_Port);
	root_element->InsertEndChild(element);
	//写入角色
	element = m_XmlDoc.NewElement(CONFIG_ROLENAME);
	element->SetAttribute(CONFIG_VALUE, m_Rolename.c_str());
	root_element->InsertEndChild(element);
	//创建模块列表
	root_element->InsertEndChild(WriteModuleList(m_XmlDoc.NewElement(ELEMENT_MODULE_LIST), m_ModuleList));
	return m_XmlDoc.SaveFile(XmlFilePath.c_str()) == tinyxml2::XML_NO_ERROR;
}

void CLCXLConfig::SetRole(int val)
{
	
	m_Role = val;
	switch (m_Role) {
	case LCXL_ROLE_ROUTER:
		m_Rolename = CONFIG_ROLENAME_ROUTE;
		break;
	case LCXL_ROLE_SERVER:
		m_Rolename = CONFIG_ROLENAME_SERVER;
		break;
	default:
		m_Rolename = CONFIG_ROLENAME_UNKNOWN;
		m_Role = LCXL_ROLE_UNKNOWN;
		break;
	}

}

void CLCXLConfig::SetRolename(const std::string val)
{
	m_Rolename = val;
	if (m_Rolename == CONFIG_ROLENAME_ROUTE) {
		m_Role = LCXL_ROLE_ROUTER;
	} else if (m_Rolename == CONFIG_ROLENAME_SERVER) {
		m_Role = LCXL_ROLE_SERVER;
	} else {
		m_Role = LCXL_ROLE_UNKNOWN;
		m_Rolename = CONFIG_ROLENAME_UNKNOWN;
	}
	
}

void CLCXLConfig::UpdateModuleList(const std::vector<APP_MODULE> &module_list)
{
	std::vector<CONFIG_MODULE>::iterator cit;
	std::vector<APP_MODULE>::const_iterator ait;

	for (cit = m_ModuleList.begin(); cit != m_ModuleList.end(); ) {
		bool isexist = false;
		for (ait = module_list.cbegin(); ait != module_list.cend(); ait++) {
			if ((*ait).miniport_net_luid.Value == (*cit).module.miniport_net_luid.Value) {
				//更新CONFIG_MODULE中的数据，以下数据以系统为准
				(*cit).module.miniport_ifindex = (*ait).miniport_ifindex;
				CopyMemory(&(*cit).module.mac_addr, &(*ait).mac_addr, sizeof((*cit).module.mac_addr));
				wcscpy_s((*cit).module.miniport_friendly_name, sizeof((*cit).module.miniport_friendly_name) / sizeof(WCHAR), (*ait).miniport_friendly_name);
				wcscpy_s((*cit).module.miniport_name, sizeof((*cit).module.miniport_name) / sizeof(WCHAR), (*ait).miniport_name);
				wcscpy_s((*cit).module.filter_module_name, sizeof((*cit).module.filter_module_name) / sizeof(WCHAR), (*ait).filter_module_name);
				isexist = true;
				break;
			}
		}
		if (!isexist) {
			cit = m_ModuleList.erase(cit);
		} else {
			cit++;
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
	//写入miniport_net_luid
	tinyxml2::XMLElement *element;

	element = owner_element->GetDocument()->NewElement(ELEMENT_MINIPORT_NET_LUID);
	element->SetAttribute(CONFIG_VALUE, std::to_string(module.module.miniport_net_luid.Value).c_str());
	owner_element->InsertEndChild(element);

	//写入mac_addr
	element = owner_element->GetDocument()->NewElement(ELEMENT_MAC_ADDR);
	element->SetAttribute(CONFIG_VALUE, string_format(
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

	//写入router_mac_addr
	element = owner_element->GetDocument()->NewElement(ELEMENT_ROUTER_MAC_ADDR);
	element->SetAttribute(CONFIG_VALUE, string_format(
		"%02x-%02x-%02x-%02x-%02x-%02x",
		module.router_mac_addr.Address[0],
		module.router_mac_addr.Address[1],
		module.router_mac_addr.Address[2],
		module.router_mac_addr.Address[3],
		module.router_mac_addr.Address[4],
		module.router_mac_addr.Address[5]).c_str());
	owner_element->InsertEndChild(element);
	return owner_element;
}

tinyxml2::XMLElement * CLCXLConfig::WriteAddrInfo(tinyxml2::XMLElement *owner_element, const LCXL_ADDR_INFO &addr)
{
	//CONFIG_MODULE_VIRTUAL_ADDR
	tinyxml2::XMLElement *element;
	//写入状态
	element = owner_element->GetDocument()->NewElement(ELEMENT_STATUS);
	element->SetAttribute(CONFIG_VALUE, addr.status);
	owner_element->InsertEndChild(element);
	//写入ipv4
	element = owner_element->GetDocument()->NewElement(ELEMENT_IPV4);
	char ipv4[16];
	inet_ntop(AF_INET, const_cast<IN_ADDR*>(&addr.ipv4), ipv4, sizeof(ipv4) / sizeof(ipv4[0]));
	element->SetAttribute(CONFIG_VALUE, ipv4);
	owner_element->InsertEndChild(element);
	//写入ipv6
	element = owner_element->GetDocument()->NewElement(ELEMENT_IPV6);
	char ipv6[100];
	inet_ntop(AF_INET6, const_cast<IN6_ADDR*>(&addr.ipv6), ipv6, sizeof(ipv6) / sizeof(ipv6[0]));
	element->SetAttribute(CONFIG_VALUE, ipv6);
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
	element->SetAttribute(CONFIG_VALUE, server.server.status);
	owner_element->InsertEndChild(element);

	element = owner_element->GetDocument()->NewElement(ELEMENT_IP_STATUS);
	element->SetAttribute(CONFIG_VALUE, server.server.ip_status);
	owner_element->InsertEndChild(element);

	element = owner_element->GetDocument()->NewElement(ELEMENT_COMMENT);
	std::wstring comment = server.comment;
	element->SetAttribute(CONFIG_VALUE, wstring_to_string(comment).c_str());
	owner_element->InsertEndChild(element);

	element = owner_element->GetDocument()->NewElement(ELEMENT_MAC_ADDR);
	element->SetAttribute(CONFIG_VALUE, string_format(
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

	ZeroMemory(&module.module, sizeof(module.module));
	//读取net_luid
	element = owner_element->FirstChildElement(ELEMENT_MINIPORT_NET_LUID);
	if (element != NULL) {
		module.module.miniport_net_luid.Value = atol(element->Attribute(CONFIG_VALUE));
	}

	//读取mac_addr
	element = owner_element->FirstChildElement(ELEMENT_MAC_ADDR);
	if (element != NULL) {
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
	}
	//读取virtual_addr
	element = owner_element->FirstChildElement(ELEMENT_VIRTUAL_ADDR);
	if (element != NULL) {
		ReadAddrInfo(element, module.module.virtual_addr);
	}
	//读取服务器列表
	element = owner_element->FirstChildElement(ELEMENT_SERVER_LIST);
	if (element != NULL) {
		ReadServerList(element, module.server_list);
	}

	//读取router_mac_addr
	element = owner_element->FirstChildElement(ELEMENT_ROUTER_MAC_ADDR);
	if (element != NULL) {
		module.router_mac_addr.Length = 6;
		sscanf_s(
			element->Attribute(CONFIG_VALUE),
			"%02x-%02x-%02x-%02x-%02x-%02x",
			&module.router_mac_addr.Address[0],
			&module.router_mac_addr.Address[1],
			&module.router_mac_addr.Address[2],
			&module.router_mac_addr.Address[3],
			&module.router_mac_addr.Address[4],
			&module.router_mac_addr.Address[5]
			);
	}
	return module;
}

LCXL_ADDR_INFO & CLCXLConfig::ReadAddrInfo(tinyxml2::XMLElement *owner_element, LCXL_ADDR_INFO &addr)
{
	tinyxml2::XMLElement *element;
	
	ZeroMemory(&addr, sizeof(addr));
	//读取状态
	element = owner_element->FirstChildElement(ELEMENT_STATUS);
	if (element != NULL) {
		addr.status = element->UnsignedAttribute(CONFIG_VALUE);
	}
	//读取ipv4
	element = owner_element->FirstChildElement(ELEMENT_IPV4);
	if (element != NULL) {
		inet_pton(AF_INET, element->Attribute(CONFIG_VALUE), &addr.ipv4);
	}
	//读取ipv6
	element = owner_element->FirstChildElement(ELEMENT_IPV6);
	if (element != NULL) {
		inet_pton(AF_INET6, element->Attribute(CONFIG_VALUE), &addr.ipv6);
	}
	return addr;
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
		server.server.status = element->UnsignedAttribute(CONFIG_VALUE);
	}
	element = owner_element->FirstChildElement(ELEMENT_IP_STATUS);
	if (element != NULL) {
		server.server.status = element->UnsignedAttribute(CONFIG_VALUE);
	}
	element = owner_element->FirstChildElement(ELEMENT_COMMENT);
	if (element != NULL) {
		wcscpy_s(server.comment, sizeof(server.comment) / sizeof(server.comment[0]), string_to_wstring(std::string(element->Attribute(CONFIG_VALUE))).c_str());
	}
	//读取mac_addr
	element = owner_element->FirstChildElement(ELEMENT_MAC_ADDR);
	if (element != NULL) {
		server.server.mac_addr.Length = 6;
		sscanf_s(
			element->Attribute(CONFIG_VALUE),
			"%02x-%02x-%02x-%02x-%02x-%02x",
			&server.server.mac_addr.Address[0],
			&server.server.mac_addr.Address[1],
			&server.server.mac_addr.Address[2],
			&server.server.mac_addr.Address[3],
			&server.server.mac_addr.Address[4],
			&server.server.mac_addr.Address[5]
			);
	}
	return server;
}

CLCXLConfig::CLCXLConfig()
{
	m_Port = CONFIG_PORT_DEFAULT;
	SetRolename(CONFIG_ROLENAME_DEFAULT);
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


