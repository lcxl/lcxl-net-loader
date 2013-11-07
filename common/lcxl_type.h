#ifndef _LCXL_TYPE_H_
#define _LCXL_TYPE_H_

typedef struct _LCXL_IP {
#define IM_UNKNOWN 0
#define IM_IPV4	1
#define IM_IPV6 2
	int						ip_mode;		//IPģʽ��IPv4����IPv6�� IM_IPV4, IM_IPV6
	union {
		//IP
		IN_ADDR			    ip_4;			//ԴIPv4��ַ
		IN6_ADDR			ip_6;			//ԴIPv6��ַ
	} addr;
} LCXL_IP, *PLCXL_IP;

typedef struct _LCXL_SERVER_ADDR {
#define SA_ENABLE_IPV4 0x01//������������IPV4Э��
#define SA_ENABLE_IPV6 0x02//������������IPV6Э��
	unsigned char	status;
	//��ʵ��IP��ַ
	IN_ADDR			ipv4;
	IN6_ADDR		ipv6;
	//MAC��ַ����
	USHORT			mac_addr_len;
	//MAC��ַ
	UCHAR			mac_addr[NDIS_MAX_PHYS_ADDRESS_LENGTH];
} LCXL_SERVER_ADDR, *PLCXL_SERVER_ADDR;//��������ַ
#endif