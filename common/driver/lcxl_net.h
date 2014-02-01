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

#endif