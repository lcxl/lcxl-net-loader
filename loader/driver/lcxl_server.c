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
		if (RtlEqualMemory(server_info->info.mac_addr.Address, mac_addr->Address, mac_addr->Length)) {
			return server_info;
		}
		Link = Link->Flink;
	}
	return NULL;
}

PSERVER_INFO_LIST_ENTRY FindAvaliableServerFormCurrentServer(IN PLCXL_LOCK_LIST server_list, IN PSERVER_INFO_LIST_ENTRY server)
{
	PSERVER_INFO_LIST_ENTRY current_server = server;

	do {

	} while (1);
	return current_server;
}

PSERVER_INFO_LIST_ENTRY SelectBestServer(IN PLCXL_LOCK_LIST server_list, IN INT ip_mode, IN PVOID ip_header, IN PTCP_HDR tcp_header, IN INT	routing_algorithm, IN PROUTING_ALGORITHM_DATA ra_data)
{
	PLIST_ENTRY Link;
	PSERVER_INFO_LIST_ENTRY best_server = NULL;
	
	ASSERT(server_list != NULL);
	UNREFERENCED_PARAMETER(ip_header);
	UNREFERENCED_PARAMETER(tcp_header);

	LockLCXLLockList(server_list);
	Link = GetListofLCXLLockList(server_list)->Flink;
	switch (routing_algorithm) {
	case RA_LEAST_CONNECTION://最小连接数
		//遍历列表
		while (Link != GetListofLCXLLockList(server_list))
		{
			PSERVER_INFO_LIST_ENTRY server_info;
			KLOCK_QUEUE_HANDLE lock_handle;
			//获取server_info的地址
			server_info = GetServerbyListEntry(Link);
			LockServer(server_info, &lock_handle);
			//检查服务器是否可用
			if (ServerIsAvaliable(server_info, ip_mode)) {
				//按照最小连接数来获取服务器（ref_count=路由表项数量+1）
				if (best_server == NULL || best_server->list_entry.ref_count > server_info->list_entry.ref_count || (best_server->info.status & SS_CHECKING) != 0 && (server_info->info.status & SS_CHECKING) == 0) {
					if (best_server != NULL) {
						//减少原来选出来的最好服务器的引用
						DecRefListEntry(server_list, &best_server->list_entry);
					}
					best_server = server_info;
					//增加引用
					IncRefListEntry(server_list, &best_server->list_entry);
				}
			}
			//解锁服务器
			UnLockServer(&lock_handle);
			Link = Link->Flink;
		}
		break;
	case RA_POLL://轮询
	{
		INT max_server_index = 0;
		INT i;
		PLIST_ENTRY head_link;
		//没有后端服务器
		if (GetListCountofLCXLLockList(server_list) == 0) {
			break;
		}
		if (ra_data->ra_poll.current_server_index >= GetListCountofLCXLLockList(server_list)) {
			ra_data->ra_poll.current_server_index = 0;
		}
		max_server_index = ra_data->ra_poll.current_server_index + GetListCountofLCXLLockList(server_list);
		head_link = GetListofLCXLLockList(server_list);
		for (i = ra_data->ra_poll.current_server_index; i < max_server_index; i++) {
			PSERVER_INFO_LIST_ENTRY server_info;
			KLOCK_QUEUE_HANDLE lock_handle;
			PLIST_ENTRY entry;
			entry = GetListEntryByIndex(head_link, i%GetListCountofLCXLLockList(server_list));
			if (entry != NULL) {
				//获取server_info的地址
				server_info = GetServerbyListEntry(entry);
				LockServer(server_info, &lock_handle);
				if (ServerIsAvaliable(server_info, ip_mode)) {
					ra_data->ra_poll.current_server_index = i;
					best_server = server_info;
				}
				UnLockServer(server_info, &lock_handle);
			} else {
				ASSERT(FALSE);
			}
			if (best_server != NULL) {
				break;
			}
		}

	}
		break;
	case RA_IP_HASH://IP HASH算法
	{
		ULONG ip_hash = 0;

		switch (ip_mode)
		{
		case IM_IPV4:
		{
			PIPV4_HEADER ipv4_header;

			ipv4_header = (PIPV4_HEADER)ip_header;
			//获得IP地址的hash值
			ip_hash = ipv4_header->DestinationAddress.S_un.S_addr % GetListCountofLCXLLockList(server_list);
		}
			break;
		case IM_IPV6:
		{
			PIPV6_HEADER ipv6_header;

			ipv6_header = (PIPV6_HEADER)ip_header;
			//获得IP地址的hash值
			ip_hash = ((ULONG)ipv6_header->DestinationAddress.u.Word[7]) | ((ULONG)ipv6_header->DestinationAddress.u.Word[6] << 2) % GetListCountofLCXLLockList(server_list);
		}
			break;
		default:

			break;
		}
	}

		break;
	default:
		ASSERT(FALSE);
		break;
	}
	
	//解锁列表
	UnlockLCXLLockList(server_list);
	return best_server;
}