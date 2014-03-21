#ifndef _LCXL_CONFIG_H_
#define _LCXL_CONFIG_H_
#include <string>
#include "../../component/tinyxml2/tinyxml2.h"


#define CONFIG_ROOT_ELEMENT "lcxlnetloader"
#define CONFIG_VALUE "value"

#define CONFIG_PORT "port"
#define CONFIG_DEFAULT_PORT 32112

#define CONFIG_ROLE "role"
#define CONFIG_DEFAULT_ROLE "unknown"
class CLCXLConfig {
private:
	tinyxml2::XMLDocument m_XmlDoc;
	int m_Port;
	std::string m_Role;
	
public:
	bool SaveXMLFile(std::string XmlFilePath);
	bool LoadXMLFile(std::string XmlFilePath);
	int GetPort() const { return m_Port; }
	std::string GetRole() const { return m_Role; }
	void SetRole(const std::string val) { m_Role = val; }
};

#endif