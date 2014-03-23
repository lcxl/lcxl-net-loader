#include "precomp.h"
#include "lcxl_server.h"

NPAGED_LOOKASIDE_LIST  g_server_mem_mgr;

//删除配置信息回调函数
VOID DelServerCallBack(PLIST_ENTRY server)
{
	ASSERT(GetServerbyListEntry(server)->list_entry.ref_count == 0);
	FreeServer(GetServerbyListEntry(server));
}
//根据MAC地址寻找服务器
//注意：使用FindServer之前需要锁定列表
PSERVER_INFO_LIST_ENTRY FindServer(IN PLCXL_LOCK_LIST server_list, IN PIF_PHYSICAL_ADDRESS mac_addr)
{
	PLIST_ENTRY Link;

	ASSERT(server_list != NULL && server_list->lock_count > 0);
	Link = GetListofLCXLLockList(server_list)->Flink;
	//遍历列表
	while (Link != GetListofLCXLLockList(server_list)) {
		PSERVER_INFO_LIST_ENTRY server_info;

		server_info = GetServerbyListEntry(Link);
		if (RtlEqualMemory(&server_info->info.mac_addr, mac_addr, sizeof(server_info->info.mac_addr))) {
			return server_info;
		}
		Link = Link->Flink;
	}
	return NULL;
}

PSERVER_INFO_LIST_ENTRY SelectBestServer(IN PLCXL_LOCK_LIST server_list, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader)
{
	PLIST_ENTRY Link;
	PSERVER_INFO_LIST_ENTRY best_server = NULL;

	ASSERT(server_list != NULL);
	UNREFERENCED_PARAMETER(pIPHeader);
	UNREFERENCED_PARAMETER(pTcpHeader);

	LockLCXLLockList(server_list);
	Link = GetListofLCXLLockList(server_list)->Flink;
	//遍历列表
	while (Link != GetListofLCXLLockList(server_list))
	{
		PSERVER_INFO_LIST_ENTRY server_info;
		KLOCK_QUEUE_HANDLE lock_handle;

		server_info = GetServerbyListEntry(Link);
		LockServer(server_info, &lock_handle);
		//检查服务器是否可用
		if ((server_info->info.status&SS_ONLINE) != 0 && 
			(
				(server_info->info.ip_status&SA_ENABLE_IPV6) && (ipMode == IM_IPV6) || 
				(server_info->info.ip_status&SA_ENABLE_IPV4) && (ipMode == IM_IPV4)
			)&& 
			(server_info->info.status&SS_DELETED) == 0) {
			if (best_server == NULL || best_server->performance.process_time > server_info->performance.process_time) {
				if (best_server != NULL) {
					//减少引用
					DecRefListEntry(server_list, &best_server->list_entry);
				}
				best_server = server_info;
				IncRefListEntry(server_list, &best_server->list_entry);
			}
		}
		UnLockServer(&lock_handle);
		Link = Link->Flink;
	}
	UnlockLCXLLockList(server_list);
	return best_server;
}