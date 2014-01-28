#ifndef _LCXL_NET_H_
#define _LCXL_NET_H_

#include <netioddk.h>
//#include <ws2def.h>
//#ifndef INCL_WINSOCK_API_PROTOTYPES
//#define INCL_WINSOCK_API_PROTOTYPES

#define ntohs(__A) ((((__A) & 0xff) << 8) | (((__A) & 0xff00) >> 8))

//#endif

#endif