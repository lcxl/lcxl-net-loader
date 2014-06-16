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
PSERVER_INFO_LIST_ENTRY FindServer(IN PLCXL_LIST server_list, IN PIF_PHYSICAL_ADDRESS mac_addr)
{
	PLIST_ENTRY Link;

	ASSERT(server_list != NULL && server_list->lock_count > 0);
	Link = GetLcxlListHead(server_list)->Flink;
	//遍历列表
	while (Link != GetLcxlListHead(server_list)) {
		PSERVER_INFO_LIST_ENTRY server_info;

		server_info = GetServerbyListEntry(Link);
		if (RtlEqualMemory(server_info->info.mac_addr.Address, mac_addr->Address, mac_addr->Length)) {
			return server_info;
		}
		Link = Link->Flink;
	}
	return NULL;
}

PSERVER_INFO_LIST_ENTRY FindAvaliableServerFormCurrentServer(IN PLCXL_LIST server_list, IN PSERVER_INFO_LIST_ENTRY server, IN INT ip_mode, IN BOOLEAN checking_last)
{
	PSERVER_INFO_LIST_ENTRY current_server = server;
	PSERVER_INFO_LIST_ENTRY online_server = NULL;
	PSERVER_INFO_LIST_ENTRY checking_server = NULL;

	LockLcxlList(server_list);
	do {
		KLOCK_QUEUE_HANDLE lock_handle;

		LockServer(current_server, &lock_handle);
		if (ServerIsAvaliable(current_server, ip_mode)) {
			if (checking_last) {
				if ((current_server->info.status & SS_CHECKING) != 0) {
					checking_server = current_server;
				} else {
					online_server = current_server;
					break;
				}
			} else {
				//不是checking状态
				online_server = current_server;
				break;
			}
			
		}
		//解锁服务器
		UnLockServer(&lock_handle);
		current_server = GetServerbyListEntry(current_server->list_entry.list_entry.Flink);
		//如果到了标题头部
		if (&current_server->list_entry.list_entry == &server_list->list) {
			current_server = GetServerbyListEntry(current_server->list_entry.list_entry.Flink);
		}
		
	} while (current_server != server);
	if (online_server == NULL) {
		online_server = checking_server;
	}
	//如果找到服务器
	if (online_server != NULL) {
		//增加引用
		IncRefListEntry(server_list, &online_server->list_entry);
	}
	//解锁列表
	UnlockLcxlList(server_list);
	
	return online_server;
}

PSERVER_INFO_LIST_ENTRY SelectBestServer(IN PLCXL_LIST server_list, IN INT ip_mode, IN PVOID ip_header, IN PTCP_HDR tcp_header, IN INT	routing_algorithm, IN PROUTING_ALGORITHM_DATA ra_data)
{
	PLIST_ENTRY Link;
	PSERVER_INFO_LIST_ENTRY best_server = NULL;
	
	ASSERT(server_list != NULL);
	UNREFERENCED_PARAMETER(ip_header);
	UNREFERENCED_PARAMETER(tcp_header);

	LockLcxlList(server_list);
	//查看是否有后端服务器
	if (GetLcxlListCount(server_list) > 0) {
		Link = GetLcxlListHead(server_list)->Flink;
		switch (routing_algorithm) {
		case RA_FAST_RESPONSE://最快响应时间
		case RA_LEAST_CONNECTION://最小连接数
			//遍历列表
			while (Link != GetLcxlListHead(server_list))
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
			PLIST_ENTRY head_link;
			PLIST_ENTRY entry;

			if (ra_data->ra_poll.current_server_index >= GetLcxlListCount(server_list)) {
				ra_data->ra_poll.current_server_index = 0;
			}
			
			head_link = GetLcxlListHead(server_list);
			entry = GetListEntryByIndex(head_link, ra_data->ra_poll.current_server_index%GetLcxlListCount(server_list));
			best_server = FindAvaliableServerFormCurrentServer(server_list, GetServerbyListEntry(entry), ip_mode, TRUE);
			if (best_server != NULL) {
				ra_data->ra_poll.current_server_index = GetListEntryIndex(head_link, &best_server->list_entry.list_entry) + 1;
			}
		}
			break;
		case RA_IP_HASH://IP HASH算法
		{
			INT ip_hash = -1;

			switch (ip_mode)
			{
			case IM_IPV4:
			{
				PIPV4_HEADER ipv4_header;

				ipv4_header = (PIPV4_HEADER)ip_header;
				//获得IP地址的hash值
				ip_hash = ipv4_header->DestinationAddress.S_un.S_addr % GetLcxlListCount(server_list);
			}
				break;
			case IM_IPV6:
			{
				PIPV6_HEADER ipv6_header;

				ipv6_header = (PIPV6_HEADER)ip_header;
				//获得IP地址的hash值
				ip_hash = ((ULONG)ipv6_header->DestinationAddress.u.Word[7]) | ((ULONG)ipv6_header->DestinationAddress.u.Word[6] << 2) % GetLcxlListCount(server_list);
			}
				break;
			default:

				break;
			}
			if (ip_hash > 0) {
				PLIST_ENTRY head_link;
				PLIST_ENTRY entry;

				head_link = GetLcxlListHead(server_list);

				entry = GetListEntryByIndex(head_link, ra_data->ra_poll.current_server_index%GetLcxlListCount(server_list));
				best_server = FindAvaliableServerFormCurrentServer(server_list, GetServerbyListEntry(entry), ip_mode, TRUE);
			}
		}

			break;
			/*
		case RA_FAST_RESPONSE://最快响应时间
			break;
			*/
		default:
			ASSERT(FALSE);
			break;
		}
	}
	//解锁列表
	UnlockLcxlList(server_list);
	return best_server;
}