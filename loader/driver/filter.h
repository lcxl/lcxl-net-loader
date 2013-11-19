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
#include "../common/driver/lcxl_net.h"
#include "../../common/lcxl_type.h"
#pragma warning(disable:28930) // Unused assignment of pointer, by design in samples
#pragma warning(disable:28931) // Unused assignment of variable, by design in samples

// TODO: Customize these to hint at your component for memory leak tracking.
// These should be treated like a pooltag.
#define FILTER_REQUEST_ID          'RTLF'
#define FILTER_ALLOC_TAG           'tliF'
#define FILTER_TAG                 'dnTF'
#define TAG_FILE_BUFFER				'FISR'
#define TAG_MODULE					'MODU'
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

//配置文件
#define LOADER_SETTING_FILE_PATH    L"\\SystemRoot\\System32\\drivers\\etc\\lcxl_loader"

//
// Types and macros to manipulate packet queue
//
typedef struct _QUEUE_ENTRY
{
    struct _QUEUE_ENTRY * Next;
}QUEUE_ENTRY, *PQUEUE_ENTRY;

typedef struct _QUEUE_HEADER
{
    PQUEUE_ENTRY     Head;
    PQUEUE_ENTRY     Tail;
} QUEUE_HEADER, PQUEUE_HEADER;


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

#define InitializeQueueHeader(_QueueHeader)             \
{                                                       \
    (_QueueHeader)->Head = (_QueueHeader)->Tail = NULL; \
}

//
// Macros for queue operations
//
#define IsQueueEmpty(_QueueHeader)      ((_QueueHeader)->Head == NULL)

#define RemoveHeadQueue(_QueueHeader)                   \
    (_QueueHeader)->Head;                               \
    {                                                   \
        PQUEUE_ENTRY pNext;                             \
        ASSERT((_QueueHeader)->Head);                   \
        pNext = (_QueueHeader)->Head->Next;             \
        (_QueueHeader)->Head = pNext;                   \
        if (pNext == NULL)                              \
            (_QueueHeader)->Tail = NULL;                \
    }

#define InsertHeadQueue(_QueueHeader, _QueueEntry)                  \
    {                                                               \
        ((PQUEUE_ENTRY)(_QueueEntry))->Next = (_QueueHeader)->Head; \
        (_QueueHeader)->Head = (PQUEUE_ENTRY)(_QueueEntry);         \
        if ((_QueueHeader)->Tail == NULL)                           \
            (_QueueHeader)->Tail = (PQUEUE_ENTRY)(_QueueEntry);     \
    }

#define InsertTailQueue(_QueueHeader, _QueueEntry)                      \
    {                                                                   \
        ((PQUEUE_ENTRY)(_QueueEntry))->Next = NULL;                     \
        if ((_QueueHeader)->Tail)                                       \
            (_QueueHeader)->Tail->Next = (PQUEUE_ENTRY)(_QueueEntry);   \
        else                                                            \
            (_QueueHeader)->Head = (PQUEUE_ENTRY)(_QueueEntry);         \
        (_QueueHeader)->Tail = (PQUEUE_ENTRY)(_QueueEntry);             \
    }

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
//!!


//服务器性能
typedef struct _LCXL_SERVER_PERFORMANCE
{
    //单个任务的平均处理时间，时间单位为微妙（us）
    //Windows下使用KeQueryPerformanceCounter
    unsigned long       process_time;
    //总内存数
    unsigned long long  total_memory;
    //当前使用内存
    unsigned long long  cur_memory;
    //CPU使用率，最高为1
    double              cpu_usage;
} LCXL_SERVER_PERFORMANCE, *PLCXL_SERVER_PERFORMANCE;

typedef struct _LCXL_SERVER_INFO {
#define SS_ENABLED	0x01//服务器处于启用状态
#define SS_ONLINE	0x02//服务器在线
	//服务器状态
	UCHAR				status;
	//计算机名
	CHAR				computer_name[256];
	//服务器真实IP地址
	LCXL_SERVER_ADDR	addr;
} LCXL_SERVER_INFO, *PLCXL_SERVER_INFO;

//服务器信息列表项
typedef struct _SERVER_INFO_LIST_ENTRY
{
	//列表项
	LIST_ENTRY				list_entry;
	//引用计数
	volatile LONG  			ref_count;
	//服务器状态
	LCXL_SERVER_INFO		info;
	//服务器性能状态
	LCXL_SERVER_PERFORMANCE	performance;
} SERVER_INFO_LIST_ENTRY, *PSERVER_INFO_LIST_ENTRY;


