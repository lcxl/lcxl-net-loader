/*++

Copyright (c) Microsoft Corporation

Module Name:

    Filter.h

Abstract:

    This module contains all prototypes and macros for filter code.

Notes:

--*/
#ifndef _FILT_H
#define _FILT_H

#include "lcxl_queue.h"
#include "lcxl_setting.h"
#include "../../common/driver/lcxl_lock_list.h"

#pragma warning(disable:28930) // Unused assignment of pointer, by design in samples
#pragma warning(disable:28931) // Unused assignment of variable, by design in samples

// TODO: Customize these to hint at your component for memory leak tracking.
// These should be treated like a pooltag.
#define FILTER_REQUEST_ID          'RTLF'
#define FILTER_ALLOC_TAG           'tliF'
#define FILTER_TAG                 'dnTF'

// TODO: Specify which version of the NDIS contract you will use here.
// In many cases, 6.0 is the best choice.  You only need to select a later
// version if you need a feature that is not available in 6.0.
//
// Legal values include:
//    6.0  Available starting with Windows Vista RTM
//    6.1  Available starting with Windows Vista SP1 / Windows Server 2008
//    6.20 Available starting with Windows 7 / Windows Server 2008 R2
//    6.30 Available starting with Windows 8 / Windows Server "8"
// Or, just use NDIS_FILTER_MAJOR_VERSION / NDIS_FILTER_MINOR_VERSION
// to pick up whatever version is defined by your build system
// (for example, "-DNDIS630").
#define FILTER_MAJOR_NDIS_VERSION   NDIS_FILTER_MAJOR_VERSION
#define FILTER_MINOR_NDIS_VERSION   NDIS_FILTER_MINOR_VERSION




#define FILTER_FRIENDLY_NAME        L"LCXL Net Loader(NDIS Filter)"
// TODO: Customize this to match the GUID in the INF
#define FILTER_UNIQUE_NAME          L"{A8CFB5DA-09DB-4CB1-93B5-92D347289EB7}" //unique name, quid name
// TODO: Customize this to match the service name in the INF
#define FILTER_SERVICE_NAME         L"NetLoader"
//
// The filter needs to handle IOCTLs
//
#define LINKNAME_STRING             L"\\DosDevices\\netloader"
#define NTDEVICE_STRING             L"\\Device\\netloader"


#if TRACK_RECEIVES
UINT         filterLogReceiveRefIndex = 0;
ULONG_PTR    filterLogReceiveRef[0x10000];
#endif

#if TRACK_SENDS
UINT         filterLogSendRefIndex = 0;
ULONG_PTR    filterLogSendRef[0x10000];
#endif

#if TRACK_RECEIVES
#define   FILTER_LOG_RCV_REF(_O, _Instance, _NetBufferList, _Ref)    \
    {\
        filterLogReceiveRef[filterLogReceiveRefIndex++] = (ULONG_PTR)(_O); \
        filterLogReceiveRef[filterLogReceiveRefIndex++] = (ULONG_PTR)(_Instance); \
        filterLogReceiveRef[filterLogReceiveRefIndex++] = (ULONG_PTR)(_NetBufferList); \
        filterLogReceiveRef[filterLogReceiveRefIndex++] = (ULONG_PTR)(_Ref); \
        if (filterLogReceiveRefIndex >= (0x10000 - 5))                    \
        {                                                              \
            filterLogReceiveRefIndex = 0;                                 \
        }                                                              \
    }
#else
#define   FILTER_LOG_RCV_REF(_O, _Instance, _NetBufferList, _Ref)
#endif

#if TRACK_SENDS
#define   FILTER_LOG_SEND_REF(_O, _Instance, _NetBufferList, _Ref)    \
    {\
        filterLogSendRef[filterLogSendRefIndex++] = (ULONG_PTR)(_O); \
        filterLogSendRef[filterLogSendRefIndex++] = (ULONG_PTR)(_Instance); \
        filterLogSendRef[filterLogSendRefIndex++] = (ULONG_PTR)(_NetBufferList); \
        filterLogSendRef[filterLogSendRefIndex++] = (ULONG_PTR)(_Ref); \
        if (filterLogSendRefIndex >= (0x10000 - 5))                    \
        {                                                              \
            filterLogSendRefIndex = 0;                                 \
        }                                                              \
    }

