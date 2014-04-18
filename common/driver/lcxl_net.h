#ifndef _LCXL_NET_H_
#define _LCXL_NET_H_

#include <netioddk.h>
//author:LCXL
//abstract:驱动共用的自定义数据包有关结构数据头文件
//#include <ws2def.h>
//#ifndef INCL_WINSOCK_API_PROTOTYPES
//#define INCL_WINSOCK_API_PROTOTYPES

#define ntohs(__A) ((((__A) & 0xff) << 8) | (((__A) & 0xff00) >> 8))
#define ntohl(__A) ((((__A) & 0xff000000) >> 24) | \
	(((__A)& 0x00ff0000) >> 8) | \
	(((__A)& 0x0000ff00) << 8) | \
	(((__A)& 0x000000ff) << 24))
//#endif

typedef struct _LCXL_ARP_ETHERNET {
	USHORT				HardwareAddressSpace;
	USHORT				ProtocolAddressSpace;
	USHORT				Opcode;
	IF_PHYSICAL_ADDRESS	SenderHardwareAddress;
	IN_ADDR				SenderProtocolAddress;
	IF_PHYSICAL_ADDRESS	TargetHardwareAddress;
	IN_ADDR				TargetProtocolAddress;
} LCXL_ARP_ETHERNET, *PLCXL_ARP_ETHERNET;

__inline VOID LCXLReadARPEthernet(IN PARP_HEADER arp_header, IN OUT PLCXL_ARP_ETHERNET lcxl_arp_ethernet)
{
	ASSERT(arp_header != NULL && lcxl_arp_ethernet != NULL);

	lcxl_arp_ethernet->HardwareAddressSpace = ntohs(arp_header->HardwareAddressSpace);
	lcxl_arp_ethernet->ProtocolAddressSpace = ntohs(arp_header->ProtocolAddressSpace);
	lcxl_arp_ethernet->Opcode = ntohs(arp_header->Opcode);

	//获取发送端MAC地址
	lcxl_arp_ethernet->SenderHardwareAddress.Length = sizeof(lcxl_arp_ethernet->SenderHardwareAddress.Address) > arp_header->HardwareAddressLength ? arp_header->HardwareAddressLength : sizeof(lcxl_arp_ethernet->SenderHardwareAddress.Address);
	NdisMoveMemory(lcxl_arp_ethernet->SenderHardwareAddress.Address, arp_header->SenderHardwareAddress, lcxl_arp_ethernet->SenderHardwareAddress.Length);
	//获取发送端IP地址
	NdisMoveMemory(
		&lcxl_arp_ethernet->SenderProtocolAddress,
		arp_header->SenderHardwareAddress + arp_header->HardwareAddressLength,
		arp_header->ProtocolAddressLength > sizeof(lcxl_arp_ethernet->SenderProtocolAddress) ? sizeof(lcxl_arp_ethernet->SenderProtocolAddress) : arp_header->ProtocolAddressLength);

	//获取接受端MAC地址
	lcxl_arp_ethernet->TargetHardwareAddress.Length = sizeof(lcxl_arp_ethernet->TargetHardwareAddress.Address) > arp_header->HardwareAddressLength ? arp_header->HardwareAddressLength : sizeof(lcxl_arp_ethernet->TargetHardwareAddress.Address);
	NdisMoveMemory(lcxl_arp_ethernet->TargetHardwareAddress.Address, arp_header->SenderHardwareAddress + arp_header->HardwareAddressLength + arp_header->ProtocolAddressLength, lcxl_arp_ethernet->TargetHardwareAddress.Length);
	//获取接受端IP地址
	NdisMoveMemory(
		&lcxl_arp_ethernet->TargetProtocolAddress,
		arp_header->SenderHardwareAddress + arp_header->HardwareAddressLength + arp_header->ProtocolAddressLength + arp_header->HardwareAddressLength,
		arp_header->ProtocolAddressLength > sizeof(lcxl_arp_ethernet->TargetProtocolAddress) ? sizeof(lcxl_arp_ethernet->TargetProtocolAddress) : arp_header->ProtocolAddressLength);

}

//************************************
// 简介: 获取以太网数据帧数据头部分
// 返回: PETHERNET_HEADER
// 参数: PNET_BUFFER_LIST nbl
// 参数: UINT * buffer_length
//************************************
__inline PETHERNET_HEADER GetEthernetHeader(PNET_BUFFER_LIST nbl, UINT *data_length) 
{
	PMDL                current_mdl;
	ULONG               buffer_length;
	ULONG               offset;
	PETHERNET_HEADER    ethernet_header = NULL;

	ASSERT(data_length != NULL);
	ASSERT(nbl != NULL);

	current_mdl = NET_BUFFER_CURRENT_MDL(NET_BUFFER_LIST_FIRST_NB(nbl));
	*data_length = NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB(nbl));
	offset = NET_BUFFER_CURRENT_MDL_OFFSET(NET_BUFFER_LIST_FIRST_NB(nbl));
	ASSERT(current_mdl != NULL);
	NdisQueryMdl(
		current_mdl,
		&ethernet_header,
		&buffer_length,
		NormalPagePriority);
	//各种有效性判断
	if (ethernet_header != NULL) {

		ASSERT(buffer_length > offset);
		//获取真正的的包数据
		//获取帧数据头
		ethernet_header = (PETHERNET_HEADER)((PUCHAR)ethernet_header + offset);
	}
	return ethernet_header;
}

//************************************
// 简介: 获取以太网数据帧数据部分
// 返回: PVOID
// 参数: IN PETHERNET_HEADER eth_header
// 参数: IN OUT UINT * buffer_length
//************************************
__inline PVOID GetEthernetData(IN PETHERNET_HEADER eth_header, IN UINT buffer_length, OUT PUSHORT ethernet_type, OUT PUINT data_length)
{
	PVOID data = NULL;

	ASSERT(eth_header != NULL);
	ASSERT(ethernet_type != NULL);
	ASSERT(data_length != NULL);

	if (buffer_length < sizeof(ETHERNET_HEADER)) {
		return NULL;
	}

	*ethernet_type = ntohs(eth_header->Type);
	//判断帧类型是不是8021P_TAG
	if (*ethernet_type == ETHERNET_TYPE_802_1Q) {
		if (buffer_length >= sizeof(ETHERNET_HEADER)+4) {
			*ethernet_type = ntohs(*(USHORT UNALIGNED *)((PUCHAR)&eth_header->Type + 4));
			data = (PUCHAR)eth_header + (sizeof(ETHERNET_HEADER)+4);
		} else {
			//缺代码
			return FALSE;
		}
	} else {
		data = (PUCHAR)eth_header + sizeof(ETHERNET_HEADER);
	}
	if (data == NULL) {
		return FALSE;
	}
	*data_length = buffer_length - (UINT)(UINT_PTR)((PUCHAR)data - (PUCHAR)eth_header);
	return data;
}

//************************************
// 简介: 计算ICMP，IP等数据包的校验和
// 返回: UINT16
// 参数: PVOID * addr
// 参数: int count
//************************************
__inline UINT16 checksum(PUINT16 addr, int count){
	/* Compute Internet Checksum for "count" bytes
	*         beginning at location "addr".
	*/
	register long sum = 0;

	while (count > 1)  {
		/*  This is the inner loop */
		sum += *addr++;
		count -= 2;
	}

	/*  Add left-over byte, if any */
	if (count > 0)
		sum += *(unsigned char *)addr;

	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~(UINT16)sum;
}

#endif