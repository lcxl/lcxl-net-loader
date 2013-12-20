#ifndef _APP_DBG_H_
#define _APP_DBG_H_
#include <stdlib.h>
__inline void OutputDebugStr(const TCHAR fmt[], ...)
{
#ifdef _DEBUG
	va_list argptr;
	PTCHAR buf;
	va_start(argptr, fmt);
	int bufsize = _vsntprintf(NULL, 0, fmt, argptr) + 2;
	buf = (PTCHAR)malloc(bufsize*sizeof(TCHAR));
	_vsntprintf(buf, bufsize, fmt, argptr);
	OutputDebugString(buf);
	free(buf);
#endif // _DEBUG
}

#endif