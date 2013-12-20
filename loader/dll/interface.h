#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include "../common/dll_interface.h"

#define DRIVER_NAME _T("\\\\.\\netloader")

#define OpenDirver() CreateFile(\
	DRIVER_NAME, \
	GENERIC_READ || GENERIC_WRITE, \
	FILE_SHARE_READ || FILE_SHARE_WRITE, \
	NULL, \
	OPEN_EXISTING, \
	FILE_ATTRIBUTE_NORMAL, \
	NULL\
	)



#endif