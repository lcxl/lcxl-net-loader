#include "precomp.h"
#include "lcxl_route.h"
#include "lcxl_setting.h"
NPAGED_LOOKASIDE_LIST  g_route_mem_mgr;

PLCXL_ROUTE_LIST_ENTRY CreateRouteListEntry(IN PLIST_ENTRY route_list)
{
	PLCXL_ROUTE_LIST_ENTRY route_info = NULL;

	ASSERT(route_list != NULL);
	route_info = AllocRoute();
	if (route_info != NULL) {
		InsertHeadList(route_list, &route_info->list_entry);
	}
	return route_info;
}

VOID InitRouteListEntry(IN OUT PLCXL_ROUTE_LIST_ENTRY route_info, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader, IN PSERVER_INFO_LIST_ENTRY server_info)
{
	LARGE_INTEGER timestamp;

	ASSERT(route_info != NULL);
	ASSERT(pIPHeader != NULL);
	ASSERT(pTcpHeader != NULL);
	ASSERT(server_info != NULL);

	//获取时间戳
	timestamp = KeQueryPerformanceCounter(NULL);
	route_info->recv_timestamp = timestamp;
	route_info->src_ip.ip_mode = ipMode;
	route_info->status = RS_NORMAL;
	route_info->dst_server = server_info;
	route_info->dst_port = pTcpHeader->th_dport;
	route_info->src_port = pTcpHeader->th_sport;
	switch (ipMode)
	{
	case IM_IPV4:
		route_info->src_ip.addr.ip_4 = ((PIPV4_HEADER)pIPHeader)->SourceAddress;
		break;
	case IM_IPV6:
		route_info->src_ip.addr.ip_6 = ((PIPV6_HEADER)pIPHeader)->SourceAddress;
		break;
	default:
		ASSERT(FALSE);
		break;
	}
}

PLCXL_ROUTE_LIST_ENTRY GetRouteListEntry(IN PLIST_ENTRY route_list, IN INT route_timeout, IN PLCXL_LOCK_LIST server_list, IN INT ip_mode, IN PVOID ip_header, IN PTCP_HDR tcp_header)
{
	union {
		PIPV4_HEADER ipv4_header;
		PIPV6_HEADER ipv6_header;
	} ip_header_union = { 0 };
	//pFilter->route_list.
	PLIST_ENTRY Link = route_list->Flink;
	

	switch (ip_mode) {
	case IM_IPV4:
		ip_header_union.ipv4_header = (PIPV4_HEADER)ip_header;

		break;
	case IM_IPV6:
		ip_header_union.ipv6_header = (PIPV6_HEADER)ip_header;
		break;
	default:
		ASSERT(FALSE);
	}

	LARGE_INTEGER timestamp;
	//获取时间戳
	timestamp = KeQueryPerformanceCounter(NULL);
	//遍历列表
	while (Link != route_list) {
		PLCXL_ROUTE_LIST_ENTRY route_info;

		route_info = CONTAINING_RECORD(Link, LCXL_ROUTE_LIST_ENTRY, list_entry);
		if (ip_mode == route_info->src_ip.ip_mode && route_info->src_port == tcp_header->th_sport && route_info->dst_port == tcp_header->th_dport) {
			switch (ip_mode) {
			case IM_IPV4:
				//查看是否匹配
				if (RtlCompareMemory(&route_info->src_ip.addr.ip_4, &ip_header_union.ipv4_header->SourceAddress, sizeof(ip_header_union.ipv4_header->SourceAddress)) == sizeof(ip_header_union.ipv4_header->SourceAddress)) {
					//更新时间戳
					route_info->recv_timestamp = timestamp;
					return route_info;
				}
				break;
			case IM_IPV6:
				if (RtlCompareMemory(&route_info->src_ip.addr.ip_6, &ip_header_union.ipv6_header->SourceAddress, sizeof(ip_header_union.ipv6_header->SourceAddress)) == sizeof(ip_header_union.ipv6_header->SourceAddress)) {
					//更新时间戳
					route_info->recv_timestamp = timestamp;
					return route_info;
				}
				break;
			default:
				ASSERT(FALSE);
				break;
			}
		}
		Link = Link->Flink;
		//如果路由表项超时
		if ((timestamp.QuadPart - route_info->recv_timestamp.QuadPart) > route_timeout*g_setting.frequency.QuadPart) {
			KdPrint(("SYS:GetRouteListEntry route timeout\n"));
			//删除超时的路由表项
			DeleteRouteListEntry(route_info, server_list);
		}
		
	}
	return NULL;
}