//路由信息
typedef struct _LCXL_ROUTE_LIST_ENTRY
{
	LIST_ENTRY		        list_entry;		//列表项
#define RS_NONE     0x00
#define RS_NORMAL   0x01					//正常
#define RS_LAST_ACK 0x02					//正在等待最后一个ACK包
#define RS_CLOSED   0x03					//连接已关闭
    int                     status;         //连接状态
	LCXL_IP					src_ip;
	//TCP
	unsigned short	        src_port;		//源端口号
	unsigned short	        dst_port;		//目的端口号
	PSERVER_INFO_LIST_ENTRY dst_server;	    //目标服务器
} LCXL_ROUTE_LIST_ENTRY, *PLCXL_ROUTE_LIST_ENTRY;
//前置申明
typedef struct _LCXL_MODULE_LIST_ENTRY LCXL_MODULE_LIST_ENTRY, *PLCXL_MODULE_LIST_ENTRY;
//!添加代码!
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
    LIST_ENTRY						FilterModuleLink;
    //Reference to this filter
    ULONG                           RefCount;

    NDIS_HANDLE                     FilterHandle;
	//模块名称
	NDIS_STRING						FilterModuleName;
	//小端口驱动友好名称
	NDIS_STRING						MiniportFriendlyName;
	//小端口驱动名称
	NDIS_STRING						MiniportName;
    //小端口驱动网络接口序号
    NET_IFINDEX                     MiniportIfIndex;

    NDIS_STATUS                     Status;
    NDIS_EVENT                      Event;
    ULONG                           BackFillSize;
    FILTER_LOCK                     Lock;    // Lock for protection of state and outstanding sends and recvs
	//过滤驱动当前状态
    FILTER_STATE                    State; 
    ULONG                           OutstandingSends;
    ULONG                           OutstandingRequest;
    ULONG                           OutstandingRcvs;
    FILTER_LOCK                     SendLock;
    FILTER_LOCK                     RcvLock;
    QUEUE_HEADER                    SendNBLQueue;
    QUEUE_HEADER                    RcvNBLQueue;


    NDIS_STRING                     FilterName;
    ULONG                           CallsRestart;
    BOOLEAN                         TrackReceives;
    BOOLEAN                         TrackSends;
#if DBG
    BOOLEAN                         bIndicating;
#endif

    PNDIS_OID_REQUEST               PendingOidRequest;
    //添加的代码
	//网卡本地唯一ID
	NET_LUID						miniport_net_luid;
    //存储模块指针
	PLCXL_MODULE_LIST_ENTRY			module;
	//路由信息
	LCXL_ROUTE_LIST_ENTRY			route_list;
    //NBL发送池
    NDIS_HANDLE                     send_net_buffer_list_pool;
	//MAC地址
	IF_PHYSICAL_ADDRESS_LH			mac_addr;
    //!添加的代码!
}LCXL_FILTER, *PLCXL_FILTER;
//添加代码


//配置文件数据结构
struct _LCXL_MODULE_LIST_ENTRY {
	LIST_ENTRY				list_entry;		//列表项
	
	//重启后删除此设置
#define ML_DELETE_AFTER_RESTART 0x1
	//标识
	INT						flag;
	//对应的Filter结构
	PLCXL_FILTER			filter;
	//真实地址
	LCXL_SERVER_ADDR		real_addr;
	//虚拟IPv4
	IN_ADDR					virtual_ipv4;
	//虚拟IPv6
	IN6_ADDR				virtual_ipv6;
	//网卡本地唯一ID
	NET_LUID				miniport_net_luid;

	//模块名称
	NDIS_STRING             filter_module_name;
	//小端口驱动友好名称
	NDIS_STRING             miniport_friendly_name;
	//小端口驱动名称
	NDIS_STRING             miniport_name;
	
	//服务器数量
	INT						server_count;
	//服务器列表
	SERVER_INFO_LIST_ENTRY	server_list;
	// 发送接收锁
	FILTER_LOCK             server_lock;
};

typedef struct _LCXL_SETTING{
	INT module_count;
	LCXL_MODULE_LIST_ENTRY module_list;
} LCXL_SETTING, *PLCXL_SETTING;
//!添加代码!

typedef struct _FILTER_DEVICE_EXTENSION
{
    ULONG            Signature;
    NDIS_HANDLE      Handle;
} FILTER_DEVICE_EXTENSION, *PFILTER_DEVICE_EXTENSION;


