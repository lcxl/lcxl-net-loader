#ifndef _LCXL_TYPE_H_
#define _LCXL_TYPE_H_
//author:LCXL
//abstract:驱动和应用程序共用的自定义数据包有关结构数据头文件
//如果是驱动程序，需要前面加头文件lcxl_net.h
//如果是Win32程序，需要前面加WinSock2.h
//#include "driver/lcxl_net.h"
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
		IF_PHYSICAL_ADDRESS mac_addr;
	} LCXL_SERVER_ADDR, *PLCXL_SERVER_ADDR;//服务器地址

	//服务器性能
	typedef struct _LCXL_SERVER_PERFORMANCE
	{
		//单个任务的平均处理时间，时间单位为微妙（us）
		//Windows下使用KeQueryPerformanceCounter
		unsigned long       process_time;
		//总内存数
		unsigned long long  total_memory;
		//当前使用内存
		unsigned long long  cur_memory;
		//CPU使用率，最高为1
		double              cpu_usage;
	} LCXL_SERVER_PERFORMANCE, *PLCXL_SERVER_PERFORMANCE;
	//服务器信息
	typedef struct _LCXL_SERVER_INFO {
#define SS_ENABLED	0x01//服务器处于启用状态
#define SS_ONLINE	0x02//服务器在线
#define SS_DELETED	0x80//服务器已被删除
		//服务器状态
		UCHAR				status;
		//计算机名
		CHAR				comment[256];
		//服务器真实IP地址
		LCXL_SERVER_ADDR	addr;
	} LCXL_SERVER_INFO, *PLCXL_SERVER_INFO;

#ifdef __cplusplus
}
#endif

#endif