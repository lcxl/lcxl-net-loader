#ifndef _LCXL_STRING_H_
#define _LCXL_STRING_H_

#include <string>
// 需包含locale、string头文件、使用setlocale函数。
std::wstring StringToWstring(const std::string &str);
std::string WstringToString(const std::wstring &str);

#endif