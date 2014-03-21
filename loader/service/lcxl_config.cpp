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
		m_Port = CONFIG_DEFAULT_PORT;
	} else {
		m_Port = ele->IntAttribute(CONFIG_VALUE);
		if (m_Port <= 0 || m_Port >= 65535) {
			OutputDebugStr(_T("port=%d is invalid.\n"), m_Port);
			return false;
		}
	}
	
	ele = m_XmlDoc.FirstChildElement(CONFIG_ROLE);
	if (ele == NULL) {
		m_Role = CONFIG_DEFAULT_ROLE;
	} else {
		m_Role = ele->Attribute(CONFIG_VALUE);
		if (m_Role.empty()) {
			m_Role = CONFIG_DEFAULT_ROLE;
		}
	}


	return resu;
}

bool CLCXLConfig::SaveXMLFile(std::string XmlFilePath)
{
	return m_XmlDoc.SaveFile(XmlFilePath.c_str()) == tinyxml2::XML_NO_ERROR;
}