#else
#define   FILTER_LOG_SEND_REF(_O, _Instance, _NetBufferList, _Ref)
#endif


//
// DEBUG related macros.
//
#if DBG
#define FILTER_ALLOC_MEM(_NdisHandle, _Size)    \
    filterAuditAllocMem(                        \
            _NdisHandle,                        \
           _Size,                               \
           __FILENUMBER,                        \
           __LINE__);

#define FILTER_FREE_MEM(_pMem)                  \
    filterAuditFreeMem(_pMem);

#else
#define FILTER_ALLOC_MEM(_NdisHandle, _Size)     \
    NdisAllocateMemoryWithTagPriority(_NdisHandle, _Size, FILTER_ALLOC_TAG, LowPoolPriority)

#define FILTER_FREE_MEM(_pMem)    NdisFreeMemory(_pMem, 0, 0)

#endif //DBG

#if DBG_SPIN_LOCK
#define FILTER_INIT_LOCK(_pLock)                          \
    filterAllocateSpinLock(_pLock, __FILENUMBER, __LINE__)

#define FILTER_FREE_LOCK(_pLock)       filterFreeSpinLock(_pLock)


#define FILTER_ACQUIRE_LOCK(_pLock, DispatchLevel)  \
    filterAcquireSpinLock(_pLock, __FILENUMBER, __LINE__, DisaptchLevel)

#define FILTER_RELEASE_LOCK(_pLock, DispatchLevel)      \
    filterReleaseSpinLock(_pLock, __FILENUMBER, __LINE__, DispatchLevel)

#else
#define FILTER_INIT_LOCK(_pLock)      NdisAllocateSpinLock(_pLock)

#define FILTER_FREE_LOCK(_pLock)      NdisFreeSpinLock(_pLock)

#define FILTER_ACQUIRE_LOCK(_pLock, DispatchLevel)              \
    {                                                           \
        if (DispatchLevel)                                      \
        {                                                       \
            NdisDprAcquireSpinLock(_pLock);                     \
        }                                                       \
        else                                                    \
        {                                                       \
            NdisAcquireSpinLock(_pLock);                        \
        }                                                       \
    }

#define FILTER_RELEASE_LOCK(_pLock, DispatchLevel)              \
    {                                                           \
        if (DispatchLevel)                                      \
        {                                                       \
            NdisDprReleaseSpinLock(_pLock);                     \
        }                                                       \
        else                                                    \
        {                                                       \
            NdisReleaseSpinLock(_pLock);                        \
        }                                                       \
    }
#endif //DBG_SPIN_LOCK


#define NET_BUFFER_LIST_LINK_TO_ENTRY(_pNBL)    ((PQUEUE_ENTRY)(NET_BUFFER_LIST_NEXT_NBL(_pNBL)))
#define ENTRY_TO_NET_BUFFER_LIST(_pEnt)         (CONTAINING_RECORD((_pEnt), NET_BUFFER_LIST, Next))



//添加代码

//move from drv_interface
#define MAX_FILTER_INSTANCE_NAME_LENGTH     256
#define MAX_FILTER_CONFIG_KEYWORD_LENGTH    256
typedef struct _FILTER_DRIVER_ALL_STAT
{
	ULONG          AttachCount;
	ULONG          DetachCount;
	ULONG          ExternalRequestFailedCount;
	ULONG          ExternalRequestSuccessCount;
	ULONG          InternalRequestFailedCount;
} FILTER_DRIVER_ALL_STAT, *PFILTER_DRIVER_ALL_STAT;


