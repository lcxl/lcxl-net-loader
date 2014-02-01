#include "precomp.h"
#include "lcxl_server.h"

NPAGED_LOOKASIDE_LIST  g_server_mem_mgr;

//删除配置信息回调函数
VOID DelServerCallBack(PLIST_ENTRY server)
{
	ASSERT(CONTAINING_RECORD(server, SERVER_INFO_LIST_ENTRY, list_entry)->ref_count == 0);
	FreeServer(CONTAINING_RECORD(server, SERVER_INFO_LIST_ENTRY, list_entry));
}

PSERVER_INFO_LIST_ENTRY SelectBestServer(IN PLCXL_LOCK_LIST server_list, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader)
{
	PLIST_ENTRY Link;
	PSERVER_INFO_LIST_ENTRY best_server = NULL;

	ASSERT(server_list != NULL);
	UNREFERENCED_PARAMETER(pIPHeader);
	UNREFERENCED_PARAMETER(pTcpHeader);
	UNREFERENCED_PARAMETER(ipMode);

	LockLCXLLockList(server_list);
	Link = GetListofLCXLLockList(server_list)->Flink;
	//遍历列表
	while (Link != GetListofLCXLLockList(server_list))
	{
		PSERVER_INFO_LIST_ENTRY server_info;
		KLOCK_QUEUE_HANDLE lock_handle;
		server_info = CONTAINING_RECORD(Link, SERVER_INFO_LIST_ENTRY, list_entry);
		LockServer(server_info, &lock_handle);
		//检查服务器是否可用
		if ((server_info->info.status&SS_ONLINE) != 0 && (server_info->info.status&SS_ENABLED) != 0 && (server_info->info.status&SS_DELETED) == 0) {
			if (best_server == NULL || best_server->performance.process_time > server_info->performance.process_time) {
				if (best_server != NULL) {
					//减少引用
					DecRefServer(best_server);
				}
				best_server = server_info;
				IncRefServer(best_server);
			}
		}
		UnLockServer(&lock_handle);
		Link = Link->Flink;
	}
	UnlockLCXLLockList(server_list);
	return best_server;
}