#define FILTER_READY_TO_PAUSE(_Filter)      \
    ((_Filter)->State == FilterPausing)

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
extern NDIS_HANDLE         g_FilterDriverHandle; // NDIS handle for filter driver
extern NDIS_HANDLE         g_FilterDriverObject;
extern NDIS_HANDLE         g_NdisFilterDeviceHandle;
extern PDEVICE_OBJECT      g_DeviceObject;

extern FILTER_LOCK         g_FilterListLock;
extern LIST_ENTRY          g_FilterModuleList;
//配置信息
extern LCXL_SETTING			g_Setting;


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


//添加代码

NTSTATUS LCXLCopyString(IN OUT PUNICODE_STRING dest, IN PUNICODE_STRING sour);

__inline PUCHAR LCXLReadFromBuf(IN PUCHAR cur_buf, IN OUT PVOID data, IN INT datalen)
{
	RtlCopyMemory(data, cur_buf, datalen);
	return cur_buf + datalen;
}

__inline PUCHAR LCXLReadStringFromBuf(IN PUCHAR cur_buf, IN OUT PUNICODE_STRING data)
{
	return LCXLReadFromBuf(LCXLReadFromBuf(cur_buf, &data->Length, sizeof(data->Length)), data->Buffer, data->Length);
}

__inline PUCHAR LCXLWriteToBuf(IN PUCHAR cur_buf, IN PVOID data, IN INT datalen)
{
	RtlCopyMemory(cur_buf, data, datalen);
	return cur_buf + datalen;
}


__inline PUCHAR LCXLWriteStringToBuf(IN PUCHAR cur_buf, IN PUNICODE_STRING data)
{
	return LCXLWriteToBuf(LCXLWriteToBuf(cur_buf, &data->Length, sizeof(data->Length)), data->Buffer, data->Length);
}

//驱动初始化使用
VOID DriverReinitialize(
	_In_      struct _DRIVER_OBJECT *DriverObject,
	_In_opt_  PVOID Context,
	_In_      ULONG Count
	);

///<summary>
///加载配置文件
///</summary>
VOID LoadSetting();
//从设置模块中加载设置
PLCXL_MODULE_LIST_ENTRY LoadModuleSetting(IN PLCXL_FILTER filter);
//寻找设置模块所对应的filter并设置此filter的信息
PLCXL_FILTER SetFilterSetting(PLCXL_MODULE_LIST_ENTRY module);

///<summary>
///保存配置文件
///</summary>
VOID SaveSetting();
///<summary>
///路由TCP数据包
///</summary>
PLCXL_ROUTE_LIST_ENTRY RouteTCPNBL(IN PLCXL_FILTER pFilter, IN INT ipMode, IN PVOID pIPHeader);

///<summary>
///是否路由此NBL，如果需要路由此NBL，返回路由信息，否则返回NULL
///</summary>
PLCXL_ROUTE_LIST_ENTRY IfRouteNBL(IN PLCXL_FILTER pFilter, IN PETHERNET_HEADER pEthHeader, IN UINT BufferLength);

///<summary>
///获取路由信息项
///</summary>
PLCXL_ROUTE_LIST_ENTRY GetRouteListEntry(IN PLCXL_FILTER pFilter, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader);

///<summary>
///获取路由信息项IPv4
///</summary>
__inline  PLCXL_ROUTE_LIST_ENTRY GetRouteListEntry4(IN PLCXL_FILTER pFilter, IN PIPV4_HEADER pIPHeader, IN PTCP_HDR pTcpHeader)
{
	return GetRouteListEntry(pFilter, IM_IPV4, pIPHeader, pTcpHeader);
}
///<summary>
///获取路由信息项IPv6
///</summary>
__inline PLCXL_ROUTE_LIST_ENTRY GetRouteListEntry6(IN PLCXL_FILTER pFilter, IN PIPV6_HEADER pIPHeader, IN PTCP_HDR pTcpHeader)
{
	return GetRouteListEntry(pFilter, IM_IPV6, pIPHeader, pTcpHeader);
}

///<summary>
//选择服务器
///</summary>
PSERVER_INFO_LIST_ENTRY SelectServer(IN PLCXL_FILTER pFilter, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader);

///<summary>
///创建路由信息表项
///</summary>
PLCXL_ROUTE_LIST_ENTRY CreateRouteListEntry(IN PLCXL_FILTER pFilter);
///<summary>
///始化路由信息表项
///</summary>
void InitRouteListEntry(IN OUT PLCXL_ROUTE_LIST_ENTRY route_info, IN INT ipMode, IN PVOID pIPHeader, IN PTCP_HDR pTcpHeader, IN PSERVER_INFO_LIST_ENTRY server_info);


//!添加代码!

#endif  //_FILT_H