typedef struct _FILTER_SET_OID
{
	WCHAR           InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
	ULONG           InstanceNameLength;
	NDIS_OID        Oid;
	NDIS_STATUS     Status;
	UCHAR           Data[sizeof(ULONG)];

}FILTER_SET_OID, *PFILTER_SET_OID;

typedef struct _FILTER_QUERY_OID
{
	WCHAR           InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
	ULONG           InstanceNameLength;
	NDIS_OID        Oid;
	NDIS_STATUS     Status;
	UCHAR           Data[sizeof(ULONG)];

}FILTER_QUERY_OID, *PFILTER_QUERY_OID;

typedef struct _FILTER_READ_CONFIG
{
	_Field_size_bytes_part_(MAX_FILTER_INSTANCE_NAME_LENGTH, InstanceNameLength)
	WCHAR                   InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
	ULONG                   InstanceNameLength;
	_Field_size_bytes_part_(MAX_FILTER_CONFIG_KEYWORD_LENGTH, KeywordLength)
		WCHAR                   Keyword[MAX_FILTER_CONFIG_KEYWORD_LENGTH];
	ULONG                   KeywordLength;
	NDIS_PARAMETER_TYPE     ParameterType;
	NDIS_STATUS             Status;
	UCHAR                   Data[sizeof(ULONG)];
}FILTER_READ_CONFIG, *PFILTER_READ_CONFIG;

typedef struct _FILTER_WRITE_CONFIG
{
	_Field_size_bytes_part_(MAX_FILTER_INSTANCE_NAME_LENGTH, InstanceNameLength)
	WCHAR                   InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
	ULONG                   InstanceNameLength;
	_Field_size_bytes_part_(MAX_FILTER_CONFIG_KEYWORD_LENGTH, KeywordLength)
		WCHAR                   Keyword[MAX_FILTER_CONFIG_KEYWORD_LENGTH];
	ULONG                   KeywordLength;
	NDIS_PARAMETER_TYPE     ParameterType;
	NDIS_STATUS             Status;
	UCHAR                   Data[sizeof(ULONG)];
}FILTER_WRITE_CONFIG, *PFILTER_WRITE_CONFIG;

//
// Enum of filter's states
// Filter can only be in one state at one time
//
typedef enum _FILTER_STATE
{
    FilterStateUnspecified,
    FilterInitialized,
    FilterPausing,
    FilterPaused,
    FilterRunning,
    FilterRestarting,
    FilterDetaching
} FILTER_STATE;


typedef struct _FILTER_REQUEST
{
    NDIS_OID_REQUEST       Request;
    NDIS_EVENT             ReqEvent;
    NDIS_STATUS            Status;
} FILTER_REQUEST, *PFILTER_REQUEST;

//
// Define the filter struct
//
typedef struct _LCXL_FILTER
{
    LIST_ENTRY						filter_module_link;
	//attach_paramters参数只有在attach的时候才能使用
	//PNDIS_FILTER_ATTACH_PARAMETERS	attach_paramters;

    NDIS_HANDLE                     filter_handle;

    NDIS_STATUS                     status;
    NDIS_EVENT                      event;
    ULONG                           back_fill_size;
    KSPIN_LOCK                     lock;    // Lock for protection of state and outstanding sends and recvs
	//过滤驱动当前状态
    volatile FILTER_STATE           state; 
    ULONG                           outstanding_sends;
    ULONG                           outstanding_request;
    ULONG                           outstanding_rcvs;
    FILTER_LOCK                     send_lock;
    FILTER_LOCK                     rcv_lock;
    QUEUE_HEADER                    send_nbl_queue;
    QUEUE_HEADER                    rcv_nbl_queue;

    ULONG                           calls_restart;
    BOOLEAN                         track_receives;
    BOOLEAN                         track_sends;
#if DBG
    BOOLEAN                         bIndicating;
#endif

    PNDIS_OID_REQUEST               pending_oid_request;
    //添加的代码
    //存储模块指针
	LCXL_MODULE_SETTING_INFO		module;
	//路由信息
	LIST_ENTRY						route_list;//LCXL_ROUTE_LIST_ENTRY
    //NBL发送池
    NDIS_HANDLE                     send_net_buffer_list_pool;
    //!添加的代码!
}LCXL_FILTER, *PLCXL_FILTER;

