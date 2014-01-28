#include "precomp.h"
#include "lcxl_route.h"

NPAGED_LOOKASIDE_LIST  g_route_mem_mgr;

PLCXL_ROUTE_LIST_ENTRY CreateRouteListEntry(IN PLIST_ENTRY route_list)
{
	PLCXL_ROUTE_LIST_ENTRY route_info;

	ASSERT(route_list != NULL);
	route_info = ALLOC_ROUTE();
	route_info->status = RS_NONE;
	InsertHeadList(route_list, &route_info->list_entry);
	return route_info;
}

void InitRouteListEntry(IN OUT PLCXL_ROUTE_LIST_ENTRY route_info, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader, IN PSERVER_INFO_LIST_ENTRY server_info)
{
	ASSERT(route_info != NULL);
	ASSERT(pIPHeader != NULL);
	ASSERT(pTcpHeader != NULL);
	ASSERT(server_info != NULL);

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

PLCXL_ROUTE_LIST_ENTRY GetRouteListEntry(IN PLIST_ENTRY route_list, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader)
{
	union {
		PIPV4_HEADER ipv4_header;
		PIPV6_HEADER ipv6_header;
	} ip_header = { 0 };
	//pFilter->route_list.
	PLIST_ENTRY Link = route_list->Flink;
	PLCXL_ROUTE_LIST_ENTRY route_info;

	switch (ipMode) {
	case IM_IPV4:
		ip_header.ipv4_header = (PIPV4_HEADER)pIPHeader;

		break;
	case IM_IPV6:
		ip_header.ipv6_header = (PIPV6_HEADER)pIPHeader;
		break;
	default:
		ASSERT(FALSE);
	}


	//遍历列表
	while (Link != route_list)
	{
		route_info = CONTAINING_RECORD(Link, LCXL_ROUTE_LIST_ENTRY, list_entry);
		if (ipMode == route_info->src_ip.ip_mode && route_info->src_port == pTcpHeader->th_sport && route_info->dst_port == pTcpHeader->th_dport) {
			switch (ipMode)
			{
			case IM_IPV4:
				//查看是否匹配
				if (RtlCompareMemory(&route_info->src_ip.addr.ip_4, &ip_header.ipv4_header->SourceAddress, sizeof(ip_header.ipv4_header->SourceAddress)) == sizeof(ip_header.ipv4_header->SourceAddress)) {
					return route_info;
				}
				break;
			case IM_IPV6:
				if (RtlCompareMemory(&route_info->src_ip.addr.ip_6, &ip_header.ipv6_header->SourceAddress, sizeof(ip_header.ipv6_header->SourceAddress)) == sizeof(ip_header.ipv6_header->SourceAddress)) {
					return route_info;
				}
				break;
			default:
				break;
			}
		}
		Link = Link->Flink;
	}
	return NULL;
}
