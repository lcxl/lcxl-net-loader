#ifndef _LCXL_NET_H_
#define _LCXL_NET_H_

#include <netioddk.h>
//typedef void             *LPVOID;

#define ntohs(__A) ((((__A) & 0xff) << 8) | (((__A) & 0xff00) >> 8))

#endif