typedef struct _FILTER_DEVICE_EXTENSION
{
    ULONG            Signature;
    NDIS_HANDLE      Handle;
} FILTER_DEVICE_EXTENSION, *PFILTER_DEVICE_EXTENSION;


#define FILTER_READY_TO_PAUSE(_Filter)      \
    ((_Filter)->state == FilterPausing)

//
// The driver should maintain a list of NDIS filter handles
//
typedef struct _FL_NDIS_FILTER_LIST
{
    LIST_ENTRY              Link;
    NDIS_HANDLE             ContextHandle;
    NDIS_STRING             FilterInstanceName;
} FL_NDIS_FILTER_LIST, *PFL_NDIS_FILTER_LIST;

//
// The context inside a cloned request
//
typedef struct _NDIS_OID_REQUEST *FILTER_REQUEST_CONTEXT,**PFILTER_REQUEST_CONTEXT;

//
// Global variables
//
extern NDIS_HANDLE         g_filter_driver_handle; // NDIS handle for filter driver
extern NDIS_HANDLE         g_filter_driver_object;
extern NDIS_HANDLE         g_ndis_filter_device_handle;
extern PDEVICE_OBJECT      g_device_object;

extern LCXL_LOCK_LIST		g_filter_list;
//配置信息
extern LCXL_SETTING			g_setting;


//
// function prototypes
//
DRIVER_INITIALIZE DriverEntry;

FILTER_SET_OPTIONS FilterRegisterOptions;

FILTER_ATTACH FilterAttach;

FILTER_DETACH FilterDetach;

DRIVER_UNLOAD FilterUnload;

FILTER_RESTART FilterRestart;

FILTER_PAUSE FilterPause;

FILTER_OID_REQUEST FilterOidRequest;

FILTER_CANCEL_OID_REQUEST FilterCancelOidRequest;

FILTER_STATUS FilterStatus;

FILTER_DEVICE_PNP_EVENT_NOTIFY FilterDevicePnPEventNotify;

FILTER_NET_PNP_EVENT FilterNetPnPEvent;

FILTER_OID_REQUEST_COMPLETE FilterOidRequestComplete;

FILTER_SEND_NET_BUFFER_LISTS FilterSendNetBufferLists;

FILTER_RETURN_NET_BUFFER_LISTS FilterReturnNetBufferLists;

FILTER_SEND_NET_BUFFER_LISTS_COMPLETE FilterSendNetBufferListsComplete;

FILTER_RECEIVE_NET_BUFFER_LISTS FilterReceiveNetBufferLists;

FILTER_CANCEL_SEND_NET_BUFFER_LISTS FilterCancelSendNetBufferLists;

FILTER_SET_MODULE_OPTIONS FilterSetModuleOptions;


