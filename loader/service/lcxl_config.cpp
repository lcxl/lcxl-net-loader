#include "lcxl_config.h"
#include <algorithm>
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
	if (m_XmlDoc.LoadFile(XmlFilePath.c_str()) == 0) {
		//验证根元素名称是否正确
		std::string root_ele_name = m_XmlDoc.RootElement()->Name();
		std::transform(root_ele_name.begin(), root_ele_name.end(),
			root_ele_name.begin(), ::tolower);
		if (root_ele_name == CONFIG_ROOT_ELEMENT) {
			OutputDebugStr(_T("The root element of the xml file is not 'lcxlnetloader'\n"));
			return false;
		}
	}
	

	tinyxml2::XMLElement *ele;

	ele = m_XmlDoc.FirstChildElement(CONFIG_PORT);
	if (ele == NULL) {
		m_Port = CONFIG_PORT_DEFAULT;
	} else {
		m_Port = ele->IntAttribute(CONFIG_VALUE);
		if (m_Port <= 0 || m_Port >= 65535) {
			OutputDebugStr(_T("port=%d is invalid.\n"), m_Port);
			return false;
		}
	}
	
	ele = m_XmlDoc.FirstChildElement(CONFIG_ROLENAME);
	if (ele == NULL) {
		SetRolename(CONFIG_ROLENAME_DEFAULT);
	} else {
		SetRolename(ele->Attribute(CONFIG_VALUE));
	}


	return true;
}

bool CLCXLConfig::SaveXMLFile(std::string XmlFilePath)
{
	m_XmlDoc.Clear();
	m_XmlDoc.InsertEndChild(m_XmlDoc.NewDeclaration());
	m_XmlDoc.SetBOM(true);
	//写入根元素
	tinyxml2::XMLNode *root_element;
	tinyxml2::XMLElement *element;

	root_element = m_XmlDoc.InsertEndChild(m_XmlDoc.NewElement(CONFIG_ROOT_ELEMENT));
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
		m_Rolename = CONFIG_ROLENAME_LOADER;
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
	if (m_Rolename == CONFIG_ROLENAME_LOADER) {
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
			if ((*ait).miniport_net_luid.Value == (*cit).miniport_net_luid.Value) {
				(*cit).miniport_ifindex = (*ait).miniport_ifindex;
				CopyMemory(&(*cit).mac_addr, &(*ait).mac_addr, sizeof((*cit).mac_addr));
				wcscpy_s((*cit).miniport_friendly_name, sizeof((*cit).miniport_friendly_name) / sizeof(WCHAR), (*ait).miniport_friendly_name);
				wcscpy_s((*cit).miniport_name, sizeof((*cit).miniport_name) / sizeof(WCHAR), (*ait).miniport_name);
				wcscpy_s((*cit).filter_module_name, sizeof((*cit).filter_module_name) / sizeof(WCHAR), (*ait).filter_module_name);
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
			if ((*ait).miniport_net_luid.Value == (*cit).miniport_net_luid.Value) {
				isexist = true;
				break;
			}
		}
		if (!isexist) {
			CONFIG_MODULE module;
			(APP_MODULE)module = (*ait);
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
	element->SetAttribute(CONFIG_VALUE, std::to_string(module.miniport_net_luid.Value).c_str());
	owner_element->InsertEndChild(element);

	//写入mac_addr
	element = owner_element->GetDocument()->NewElement(ELEMENT_MAC_ADDR);
	element->SetAttribute(CONFIG_VALUE, string_format(
		"%02x-%02x-%02x-%02x-%02x-%02x",
		module.mac_addr.Address[0],
		module.mac_addr.Address[1],
		module.mac_addr.Address[2],
		module.mac_addr.Address[3],
		module.mac_addr.Address[4],
		module.mac_addr.Address[5]).c_str());
	owner_element->InsertEndChild(element);

	//插入virtual_addr
	owner_element->InsertEndChild(
		WriteAddrInfo(
		owner_element->GetDocument()->NewElement(ELEMENT_VIRTUAL_ADDR),
		module.virtual_addr));
	//插入服务器列表
	owner_element->InsertEndChild(WriteServerList(owner_element->GetDocument()->NewElement(ELEMENT_SERVER_LIST), module.server_list));
	/*
	std::vector<CONFIG_MODULE_INFO>::const_iterator it;
	for (it = module_list.begin(); it != module_list.end(); it++) {
		//插入模块
		owner_element->InsertEndChild(WriteModule(owner_element->GetDocument()->NewElement(CONFIG_MODULE), *it));


	}
	*/

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
	inet_ntop(AF_INET, const_cast<IN6_ADDR*>(&addr.ipv6), ipv6, sizeof(ipv6) / sizeof(ipv4[0]));
	element->SetAttribute(CONFIG_VALUE, ipv6);
	owner_element->InsertEndChild(element);

	return owner_element;
}

tinyxml2::XMLElement * CLCXLConfig::WriteServerList(tinyxml2::XMLElement *owner_element, const std::vector<LCXL_SERVER> &server_list)
{
	std::vector<LCXL_SERVER>::const_iterator it;
	for (it = server_list.begin(); it != server_list.end(); it++) {
		//插入服务器
		owner_element->InsertEndChild(WriteServer(owner_element->GetDocument()->NewElement(ELEMENT_SERVER), *it));
	}
	return owner_element;
}

tinyxml2::XMLElement * CLCXLConfig::WriteServer(tinyxml2::XMLElement *owner_element, const LCXL_SERVER &server)
{
	tinyxml2::XMLElement *element;

	element = owner_element->GetDocument()->NewElement(ELEMENT_STATUS);
	element->SetAttribute(CONFIG_VALUE, server.status);
	owner_element->InsertEndChild(element);

	element = owner_element->GetDocument()->NewElement(ELEMENT_IP_STATUS);
	element->SetAttribute(CONFIG_VALUE, server.ip_status);
	owner_element->InsertEndChild(element);

	element = owner_element->GetDocument()->NewElement(ELEMENT_COMMENT);
	std::wstring comment = server.comment;
	element->SetAttribute(CONFIG_VALUE, wstring_to_string(comment).c_str());
	owner_element->InsertEndChild(element);

	element = owner_element->GetDocument()->NewElement(ELEMENT_MAC_ADDR);
	element->SetAttribute(CONFIG_VALUE, string_format(
		"%02x-%02x-%02x-%02x-%02x-%02x",
		server.mac_addr.Address[0],
		server.mac_addr.Address[1],
		server.mac_addr.Address[2],
		server.mac_addr.Address[3],
		server.mac_addr.Address[4],
		server.mac_addr.Address[5]).c_str());
	owner_element->InsertEndChild(element);

	return owner_element;
}


