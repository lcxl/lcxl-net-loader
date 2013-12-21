#ifndef _LCXL_STRING_H_
#define _LCXL_STRING_H_

#include <string>
#include <tchar.h>

#define tstring basic_string<TCHAR>

#ifdef _UNICODE
#define to_tstring to_wstring
#else
#define to_tstring to_string
#endif

std::wstring StringToWString(const std::string &str);
std::string WStringToString(const std::wstring &str);

#ifdef _UNICODE
#define TStringToWString(__STR)  (__STR)
#define TStringToString(__STR)  WStringToString(__STR)
#define WStringToTString(__STR) (__STR)
#define StringToTString(__STR) StringToWString(__STR)
#else
#define TStringToWString(__STR)  StringToWString(__STR)
#define TStringToString(__STR)  (__STR)
#define WStringToTString(__STR) WStringToString(__STR)
#define StringToTString(__STR) (__STR)
#endif

#endif