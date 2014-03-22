#include "lcxl_config.h"
#include <algorithm>
#include <tchar.h>

#ifdef _DEBUG
void OutputDebugStr(const TCHAR fmt[], ...);
#else
#define OutputDebugStr(__fmt, ...)
#endif // _DEBUG

bool CLCXLConfig::LoadXMLFile(std::string XmlFilePath)
{
	

	bool resu = m_XmlDoc.LoadFile(XmlFilePath.c_str()) == 0;
	if (!resu) {
		return false;
	}
	//验证根元素名称是否正确
	std::string root_ele_name = m_XmlDoc.RootElement()->Name();
	std::transform(root_ele_name.begin(), root_ele_name.end(),
		root_ele_name.begin(), ::tolower);
	if (root_ele_name == CONFIG_ROOT_ELEMENT) {
		OutputDebugStr(_T("The root element of the xml file is not 'lcxlnetloader'\n"));
		return false;
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
		SetRoleName(CONFIG_ROLENAME_DEFAULT);
	} else {
		SetRoleName(ele->Attribute(CONFIG_VALUE));
	}


	return resu;
}

bool CLCXLConfig::SaveXMLFile(std::string XmlFilePath)
{
	return m_XmlDoc.SaveFile(XmlFilePath.c_str()) == tinyxml2::XML_NO_ERROR;
}

void CLCXLConfig::SetRole(int val)
{
	
	m_Role = val;
	switch (m_Role) {
	case LCXL_ROLE_ROUTER:
		m_RoleName = CONFIG_ROLENAME_LOADER;
		break;
	case LCXL_ROLE_SERVER:
		m_RoleName = CONFIG_ROLENAME_SERVER;
		break;
	default:
		m_RoleName = CONFIG_ROLENAME_UNKNOWN;
		m_Role = LCXL_ROLE_UNKNOWN;
		break;
	}

}

void CLCXLConfig::SetRoleName(const std::string val)
{
	m_RoleName = val;
	if (m_RoleName == CONFIG_ROLENAME_LOADER) {
		m_Role = LCXL_ROLE_ROUTER;
	} else if (m_RoleName == CONFIG_ROLENAME_SERVER) {
		m_Role = LCXL_ROLE_SERVER;
	} else {
		m_Role = LCXL_ROLE_UNKNOWN;
		m_RoleName = CONFIG_ROLENAME_UNKNOWN;
	}
	
}
