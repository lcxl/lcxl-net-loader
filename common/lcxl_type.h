#ifndef _LCXL_TYPE_H_
#define _LCXL_TYPE_H_
#include <ifdef.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _LCXL_IP {
#define IM_UNKNOWN 0
#define IM_IPV4	1
#define IM_IPV6 2
		int						ip_mode;		//IP模式，IPv4还是IPv6， IM_IPV4, IM_IPV6
		union {
			//IP
			IN_ADDR			    ip_4;			//源IPv4地址
			IN6_ADDR			ip_6;			//源IPv6地址
		} addr;
	} LCXL_IP, *PLCXL_IP;

	typedef struct _LCXL_SERVER_ADDR {
#define SA_ENABLE_IPV4 0x01//服务器启用了IPV4协议
#define SA_ENABLE_IPV6 0x02//服务器启用了IPV6协议
		UCHAR			status;
		//真实的IP地址
		IN_ADDR			ipv4;
		IN6_ADDR		ipv6;
		//MAC地址
		IF_PHYSICAL_ADDRESS_LH mac_addr;
	} LCXL_SERVER_ADDR, *PLCXL_SERVER_ADDR;//服务器地址

#ifdef __cplusplus
}
#endif

#endif