_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS
FilterRegisterDevice(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
FilterDeregisterDevice(
    VOID
    );

DRIVER_DISPATCH FilterDispatch;

DRIVER_DISPATCH FilterDeviceIoControl;

_IRQL_requires_max_(DISPATCH_LEVEL)
PLCXL_FILTER
filterFindFilterModule(
    _In_reads_bytes_(BufferLength)
         PUCHAR                   Buffer,
    _In_ ULONG                    BufferLength
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NDIS_STATUS
filterDoInternalRequest(
    _In_ PLCXL_FILTER                   FilterModuleContext,
    _In_ NDIS_REQUEST_TYPE            RequestType,
    _In_ NDIS_OID                     Oid,
    _Inout_updates_bytes_to_(InformationBufferLength, *pBytesProcessed)
         PVOID                        InformationBuffer,
    _In_ ULONG                        InformationBufferLength,
    _In_opt_ ULONG                    OutputBufferLength,
    _In_ ULONG                        MethodId,
    _Out_ PULONG                      pBytesProcessed
    );

VOID
filterInternalRequestComplete(
    _In_ NDIS_HANDLE                  FilterModuleContext,
    _In_ PNDIS_OID_REQUEST            NdisRequest,
    _In_ NDIS_STATUS                  Status
    );

//驱动初始化使用
VOID DriverReinitialize(
	_In_      struct _DRIVER_OBJECT *DriverObject,
	_In_opt_  PVOID Context,
	_In_      ULONG Count
	);
typedef struct _NPROT_SEND_NETBUFLIST_RSVD{
	ULONG                  owner_packet;
	NDIS_HANDLE            filter;
} NPROT_SEND_NETBUFLIST_RSVD, *PNPROT_SEND_NETBUFLIST_RSVD;

///<summary>
///路由TCP数据包
///返回路由信息
///</summary>
PLCXL_ROUTE_LIST_ENTRY RouteTCPNBL(IN PLCXL_FILTER filter, IN INT ip_mode, IN PVOID ip_header);

//************************************
// 简介: 检查TCP数据包的mac地址是不是我们所关心的地址
// 返回: BOOLEAN
// 参数: IN PIF_PHYSICAL_ADDRESS mac_addr
// 参数: IN BOOLEAN is_recv
// 参数: IN PETHERNET_HEADER pEthHeader
//************************************
BOOLEAN CheckTCPNBLMacAddr(IN PIF_PHYSICAL_ADDRESS mac_addr, IN BOOLEAN is_recv, IN PETHERNET_HEADER eth_header);

typedef enum _PROCESS_NBL_RESULT_CODE {
	//数据包可以通过
	PNRC_PASS = 0,
	//拦截此数据包
	PNRC_DROP = 1,
	//要更改此数据包
	PNRC_MODIFY = 2,
	//要路由此数据包
	PNRC_ROUTER = 3,
	//处理ICMP NS数据包
	PNRC_ICMPV6_NS = 4,
	//处理ICMP NA数据包
	PNRC_ICMPV6_NA = 5
} PROCESS_NBL_RESULT_CODE, *PPROCESS_NBL_RESULT_CODE;


typedef struct _PROCESS_NBL_RESULT{
	PROCESS_NBL_RESULT_CODE code;
	union {
		//路由信息
		PLCXL_ROUTE_LIST_ENTRY	route;
		/*
		//ss
		struct {
			//IP头
			IPV6_HEADER ip_header;
			//邻居发现协议头
			struct {
				ND_NEIGHBOR_ADVERT_HEADER neighber_advert;
				//option
				ND_OPTION_HDR option;
				//目标
				DL_EUI48 target;
			} na;
			
		} icmpns;
		*/
		LCXL_IP			modifyip;//PNRC_MODIFY
	} data;
} PROCESS_NBL_RESULT, *PPROCESS_NBL_RESULT;

//************************************
// 简介: 处理此NBL
// 返回: BOOLEAN TRUE:已处理，不要交给上层驱动程序；FALSE:未处理，交给上层处理程序
// 参数: IN PLCXL_FILTER pFilter
// 参数: IN PETHERNET_HEADER pEthHeader 以太网协议头
// 参数: IN UINT BufferLength
// 参数: IN OUT PROCESS_NBL_RETURN_DATA return_data 如果需要路由此NBL，返回路由信息，否则返回NULL。注意，只有当函数返回为FALSE时此值恒为NULL
//************************************
VOID ProcessNBL(IN PLCXL_FILTER filter, IN BOOLEAN is_recv, IN INT lcxl_role, IN PETHERNET_HEADER eth_header, IN UINT data_length, IN OUT PPROCESS_NBL_RESULT return_data);

VOID ProcessARP(IN PLCXL_FILTER filter, IN BOOLEAN is_recv, IN INT lcxl_role, IN PARP_HEADER arp_header, IN PLCXL_ADDR_INFO virtual_addr, IN OUT PPROCESS_NBL_RESULT return_data);
//************************************
// 简介: 处理ICMPv6数据包
// 返回: VOID
// 参数: IN PLCXL_FILTER filter
// 参数: IN BOOLEAN is_recv
// 参数: IN INT lcxl_role
// 参数: IN PIPV6_HEADER ip_header
// 参数: IN PLCXL_ADDR_INFO virtual_addr
// 参数: IN OUT PPROCESS_NBL_RESULT return_data
//************************************
VOID ProcessICMPv6(IN PLCXL_FILTER filter, IN BOOLEAN is_recv, IN INT lcxl_role, IN PIPV6_HEADER ip_header, IN PLCXL_ADDR_INFO virtual_addr, IN OUT PPROCESS_NBL_RESULT return_data);
//************************************
// 简介: 处理TCP数据包
// 返回: VOID
// 参数: IN PLCXL_FILTER filter
// 参数: IN BOOLEAN is_recv
// 参数: IN INT lcxl_role
// 参数: IN PVOID ip_header
// 参数: IN INT ipMode
// 参数: IN PLCXL_ADDR_INFO virtual_addr
// 参数: IN OUT PPROCESS_NBL_RESULT return_data
//************************************
VOID ProcessTCP(IN PLCXL_FILTER filter, IN BOOLEAN is_recv, IN INT lcxl_role, IN PVOID ip_header, IN INT ip_mode, IN PLCXL_ADDR_INFO virtual_addr, IN OUT PPROCESS_NBL_RESULT return_data);

//************************************
// 简介: 处理UDP包
// 返回: VOID
// 参数: IN PLCXL_FILTER filter
// 参数: IN BOOLEAN is_recv
// 参数: IN INT lcxl_role
// 参数: IN PVOID ip_header
// 参数: IN INT ipMode
// 参数: IN PLCXL_ADDR_INFO virtual_addr
// 参数: IN OUT PPROCESS_NBL_RESULT return_data
//************************************
VOID ProcessUDP(IN PLCXL_FILTER filter, IN BOOLEAN is_recv, IN INT lcxl_role, IN PVOID ip_header, IN INT ip_mode, IN PLCXL_ADDR_INFO virtual_addr, IN OUT PPROCESS_NBL_RESULT return_data);

NTSTATUS CreateNBL(IN PLCXL_FILTER filter, IN PETHERNET_HEADER send_buffer, IN UINT datalen, OUT PNET_BUFFER_LIST *nbl);

PNET_BUFFER_LIST DropOwnerNBL(IN PLCXL_FILTER filter, IN PNET_BUFFER_LIST NetBufferLists);
//************************************
// 简介: 寻找LCXL_FILTER结构，注意，需要锁定表之后才能使用
// 返回: PLCXL_FILTER
// 参数: IN NET_LUID miniport_net_luid 网卡的LUID
//************************************
PLCXL_FILTER FindFilter(IN NET_LUID miniport_net_luid);

//************************************
// 简介: 锁定filter
// 返回: void
// 参数: IN PLCXL_FILTER filter
// 参数: OUT PKLOCK_QUEUE_HANDLE lock_handle_in_stack
//************************************
__inline void LockFilter(IN PLCXL_FILTER filter, OUT PKLOCK_QUEUE_HANDLE lock_handle_in_stack) {
	ASSERT(filter != NULL && lock_handle_in_stack != NULL);
	KeAcquireInStackQueuedSpinLock(&filter->lock, lock_handle_in_stack);
}

//************************************
// 简介: 解锁filter
// 返回: void
// 参数: IN PKLOCK_QUEUE_HANDLE lock_handle_in_stack
//************************************
__inline void UnlockFilter(IN PKLOCK_QUEUE_HANDLE lock_handle_in_stack) {
	ASSERT(lock_handle_in_stack != NULL);
	KeReleaseInStackQueuedSpinLock(lock_handle_in_stack);
}

void DelLCXLFilterCallBack(PLIST_ENTRY filter);
#endif  //_FILT_H


