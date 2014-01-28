#include "precomp.h"
#include "lcxl_server.h"

NPAGED_LOOKASIDE_LIST  g_server_mem_mgr;

PSERVER_INFO_LIST_ENTRY SelectServer(IN PLIST_ENTRY server_list, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader)
{
	PLIST_ENTRY Link = server_list->Flink;
	PSERVER_INFO_LIST_ENTRY best_server = NULL;

	ASSERT(server_list != NULL);
	UNREFERENCED_PARAMETER(pIPHeader);
	UNREFERENCED_PARAMETER(pTcpHeader);
	UNREFERENCED_PARAMETER(ipMode);

	//遍历列表
	while (Link != server_list)
	{
		PSERVER_INFO_LIST_ENTRY server_info;

		server_info = CONTAINING_RECORD(Link, SERVER_INFO_LIST_ENTRY, list_entry);
		//检查服务器是否可用
		if ((server_info->info.status&SS_ENABLED) != 0 && (server_info->info.status&SS_ONLINE) != 0) {
			if (best_server == NULL || best_server->performance.process_time > server_info->performance.process_time) {
				best_server = server_info;
			}
		}
		Link = Link->Flink;
	}
	if (best_server != NULL) {
		//best_server->ref_count++;
		ASSERT(best_server->ref_count > 0);
		InterlockedIncrement(&best_server->ref_count);
	}
	return best_server;
}