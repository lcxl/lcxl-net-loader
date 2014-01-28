#ifndef _LCXL_SERVER_H_
#define _LCXL_SERVER_H_
/*
author:
LCX
abstract:
服务器相关头文件
*/

#define TAG_SERVER      'SERV'

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

typedef struct _LCXL_SERVER_INFO {
#define SS_ENABLED	0x01//服务器处于启用状态
#define SS_ONLINE	0x02//服务器在线
	//服务器状态
	UCHAR				status;
	//计算机名
	CHAR				computer_name[256];
	//服务器真实IP地址
	LCXL_SERVER_ADDR	addr;
} LCXL_SERVER_INFO, *PLCXL_SERVER_INFO;

//服务器信息列表项
typedef struct _SERVER_INFO_LIST_ENTRY
{
	//列表项
	LIST_ENTRY				list_entry;
	//引用计数
	volatile LONG  			ref_count;
	//服务器状态
	LCXL_SERVER_INFO		info;
	//服务器性能状态
	LCXL_SERVER_PERFORMANCE	performance;
} SERVER_INFO_LIST_ENTRY, *PSERVER_INFO_LIST_ENTRY;

extern NPAGED_LOOKASIDE_LIST  g_server_mem_mgr;

#define INIT_SERVER_MEM_MGR() ExInitializeNPagedLookasideList(&g_server_mem_mgr, NULL, NULL, 0, sizeof(SERVER_INFO_LIST_ENTRY), TAG_SERVER, 0)
#define ALLOC_SERVER() (PSERVER_INFO_LIST_ENTRY)ExAllocateFromNPagedLookasideList(&g_server_mem_mgr)
#define FREE_SERVER(__buf) ExFreeToNPagedLookasideList(&g_server_mem_mgr, __buf)
#define DEL_SERVER_MEM_MGR() ExDeleteNPagedLookasideList(&g_server_mem_mgr)

///<summary>
//选择服务器
///</summary>
PSERVER_INFO_LIST_ENTRY SelectServer(IN PLIST_ENTRY server_list, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader);

#endif