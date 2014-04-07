#ifndef _LCXL_FUNC_H_
#define _LCXL_FUNC_H_
#include "../../component/lcxl_iocp/lcxl_string.h"
#include <Windows.h>
std::tstring GetAppFilePath();
std::tstring ExtractFilePath(const std::tstring file_path);
//ÁÙ½çÇøËø
class CCSLocker {
private:
	LPCRITICAL_SECTION m_cs;
public:
	CCSLocker(LPCRITICAL_SECTION cs) {
		_ASSERT(cs);
		m_cs = cs;
		EnterCriticalSection(m_cs);
	}

	CCSLocker(const CCSLocker &value) {
		m_cs = value.m_cs;
		EnterCriticalSection(m_cs);
	}

	~CCSLocker() {
		LeaveCriticalSection(m_cs);
	}
	//½ûÖ¹¸³Öµ
	CCSLocker& operator=(CCSLocker &value) {
		_ASSERT(FALSE);
	}
};

#endif