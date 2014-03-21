#include "lcxl_func.h"
#include <windows.h>

std::tstring GetAppFilePath()
{
	TCHAR szFilePath[MAX_PATH + 1];
	GetModuleFileName(NULL, szFilePath, MAX_PATH); 
	return szFilePath;
}

std::tstring ExtractFilePath(const std::tstring file_path)
{
	int pos = file_path.rfind(_T('\\'));
	if (pos == std::tstring::npos) {
		int pos = file_path.rfind(_T('/'));
	}
	if (pos == std::tstring::npos) {
		return _T("");
	} else {
		return file_path.substr(0, pos+1);
	}
	
}
