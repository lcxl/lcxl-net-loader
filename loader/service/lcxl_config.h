#ifndef _LCXL_CONFIG_H_
#define _LCXL_CONFIG_H_
#include <WinSock2.h>
#include <Ws2ipdef.h>
#include <string>
#include "../../component/tinyxml2/tinyxml2.h"
#include "../common/drv_interface_type.h"

#define CONFIG_ROOT_ELEMENT "lcxlnetloader"
#define CONFIG_VALUE "value"

#define CONFIG_PORT "port"
#define CONFIG_PORT_DEFAULT 32112

#define CONFIG_ROLENAME "rolename"
#define CONFIG_ROLENAME_UNKNOWN "unknown"
#define CONFIG_ROLENAME_LOADER "loader"
#define CONFIG_ROLENAME_SERVER "server"
#define CONFIG_ROLENAME_DEFAULT CONFIG_ROLENAME_UNKNOWN

class CLCXLConfig {
private:
	tinyxml2::XMLDocument m_XmlDoc;
	int m_Port;
	std::string m_RoleName;
	int m_Role;
	
public:
	bool SaveXMLFile(std::string XmlFilePath);
	bool LoadXMLFile(std::string XmlFilePath);

	int GetPort() const { return m_Port; }

	std::string GetRoleName() const { return m_RoleName; }
	void SetRoleName(const std::string val);
	
	int GetRole() const { return m_Role; }
	void SetRole(int val);
};

#endif