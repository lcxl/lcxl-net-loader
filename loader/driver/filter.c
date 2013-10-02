/*++

Copyright (c) Microsoft Corporation

Module Name:

    Filter.c

Abstract:

    Sample NDIS Lightweight filter driver

--*/

#include "precomp.h"

#define __FILENUMBER    'PNPF'

// This directive puts the DriverEntry function into the INIT segment of the
// driver.  To conserve memory, the code will be discarded when the driver's
// DriverEntry function returns.  You can declare other functions used only
// during initialization here.
#pragma NDIS_INIT_FUNCTION(DriverEntry)


USHORT UTIL_htons( USHORT hostshort )
{
  PUCHAR  pBuffer;
  USHORT  nResult;

  nResult = 0;
  pBuffer = (PUCHAR )&hostshort;

  nResult = ( (pBuffer[ 0 ] << 8) & 0xFF00) | (pBuffer[ 1 ] & 0x00FF);

  return( nResult );
}


/*UTIL_ntohs把网络字节顺序转换成主机字节顺序*/
USHORT UTIL_ntohs( USHORT netshort )
{
  return( UTIL_htons( netshort ) );
}

//
// Global variables
//
NDIS_HANDLE         g_FilterDriverHandle; // NDIS handle for filter driver
NDIS_HANDLE         g_FilterDriverObject;
NDIS_HANDLE         g_NdisFilterDeviceHandle = NULL;
PDEVICE_OBJECT      g_DeviceObject = NULL;
//过滤列表锁
FILTER_LOCK         g_FilterListLock;
//过滤模块列表
LIST_ENTRY          g_FilterModuleList;

//添加代码
//定义lookaside列表
#define TAG_ROUTE       'ROUT'
#define TAG_SERVER      'SERV'
NPAGED_LOOKASIDE_LIST  g_route_mem_mgr;
NPAGED_LOOKASIDE_LIST  g_server_mem_mgr;

#define GET_MEM_ROUTE() (PLCXL_ROUTE_LIST_ENTRY)ExAllocateFromNPagedLookasideList(&g_route_mem_mgr)
#define FREE_MEM_ROUTE(__buf) ExFreeToNPagedLookasideList(&g_route_mem_mgr, __buf)
#define GET_MEM_SERVER() (PSERVER_INFO_LIST_ENTRY)ExAllocateFromNPagedLookasideList(&g_server_mem_mgr)
#define FREE_MEM_SERVER(__buf) ExFreeToNPagedLookasideList(&g_server_mem_mgr, __buf)
//转发的NBL的内存TAG
#define TAG_SEND_NBL 'SEND'
//!添加代码!

NDIS_FILTER_PARTIAL_CHARACTERISTICS DefaultChars = {
{ 0, 0, 0},
      0,
      FilterSendNetBufferLists,
      FilterSendNetBufferListsComplete,
      NULL,
      FilterReceiveNetBufferLists,
      FilterReturnNetBufferLists
};


_Use_decl_annotations_
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT      DriverObject,
    PUNICODE_STRING     RegistryPath
    )
/*++

Routine Description:

    First entry point to be called, when this driver is loaded.
    Register with NDIS as a filter driver and create a device
    for communication with user-mode.

Arguments:

    DriverObject - pointer to the system's driver object structure
                   for this driver

    RegistryPath - system's registry path for this driver

Return Value:

    STATUS_SUCCESS if all initialization is successful, STATUS_XXX
    error code if not.

--*/
{
    NDIS_STATUS Status;
    NDIS_FILTER_DRIVER_CHARACTERISTICS      FChars;
    NDIS_STRING ServiceName  = RTL_CONSTANT_STRING(FILTER_SERVICE_NAME);
    NDIS_STRING UniqueName   = RTL_CONSTANT_STRING(FILTER_UNIQUE_NAME);
    NDIS_STRING FriendlyName = RTL_CONSTANT_STRING(FILTER_FRIENDLY_NAME);
    BOOLEAN bFalse = FALSE;

    UNREFERENCED_PARAMETER(RegistryPath);

    DEBUGP(DL_TRACE, "===>DriverEntry...\n");

    g_FilterDriverObject = DriverObject;

    do
    {
        //添加代码
        ExInitializeNPagedLookasideList(&g_route_mem_mgr, NULL, NULL, 0, sizeof(LCXL_ROUTE_LIST_ENTRY), TAG_ROUTE, 0);
        ExInitializeNPagedLookasideList(&g_server_mem_mgr, NULL, NULL, 0, sizeof(SERVER_INFO_LIST_ENTRY), TAG_SERVER, 0);
        //!添加代码!

        NdisZeroMemory(&FChars, sizeof(NDIS_FILTER_DRIVER_CHARACTERISTICS));
        FChars.Header.Type = NDIS_OBJECT_TYPE_FILTER_DRIVER_CHARACTERISTICS;
        FChars.Header.Size = sizeof(NDIS_FILTER_DRIVER_CHARACTERISTICS);
#if NDIS_SUPPORT_NDIS61
        FChars.Header.Revision = NDIS_FILTER_CHARACTERISTICS_REVISION_2;
#else
        FChars.Header.Revision = NDIS_FILTER_CHARACTERISTICS_REVISION_1;
#endif

        FChars.MajorNdisVersion = FILTER_MAJOR_NDIS_VERSION;
        FChars.MinorNdisVersion = FILTER_MINOR_NDIS_VERSION;
        FChars.MajorDriverVersion = 1;
        FChars.MinorDriverVersion = 0;
        FChars.Flags = 0;

        FChars.FriendlyName = FriendlyName;
        FChars.UniqueName = UniqueName;
        FChars.ServiceName = ServiceName;

        //
        // TODO: Most handlers are optional, however, this sample includes them
        // all for illustrative purposes.  If you do not need a particular 
        // handler, set it to NULL and NDIS will more efficiently pass the
        // operation through on your behalf.
        //
        FChars.SetOptionsHandler = FilterRegisterOptions;
        FChars.AttachHandler = FilterAttach;
        FChars.DetachHandler = FilterDetach;
        FChars.RestartHandler = FilterRestart;
        FChars.PauseHandler = FilterPause;
        FChars.SetFilterModuleOptionsHandler = FilterSetModuleOptions;
        FChars.OidRequestHandler = FilterOidRequest;
        FChars.OidRequestCompleteHandler = FilterOidRequestComplete;
        FChars.CancelOidRequestHandler = FilterCancelOidRequest;

        FChars.SendNetBufferListsHandler = FilterSendNetBufferLists;
        FChars.ReturnNetBufferListsHandler = FilterReturnNetBufferLists;
        FChars.SendNetBufferListsCompleteHandler = FilterSendNetBufferListsComplete;
        FChars.ReceiveNetBufferListsHandler = FilterReceiveNetBufferLists;
        FChars.DevicePnPEventNotifyHandler = FilterDevicePnPEventNotify;
        FChars.NetPnPEventHandler = FilterNetPnPEvent;
        FChars.StatusHandler = FilterStatus;
        FChars.CancelSendNetBufferListsHandler = FilterCancelSendNetBufferLists;

        DriverObject->DriverUnload = FilterUnload;

        g_FilterDriverHandle = NULL;

        //
        // Initialize spin locks
        //
        FILTER_INIT_LOCK(&g_FilterListLock);

        InitializeListHead(&g_FilterModuleList);

        Status = NdisFRegisterFilterDriver(DriverObject,
                                           (NDIS_HANDLE)g_FilterDriverObject,
                                           &FChars,
                                           &g_FilterDriverHandle);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, "Register filter driver failed.\n");
            break;
        }

        Status = FilterRegisterDevice();

        if (Status != NDIS_STATUS_SUCCESS)
        {
            NdisFDeregisterFilterDriver(g_FilterDriverHandle);
            FILTER_FREE_LOCK(&g_FilterListLock);
            DEBUGP(DL_WARN, "Register device for the filter driver failed.\n");
            break;
        }


    }
    while(bFalse);


    DEBUGP(DL_TRACE, "<===DriverEntry, Status = %8x\n", Status);
    return Status;

}

_Use_decl_annotations_
NDIS_STATUS
FilterRegisterOptions(
    NDIS_HANDLE  NdisFilterDriverHandle,
    NDIS_HANDLE  FilterDriverContext
    )
/*++

Routine Description:

    Register optional handlers with NDIS.  This sample does not happen to
    have any optional handlers to register, so this routine does nothing
    and could simply have been omitted.  However, for illustrative purposes,
    it is presented here.

Arguments:

    NdisFilterDriverHandle - pointer the driver handle received from
                             NdisFRegisterFilterDriver

    FilterDriverContext    - pointer to our context passed into
                             NdisFRegisterFilterDriver

Return Value:

    NDIS_STATUS_SUCCESS

--*/
{
    DEBUGP(DL_TRACE, "===>FilterRegisterOptions\n");

    ASSERT(NdisFilterDriverHandle == g_FilterDriverHandle);
    ASSERT(FilterDriverContext == (NDIS_HANDLE)g_FilterDriverObject);

    if ((NdisFilterDriverHandle != (NDIS_HANDLE)g_FilterDriverHandle) ||
        (FilterDriverContext != (NDIS_HANDLE)g_FilterDriverObject))
    {
        return NDIS_STATUS_INVALID_PARAMETER;
    }

    DEBUGP(DL_TRACE, "<===FilterRegisterOptions\n");

    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
NDIS_STATUS
FilterAttach(
    NDIS_HANDLE                     NdisFilterHandle,
    NDIS_HANDLE                     FilterDriverContext,
    PNDIS_FILTER_ATTACH_PARAMETERS  AttachParameters
    )
/*++

Routine Description:

    Filter attach routine.
    Create filter's context, allocate NetBufferLists and NetBuffer pools and any
    other resources, and read configuration if needed.

Arguments:

    NdisFilterHandle - Specify a handle identifying this instance of the filter. FilterAttach
                       should save this handle. It is a required  parameter in subsequent calls
                       to NdisFxxx functions.
    FilterDriverContext - Filter driver context passed to NdisFRegisterFilterDriver.

    AttachParameters - attach parameters

Return Value:

    NDIS_STATUS_SUCCESS: FilterAttach successfully allocated and initialize data structures
                         for this filter instance.
    NDIS_STATUS_RESOURCES: FilterAttach failed due to insufficient resources.
    NDIS_STATUS_FAILURE: FilterAttach could not set up this instance of this filter and it has called
                         NdisWriteErrorLogEntry with parameters specifying the reason for failure.

N.B.:  FILTER can use NdisRegisterDeviceEx to create a device, so the upper 
    layer can send Irps to the filter.

--*/
{
    PLCXL_FILTER              pFilter = NULL;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    NDIS_FILTER_ATTRIBUTES  FilterAttributes;
    ULONG                   Size;
    BOOLEAN               bFalse = FALSE;
    //添加代码
    NET_BUFFER_LIST_POOL_PARAMETERS PoolParameters;
    //!添加代码!
    DEBUGP(DL_TRACE, "===>FilterAttach: NdisFilterHandle %p\n", NdisFilterHandle);

    do
    {
        ASSERT(FilterDriverContext == (NDIS_HANDLE)g_FilterDriverObject);
        if (FilterDriverContext != (NDIS_HANDLE)g_FilterDriverObject)
        {
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        // Verify the media type is supported.  This is a last resort; the
        // the filter should never have been bound to an unsupported miniport
        // to begin with.  If this driver is marked as a Mandatory filter (which
        // is the default for this sample; see the INF file), failing to attach 
        // here will leave the network adapter in an unusable state.
        //
        // Your setup/install code should not bind the filter to unsupported
        // media types.
        if ((AttachParameters->MiniportMediaType != NdisMedium802_3)
                && (AttachParameters->MiniportMediaType != NdisMediumWan)
                && (AttachParameters->MiniportMediaType != NdisMediumWirelessWan))
        {
           DEBUGP(DL_ERROR, "Unsupported media type.\n");

           Status = NDIS_STATUS_INVALID_PARAMETER;
           break;
        }

        Size = sizeof(LCXL_FILTER) +
               AttachParameters->FilterModuleGuidName->Length +
               AttachParameters->BaseMiniportInstanceName->Length +
               AttachParameters->BaseMiniportName->Length;

        pFilter = (PLCXL_FILTER)FILTER_ALLOC_MEM(NdisFilterHandle, Size);
        if (pFilter == NULL)
        {
            DEBUGP(DL_WARN, "Failed to allocate context structure.\n");
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pFilter, sizeof(LCXL_FILTER));
		//模块名称
        pFilter->FilterModuleName.Length = pFilter->FilterModuleName.MaximumLength = AttachParameters->FilterModuleGuidName->Length;
        pFilter->FilterModuleName.Buffer = (PWSTR)((PUCHAR)pFilter + sizeof(LCXL_FILTER));
        NdisMoveMemory(pFilter->FilterModuleName.Buffer,
                        AttachParameters->FilterModuleGuidName->Buffer,
                        pFilter->FilterModuleName.Length);



        pFilter->MiniportFriendlyName.Length = pFilter->MiniportFriendlyName.MaximumLength = AttachParameters->BaseMiniportInstanceName->Length;
        pFilter->MiniportFriendlyName.Buffer = (PWSTR)((PUCHAR)pFilter->FilterModuleName.Buffer + pFilter->FilterModuleName.Length);
        NdisMoveMemory(pFilter->MiniportFriendlyName.Buffer,
                        AttachParameters->BaseMiniportInstanceName->Buffer,
                        pFilter->MiniportFriendlyName.Length);


        pFilter->MiniportName.Length = pFilter->MiniportName.MaximumLength = AttachParameters->BaseMiniportName->Length;
        pFilter->MiniportName.Buffer = (PWSTR)((PUCHAR)pFilter->MiniportFriendlyName.Buffer +
                                                   pFilter->MiniportFriendlyName.Length);
        NdisMoveMemory(pFilter->MiniportName.Buffer,
                        AttachParameters->BaseMiniportName->Buffer,
                        pFilter->MiniportName.Length);
        
        pFilter->MiniportIfIndex = AttachParameters->BaseMiniportIfIndex;
        //添加代码
        //保存MAC地址（一个问题，当用户手动修改了MAC地址，会怎样- -）
        pFilter->mac_addr_len = AttachParameters->MacAddressLength;
        NdisMoveMemory(pFilter->cur_mac_addr, AttachParameters->CurrentMacAddress, pFilter->mac_addr_len);
		//初始化服务器列表
		InitializeListHead(&pFilter->server_list.list_entry);
		//初始化路由列表
		InitializeListHead(&pFilter->route_list.list_entry);
        //初始化NBL发送池
        PoolParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        PoolParameters.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        PoolParameters.Header.Size = sizeof(PoolParameters);
        PoolParameters.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT;
        //PoolParameters.ContextSize = sizeof(NPROT_SEND_NETBUFLIST_RSVD);
        PoolParameters.fAllocateNetBuffer = TRUE;
        PoolParameters.PoolTag = TAG_SEND_NBL;
        pFilter->SendNetBufferListPool = NdisAllocateNetBufferListPool( NdisFilterHandle, &PoolParameters); 

		//!添加代码!
        //
        // The filter should initialize TrackReceives and TrackSends properly. For this
        // driver, since its default characteristic has both a send and a receive handler,
        // these fields are initialized to TRUE.
        //
        pFilter->TrackReceives = TRUE;
        pFilter->TrackSends = TRUE;
        pFilter->FilterHandle = NdisFilterHandle;


        NdisZeroMemory(&FilterAttributes, sizeof(NDIS_FILTER_ATTRIBUTES));
        FilterAttributes.Header.Revision = NDIS_FILTER_ATTRIBUTES_REVISION_1;
        FilterAttributes.Header.Size = sizeof(NDIS_FILTER_ATTRIBUTES);
        FilterAttributes.Header.Type = NDIS_OBJECT_TYPE_FILTER_ATTRIBUTES;
        FilterAttributes.Flags = 0;

        NDIS_DECLARE_FILTER_MODULE_CONTEXT(LCXL_FILTER);
        Status = NdisFSetAttributes(NdisFilterHandle,
                                    pFilter,
                                    &FilterAttributes);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, "Failed to set attributes.\n");
            break;
        }


        pFilter->State = FilterPaused;

        FILTER_ACQUIRE_LOCK(&g_FilterListLock, bFalse);
        InsertHeadList(&g_FilterModuleList, &pFilter->FilterModuleLink);
        FILTER_RELEASE_LOCK(&g_FilterListLock, bFalse);

    }
    while (bFalse);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (pFilter != NULL)
        {
            FILTER_FREE_MEM(pFilter);
        }
    }

    DEBUGP(DL_TRACE, "<===FilterAttach:    Status %x\n", Status);
    return Status;
}

_Use_decl_annotations_
NDIS_STATUS
FilterPause(
    NDIS_HANDLE                     FilterModuleContext,
    PNDIS_FILTER_PAUSE_PARAMETERS   PauseParameters
    )
/*++

Routine Description:

    Filter pause routine.
    Complete all the outstanding sends and queued sends,
    wait for all the outstanding recvs to be returned
    and return all the queued receives.

Arguments:

    FilterModuleContext - pointer to the filter context stucture
    PauseParameters     - additional information about the pause

Return Value:

    NDIS_STATUS_SUCCESS if filter pauses successfully, NDIS_STATUS_PENDING
    if not.  No other return value is allowed (pause must succeed, eventually).

N.B.: When the filter is in Pausing state, it can still process OID requests, 
    complete sending, and returning packets to NDIS, and also indicate status.
    After this function completes, the filter must not attempt to send or 
    receive packets, but it may still process OID requests and status 
    indications.

--*/
{
    PLCXL_FILTER          pFilter = (PLCXL_FILTER)(FilterModuleContext);
    NDIS_STATUS         Status;
    BOOLEAN               bFalse = FALSE;

    UNREFERENCED_PARAMETER(PauseParameters);

    DEBUGP(DL_TRACE, "===>NDISLWF FilterPause: FilterInstance %p\n", FilterModuleContext);

    //
    // Set the flag that the filter is going to pause
    //
    FILTER_ASSERT(pFilter->State == FilterRunning);

    FILTER_ACQUIRE_LOCK(&pFilter->Lock, bFalse);
    pFilter->State = FilterPausing;
    FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);

    //
    // Do whatever work is required to bring the filter into the Paused state.
    //
    // If you have diverted and queued any send or receive NBLs, return them 
    // now.
    //
    // If you send or receive original NBLs, stop doing that and wait for your
    // NBLs to return to you now.
    //


    Status = NDIS_STATUS_SUCCESS;

    pFilter->State = FilterPaused;

    DEBUGP(DL_TRACE, "<===FilterPause:  Status %x\n", Status);
    return Status;
}

_Use_decl_annotations_
NDIS_STATUS
FilterRestart(
    NDIS_HANDLE                     FilterModuleContext,
    PNDIS_FILTER_RESTART_PARAMETERS RestartParameters
    )
/*++

Routine Description:

    Filter restart routine.
    Start the datapath - begin sending and receiving NBLs.

Arguments:

    FilterModuleContext - pointer to the filter context stucture.
    RestartParameters   - additional information about the restart operation.

Return Value:

    NDIS_STATUS_SUCCESS: if filter restarts successfully
    NDIS_STATUS_XXX: Otherwise.

--*/
{
    NDIS_STATUS     Status;
    PLCXL_FILTER      pFilter = (PLCXL_FILTER)FilterModuleContext;
    NDIS_HANDLE     ConfigurationHandle = NULL;


    PNDIS_RESTART_GENERAL_ATTRIBUTES NdisGeneralAttributes;
    PNDIS_RESTART_ATTRIBUTES         NdisRestartAttributes;
    NDIS_CONFIGURATION_OBJECT        ConfigObject;

    DEBUGP(DL_TRACE, "===>FilterRestart:   FilterModuleContext %p\n", FilterModuleContext);

    FILTER_ASSERT(pFilter->State == FilterPaused);

    ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
    ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
    ConfigObject.Header.Size = sizeof(NDIS_CONFIGURATION_OBJECT);
    ConfigObject.NdisHandle = g_FilterDriverHandle;
    ConfigObject.Flags = 0;

    Status = NdisOpenConfigurationEx(&ConfigObject, &ConfigurationHandle);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        //
        // Filter driver can choose to fail the restart if it cannot open the configuration
        //

#if 0
        //
        // The code is here just to demonstrate how to call NDIS to write an 
        // event to the eventlog.
        //
        PWCHAR              ErrorString = L"Ndislwf";

        DEBUGP(DL_WARN, "FilterRestart: Cannot open configuration.\n");
        NdisWriteEventLogEntry(g_FilterDriverObject,
                                EVENT_NDIS_DRIVER_FAILURE,
                                0,
                                1,
                                &ErrorString,
                                sizeof(Status),
                                &Status);
#endif

    }

    //
    // This sample doesn't actually do anything with the configuration handle;
    // it is opened here for illustrative purposes.  If you do not need to
    // read configuration, you may omit the code manipulating the 
    // ConfigurationHandle.
    //

    if (Status == NDIS_STATUS_SUCCESS)
    {
        NdisCloseConfiguration(ConfigurationHandle);
    }

    NdisRestartAttributes = RestartParameters->RestartAttributes;

    //
    // If NdisRestartAttributes is not NULL, then the filter can modify generic 
    // attributes and add new media specific info attributes at the end. 
    // Otherwise, if NdisRestartAttributes is NULL, the filter should not try to 
    // modify/add attributes.
    //
    if (NdisRestartAttributes != NULL)
    {
        PNDIS_RESTART_ATTRIBUTES   NextAttributes;

        ASSERT(NdisRestartAttributes->Oid == OID_GEN_MINIPORT_RESTART_ATTRIBUTES);

        NdisGeneralAttributes = (PNDIS_RESTART_GENERAL_ATTRIBUTES)NdisRestartAttributes->Data;

        //
        // Check to see if we need to change any attributes. For example, the
        // driver can change the current MAC address here. Or the driver can add
        // media specific info attributes.
        //
        NdisGeneralAttributes->LookaheadSize = 128;

        //
        // Check each attribute to see whether the filter needs to modify it.
        //
        NextAttributes = NdisRestartAttributes->Next;

        while (NextAttributes != NULL)
        {
            //
            // If somehow the filter needs to change a attributes which requires more space then
            // the current attributes:
            // 1. Remove the attribute from the Attributes list:
            //    TempAttributes = NextAttributes;
            //    NextAttributes = NextAttributes->Next;
            // 2. Free the memory for the current attributes: NdisFreeMemory(TempAttributes, 0 , 0);
            // 3. Dynamically allocate the memory for the new attributes by calling
            //    NdisAllocateMemoryWithTagPriority:
            //    NewAttributes = NdisAllocateMemoryWithTagPriority(Handle, size, Priority);
            // 4. Fill in the new attribute
            // 5. NewAttributes->Next = NextAttributes;
            // 6. NextAttributes = NewAttributes; // Just to make the next statement work.
            //
            NextAttributes = NextAttributes->Next;
        }

        //
        // Add a new attributes at the end
        // 1. Dynamically allocate the memory for the new attributes by calling
        //    NdisAllocateMemoryWithTagPriority.
        // 2. Fill in the new attribute
        // 3. NextAttributes->Next = NewAttributes;
        // 4. NewAttributes->Next = NULL;



    }

    //
    // If everything is OK, set the filter in running state.
    //
    pFilter->State = FilterRunning; // when successful


    Status = NDIS_STATUS_SUCCESS;


    //
    // Ensure the state is Paused if restart failed.
    //

    if (Status != NDIS_STATUS_SUCCESS)
    {
        pFilter->State = FilterPaused;
    }


    DEBUGP(DL_TRACE, "<===FilterRestart:  FilterModuleContext %p, Status %x\n", FilterModuleContext, Status);
    return Status;
}


_Use_decl_annotations_
VOID
FilterDetach(
    NDIS_HANDLE     FilterModuleContext
    )
/*++

Routine Description:

    Filter detach routine.
    This is a required function that will deallocate all the resources allocated during
    FilterAttach. NDIS calls FilterAttach to remove a filter instance from a filter stack.

Arguments:

    FilterModuleContext - pointer to the filter context area.

Return Value:
    None.

NOTE: Called at PASSIVE_LEVEL and the filter is in paused state

--*/
{
    PLCXL_FILTER                  pFilter = (PLCXL_FILTER)FilterModuleContext;
    BOOLEAN                      bFalse = FALSE;


    DEBUGP(DL_TRACE, "===>FilterDetach:    FilterInstance %p\n", FilterModuleContext);


    //
    // Filter must be in paused state
    //
    FILTER_ASSERT(pFilter->State == FilterPaused);


    //
    // Detach must not fail, so do not put any code here that can possibly fail.
    //

    //
    // Free filter instance name if allocated.
    //
    if (pFilter->FilterName.Buffer != NULL)
    {
        FILTER_FREE_MEM(pFilter->FilterName.Buffer);
    }

    //添加代码
    if (pFilter->SendNetBufferListPool!=NULL) {
        NdisFreeNetBufferListPool(pFilter->SendNetBufferListPool);
    }
    //!添加代码!

    FILTER_ACQUIRE_LOCK(&g_FilterListLock, bFalse);
    RemoveEntryList(&pFilter->FilterModuleLink);
    FILTER_RELEASE_LOCK(&g_FilterListLock, bFalse);


    //
    // Free the memory allocated
    FILTER_FREE_MEM(pFilter);

    DEBUGP(DL_TRACE, "<===FilterDetach Successfully\n");
    return;
}

_Use_decl_annotations_
VOID
FilterUnload(
    PDRIVER_OBJECT      DriverObject
    )
/*++

Routine Description:

    Filter driver's unload routine.
    Deregister the driver from NDIS.

Arguments:

    DriverObject - pointer to the system's driver object structure
                   for this driver

Return Value:

    NONE

--*/
{
#if DBG
    BOOLEAN               bFalse = FALSE;
#endif

    UNREFERENCED_PARAMETER(DriverObject);

    DEBUGP(DL_TRACE, "===>FilterUnload\n");

    //
    // Should free the filter context list
    //
    FilterDeregisterDevice();
    NdisFDeregisterFilterDriver(g_FilterDriverHandle);

#if DBG
    FILTER_ACQUIRE_LOCK(&g_FilterListLock, bFalse);
    ASSERT(IsListEmpty(&g_FilterModuleList));

    FILTER_RELEASE_LOCK(&g_FilterListLock, bFalse);

#endif

    FILTER_FREE_LOCK(&g_FilterListLock);

    DEBUGP(DL_TRACE, "<===FilterUnload\n");
    //添加代码
    ExDeleteNPagedLookasideList(&g_route_mem_mgr);
    ExDeleteNPagedLookasideList(&g_server_mem_mgr);
    //!添加代码!
    return;

}

_Use_decl_annotations_
NDIS_STATUS
FilterOidRequest(
    NDIS_HANDLE         FilterModuleContext,
    PNDIS_OID_REQUEST   Request
    )
/*++

Routine Description:

    Request handler
    Handle requests from upper layers

Arguments:

    FilterModuleContext   - our filter
    Request               - the request passed down


Return Value:

     NDIS_STATUS_SUCCESS
     NDIS_STATUS_PENDING
     NDIS_STATUS_XXX

NOTE: Called at <= DISPATCH_LEVEL  (unlike a miniport's MiniportOidRequest)

--*/
{
    PLCXL_FILTER              pFilter = (PLCXL_FILTER)FilterModuleContext;
    NDIS_STATUS             Status;
    PNDIS_OID_REQUEST       ClonedRequest=NULL;
    BOOLEAN                 bSubmitted = FALSE;
    PFILTER_REQUEST_CONTEXT Context;
    BOOLEAN                 bFalse = FALSE;


    DEBUGP(DL_TRACE, "===>FilterOidRequest: Request %p.\n", Request);

    //
    // Most of the time, a filter will clone the OID request and pass down
    // the clone.  When the clone completes, the filter completes the original
    // OID request.
    //
    // If your filter needs to modify a specific request, it can modify the
    // request before or after sending down the cloned request.  Or, it can
    // complete the original request on its own without sending down any
    // clone at all.
    //
    // If your filter driver does not need to modify any OID requests, then
    // you may simply omit this routine entirely; NDIS will pass OID requests
    // down on your behalf.  This is more efficient than implementing a 
    // routine that does nothing but clone all requests, as in the sample here.
    //

    do
    {
        Status = NdisAllocateCloneOidRequest(pFilter->FilterHandle,
                                            Request,
                                            FILTER_TAG,
                                            &ClonedRequest);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, "FilerOidRequest: Cannot Clone Request\n");
            break;
        }

        Context = (PFILTER_REQUEST_CONTEXT)(&ClonedRequest->SourceReserved[0]);
        *Context = Request;

        bSubmitted = TRUE;

        //
        // Use same request ID
        //
        ClonedRequest->RequestId = Request->RequestId;

        pFilter->PendingOidRequest = ClonedRequest;


        Status = NdisFOidRequest(pFilter->FilterHandle, ClonedRequest);

        if (Status != NDIS_STATUS_PENDING)
        {


            FilterOidRequestComplete(pFilter, ClonedRequest, Status);
            Status = NDIS_STATUS_PENDING;
        }


    }while (bFalse);

    if (bSubmitted == FALSE)
    {
        switch(Request->RequestType)
        {
            case NdisRequestMethod:
                Request->DATA.METHOD_INFORMATION.BytesRead = 0;
                Request->DATA.METHOD_INFORMATION.BytesNeeded = 0;
                Request->DATA.METHOD_INFORMATION.BytesWritten = 0;
                break;

            case NdisRequestSetInformation:
                Request->DATA.SET_INFORMATION.BytesRead = 0;
                Request->DATA.SET_INFORMATION.BytesNeeded = 0;
                break;

            case NdisRequestQueryInformation:
            case NdisRequestQueryStatistics:
            default:
                Request->DATA.QUERY_INFORMATION.BytesWritten = 0;
                Request->DATA.QUERY_INFORMATION.BytesNeeded = 0;
                break;
        }

    }
    DEBUGP(DL_TRACE, "<===FilterOidRequest: Status %8x.\n", Status);

    return Status;

}

_Use_decl_annotations_
VOID
FilterCancelOidRequest(
    NDIS_HANDLE             FilterModuleContext,
    PVOID                   RequestId
    )
/*++

Routine Description:

    Cancels an OID request

    If your filter driver does not intercept and hold onto any OID requests,
    then you do not need to implement this routine.  You may simply omit it.
    Furthermore, if the filter only holds onto OID requests so it can pass
    down a clone (the most common case) the filter does not need to implement 
    this routine; NDIS will then automatically request that the lower-level 
    filter/miniport cancel your cloned OID.

    Most filters do not need to implement this routine.

Arguments:

    FilterModuleContext   - our filter
    RequestId             - identifies the request(s) to cancel

--*/
{
    PLCXL_FILTER                          pFilter = (PLCXL_FILTER)FilterModuleContext;
    PNDIS_OID_REQUEST                   Request = NULL;
    PFILTER_REQUEST_CONTEXT             Context;
    PNDIS_OID_REQUEST                   OriginalRequest = NULL;
    BOOLEAN                             bFalse = FALSE;

    FILTER_ACQUIRE_LOCK(&pFilter->Lock, bFalse);

    Request = pFilter->PendingOidRequest;

    if (Request != NULL)
    {
        Context = (PFILTER_REQUEST_CONTEXT)(&Request->SourceReserved[0]);

        OriginalRequest = (*Context);
    }

    if ((OriginalRequest != NULL) && (OriginalRequest->RequestId == RequestId))
    {
        FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);

        NdisFCancelOidRequest(pFilter->FilterHandle, RequestId);
    }
    else
    {
        FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);
    }


}

_Use_decl_annotations_
VOID
FilterOidRequestComplete(
    NDIS_HANDLE         FilterModuleContext,
    PNDIS_OID_REQUEST   Request,
    NDIS_STATUS         Status
    )
/*++

Routine Description:

    Notification that an OID request has been completed

    If this filter sends a request down to a lower layer, and the request is
    pended, the FilterOidRequestComplete routine is invoked when the request
    is complete.  Most requests we've sent are simply clones of requests
    received from a higher layer; all we need to do is complete the original
    higher request.

    However, if this filter driver sends original requests down, it must not
    attempt to complete a pending request to the higher layer.

Arguments:

    FilterModuleContext   - our filter context area
    NdisRequest           - the completed request
    Status                - completion status

--*/
{
    PLCXL_FILTER                          pFilter = (PLCXL_FILTER)FilterModuleContext;
    PNDIS_OID_REQUEST                   OriginalRequest;
    PFILTER_REQUEST_CONTEXT             Context;
    BOOLEAN                             bFalse = FALSE;

    DEBUGP(DL_TRACE, "===>FilterOidRequestComplete, Request %p.\n", Request);

    Context = (PFILTER_REQUEST_CONTEXT)(&Request->SourceReserved[0]);
    OriginalRequest = (*Context);

    //
    // This is an internal request
    //
    if (OriginalRequest == NULL)
    {
        filterInternalRequestComplete(pFilter, Request, Status);
        return;
    }


    FILTER_ACQUIRE_LOCK(&pFilter->Lock, bFalse);

    ASSERT(pFilter->PendingOidRequest == Request);
    pFilter->PendingOidRequest = NULL;

    FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);


    //
    // Copy the information from the returned request to the original request
    //
    switch(Request->RequestType)
    {
        case NdisRequestMethod:
            OriginalRequest->DATA.METHOD_INFORMATION.OutputBufferLength =  Request->DATA.METHOD_INFORMATION.OutputBufferLength;
            OriginalRequest->DATA.METHOD_INFORMATION.BytesRead = Request->DATA.METHOD_INFORMATION.BytesRead;
            OriginalRequest->DATA.METHOD_INFORMATION.BytesNeeded = Request->DATA.METHOD_INFORMATION.BytesNeeded;
            OriginalRequest->DATA.METHOD_INFORMATION.BytesWritten = Request->DATA.METHOD_INFORMATION.BytesWritten;
            break;

        case NdisRequestSetInformation:
            OriginalRequest->DATA.SET_INFORMATION.BytesRead = Request->DATA.SET_INFORMATION.BytesRead;
            OriginalRequest->DATA.SET_INFORMATION.BytesNeeded = Request->DATA.SET_INFORMATION.BytesNeeded;
            break;

        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
        default:
            OriginalRequest->DATA.QUERY_INFORMATION.BytesWritten = Request->DATA.QUERY_INFORMATION.BytesWritten;
            OriginalRequest->DATA.QUERY_INFORMATION.BytesNeeded = Request->DATA.QUERY_INFORMATION.BytesNeeded;
            break;
    }


    (*Context) = NULL;

    NdisFreeCloneOidRequest(pFilter->FilterHandle, Request);

    NdisFOidRequestComplete(pFilter->FilterHandle, OriginalRequest, Status);

    DEBUGP(DL_TRACE, "<===FilterOidRequestComplete.\n");
}


_Use_decl_annotations_
VOID
FilterStatus(
    NDIS_HANDLE             FilterModuleContext,
    PNDIS_STATUS_INDICATION StatusIndication
    )
/*++

Routine Description:

    Status indication handler

Arguments:

    FilterModuleContext     - our filter context
    StatusIndication        - the status being indicated

NOTE: called at <= DISPATCH_LEVEL

  FILTER driver may call NdisFIndicateStatus to generate a status indication to 
  all higher layer modules.

--*/
{
    PLCXL_FILTER              pFilter = (PLCXL_FILTER)FilterModuleContext;
#if DBG
    BOOLEAN                  bFalse = FALSE;
#endif

    DEBUGP(DL_TRACE, "===>FilterStatus, IndicateStatus = %8x.\n", StatusIndication->StatusCode);


    //
    // The filter may do processing on the status indication here, including
    // intercepting and dropping it entirely.  However, the sample does nothing
    // with status indications except pass them up to the higher layer.  It is 
    // more efficient to omit the FilterStatus handler entirely if it does 
    // nothing, but it is included in this sample for illustrative purposes.
    //

#if DBG
    FILTER_ACQUIRE_LOCK(&pFilter->Lock, bFalse);
    ASSERT(pFilter->bIndicating == FALSE);
    pFilter->bIndicating = TRUE;
    FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);
#endif // DBG

    NdisFIndicateStatus(pFilter->FilterHandle, StatusIndication);

#if DBG
    FILTER_ACQUIRE_LOCK(&pFilter->Lock, bFalse);
    ASSERT(pFilter->bIndicating == TRUE);
    pFilter->bIndicating = FALSE;
    FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);
#endif // DBG

    DEBUGP(DL_TRACE, "<===FilterStatus.\n");

}

_Use_decl_annotations_
VOID
FilterDevicePnPEventNotify(
    NDIS_HANDLE             FilterModuleContext,
    PNET_DEVICE_PNP_EVENT   NetDevicePnPEvent
    )
/*++

Routine Description:

    Device PNP event handler

Arguments:

    FilterModuleContext         - our filter context
    NetDevicePnPEvent           - a Device PnP event

NOTE: called at PASSIVE_LEVEL

--*/
{
    PLCXL_FILTER             pFilter = (PLCXL_FILTER)FilterModuleContext;
    NDIS_DEVICE_PNP_EVENT  DevicePnPEvent = NetDevicePnPEvent->DevicePnPEvent;
#if DBG
    BOOLEAN                bFalse = FALSE;
#endif

    DEBUGP(DL_TRACE, "===>FilterDevicePnPEventNotify: NetPnPEvent = %p.\n", NetDevicePnPEvent);

    //
    // The filter may do processing on the event here, including intercepting
    // and dropping it entirely.  However, the sample does nothing with Device
    // PNP events, except pass them down to the next lower* layer.  It is more
    // efficient to omit the FilterDevicePnPEventNotify handler entirely if it
    // does nothing, but it is included in this sample for illustrative purposes.
    //
    // * Trivia: Device PNP events percolate DOWN the stack, instead of upwards
    // like status indications and Net PNP events.  So the next layer is the
    // LOWER layer.
    //

    switch (DevicePnPEvent)
    {

        case NdisDevicePnPEventQueryRemoved:
        case NdisDevicePnPEventRemoved:
        case NdisDevicePnPEventSurpriseRemoved:
        case NdisDevicePnPEventQueryStopped:
        case NdisDevicePnPEventStopped:
        case NdisDevicePnPEventPowerProfileChanged:
        case NdisDevicePnPEventFilterListChanged:

            break;

        default:
            DEBUGP(DL_ERROR, "FilterDevicePnPEventNotify: Invalid event.\n");
            FILTER_ASSERT(bFalse);

            break;
    }

    NdisFDevicePnPEventNotify(pFilter->FilterHandle, NetDevicePnPEvent);

    DEBUGP(DL_TRACE, "<===FilterDevicePnPEventNotify\n");

}

_Use_decl_annotations_
NDIS_STATUS
FilterNetPnPEvent(
    NDIS_HANDLE              FilterModuleContext,
    PNET_PNP_EVENT_NOTIFICATION NetPnPEventNotification
    )
/*++

Routine Description:

    Net PNP event handler

Arguments:

    FilterModuleContext         - our filter context
    NetPnPEventNotification     - a Net PnP event

NOTE: called at PASSIVE_LEVEL

--*/
{
    PLCXL_FILTER                pFilter = (PLCXL_FILTER)FilterModuleContext;
    NDIS_STATUS               Status = NDIS_STATUS_SUCCESS;

    //
    // The filter may do processing on the event here, including intercepting 
    // and dropping it entirely.  However, the sample does nothing with Net PNP
    // events, except pass them up to the next higher layer.  It is more
    // efficient to omit the FilterNetPnPEvent handler entirely if it does
    // nothing, but it is included in this sample for illustrative purposes.
    //

    Status = NdisFNetPnPEvent(pFilter->FilterHandle, NetPnPEventNotification);

    return Status;
}

_Use_decl_annotations_
VOID
FilterSendNetBufferListsComplete(
    NDIS_HANDLE         FilterModuleContext,
    PNET_BUFFER_LIST    NetBufferLists,
    ULONG               SendCompleteFlags
    )
/*++

Routine Description:

    Send complete handler

    This routine is invoked whenever the lower layer is finished processing 
    sent NET_BUFFER_LISTs.  If the filter does not need to be involved in the
    send path, you should remove this routine and the FilterSendNetBufferLists
    routine.  NDIS will pass along send packets on behalf of your filter more 
    efficiently than the filter can.

Arguments:

    FilterModuleContext     - our filter context
    NetBufferLists          - a chain of NBLs that are being returned to you
    SendCompleteFlags       - flags (see documentation)

Return Value:

     NONE

--*/
{
    PLCXL_FILTER         pFilter = (PLCXL_FILTER)FilterModuleContext;
    ULONG              NumOfSendCompletes = 0;
    BOOLEAN            DispatchLevel;
    PNET_BUFFER_LIST   CurrNbl;
    //添加代码
    //前一个NBL
    PNET_BUFFER_LIST   PrepNbl;
    //!添加代码!
    DEBUGP(DL_TRACE, "===>SendNBLComplete, NetBufferList: %p.\n", NetBufferLists);


    //
    // If your filter injected any send packets into the datapath to be sent,
    // you must identify their NBLs here and remove them from the chain.  Do not
    // attempt to send-complete your NBLs up to the higher layer.
    //

    //
    // If your filter has modified any NBLs (or NBs, MDLs, etc) in your
    // FilterSendNetBufferLists handler, you must undo the modifications here.
    // In general, NBLs must be returned in the same condition in which you had
    // you received them.  (Exceptions: the NBLs can be re-ordered on the linked
    // list, and the scratch fields are don't-care).
    //

    if (pFilter->TrackSends)
    {
        CurrNbl = NetBufferLists;
        while (CurrNbl)
        {
            NumOfSendCompletes++;
            CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);

        }
        DispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendCompleteFlags);
        FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);
        pFilter->OutstandingSends -= NumOfSendCompletes;
        FILTER_LOG_SEND_REF(2, pFilter, PrevNbl, pFilter->OutstandingSends);
        FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
    }
    //添加代码
    //Note  A filter driver should keep track of send requests that 
    //it originates and make sure that it does NOT call the NdisFSendNetBufferListsComplete function 
    //when such requests are complete.
    CurrNbl = NetBufferLists;
    PrepNbl = NULL;
    while (CurrNbl != NULL) {
        //如果是本驱动发出来的NBL，将此NBL脱离当前的NBL链
        if (CurrNbl->SourceHandle == pFilter->FilterHandle) {
            PNET_BUFFER_LIST pOwnerNBL;

            pOwnerNBL = CurrNbl;
            //判断是否是链表头
            if (CurrNbl != NetBufferLists) {
                //不是链表头
                CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
                ASSERT(PrepNbl!=NULL);
                NET_BUFFER_LIST_NEXT_NBL(PrepNbl) = CurrNbl;
            } else {
                //是链表头
                NetBufferLists = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
            }
            //断开和原始链的关联
            NET_BUFFER_LIST_NEXT_NBL(pOwnerNBL) = NULL;
            //释放NBL
            NdisFreeNetBufferList(pOwnerNBL);
        } else {
            PrepNbl = CurrNbl;
            CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
        }
    }
    //!添加代码!

    // Send complete the NBLs.  If you removed any NBLs from the chain, make
    // sure the chain isn't empty (i.e., NetBufferLists!=NULL).
	if (NetBufferLists!=NULL) {
		NdisFSendNetBufferListsComplete(pFilter->FilterHandle, NetBufferLists, SendCompleteFlags);
	}
    DEBUGP(DL_TRACE, "<===SendNBLComplete.\n");
}


_Use_decl_annotations_
VOID
FilterSendNetBufferLists(
    NDIS_HANDLE         FilterModuleContext,
    PNET_BUFFER_LIST    NetBufferLists,
    NDIS_PORT_NUMBER    PortNumber,
    ULONG               SendFlags
    )
/*++

Routine Description:

    Send Net Buffer List handler
    This function is an optional function for filter drivers. If provided, NDIS
    will call this function to transmit a linked list of NetBuffers, described by a
    NetBufferList, over the network. If this handler is NULL, NDIS will skip calling
    this filter when sending a NetBufferList and will call the next lower 
    driver in the stack.  A filter that doesn't provide a FilerSendNetBufferList
    handler can not originate a send on its own.

Arguments:

    FilterModuleContext     - our filter context area
    NetBufferLists          - a List of NetBufferLists to send
    PortNumber              - Port Number to which this send is targeted
    SendFlags               - specifies if the call is at DISPATCH_LEVEL

--*/
{
    PLCXL_FILTER          pFilter = (PLCXL_FILTER)FilterModuleContext;
    PNET_BUFFER_LIST    CurrNbl;
    BOOLEAN             DispatchLevel;
    BOOLEAN             bFalse = FALSE;

    DEBUGP(DL_TRACE, "===>SendNetBufferList: NBL = %p.\n", NetBufferLists);

    do
    {

       DispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags);
#if DBG
        //
        // we should never get packets to send if we are not in running state
        //

        FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);
        //
        // If the filter is not in running state, fail the send
        //
        if (pFilter->State != FilterRunning)
        {
            FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);

            CurrNbl = NetBufferLists;
            while (CurrNbl)
            {
                NET_BUFFER_LIST_STATUS(CurrNbl) = NDIS_STATUS_PAUSED;
                CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
            }
            NdisFSendNetBufferListsComplete(pFilter->FilterHandle,
                        NetBufferLists,
                        DispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0);
            break;

        }
        FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
#endif
        if (pFilter->TrackSends)
        {
            FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);
            CurrNbl = NetBufferLists;
            while (CurrNbl)
            {
                pFilter->OutstandingSends++;
                FILTER_LOG_SEND_REF(1, pFilter, CurrNbl, pFilter->OutstandingSends);

                CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
            }
            FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
        }
        
        //
        // If necessary, queue the NetBufferLists in a local structure for later
        // processing.  However, do not queue them for "too long", or else the
        // system's performance may be degraded.  If you need to hold onto an
        // NBL for an unbounded amount of time, then allocate memory, perform a
        // deep copy, and complete the original NBL.
        //
        
        NdisFSendNetBufferLists(pFilter->FilterHandle, NetBufferLists, PortNumber, SendFlags);


    }
    while (bFalse);

    DEBUGP(DL_TRACE, "<===SendNetBufferList. \n");
}

_Use_decl_annotations_
VOID
FilterReturnNetBufferLists(
    NDIS_HANDLE         FilterModuleContext,
    PNET_BUFFER_LIST    NetBufferLists,
    ULONG               ReturnFlags
    )
/*++

Routine Description:

    FilterReturnNetBufferLists handler.
    FilterReturnNetBufferLists is an optional function. If provided, NDIS calls
    FilterReturnNetBufferLists to return the ownership of one or more NetBufferLists
    and their embedded NetBuffers to the filter driver. If this handler is NULL, NDIS
    will skip calling this filter when returning NetBufferLists to the underlying
    miniport and will call the next lower driver in the stack. A filter that doesn't
    provide a FilterReturnNetBufferLists handler cannot originate a receive indication
    on its own.

Arguments:

    FilterInstanceContext       - our filter context area
    NetBufferLists              - a linked list of NetBufferLists that this 
                                  filter driver indicated in a previous call to 
                                  NdisFIndicateReceiveNetBufferLists
    ReturnFlags                 - flags specifying if the caller is at DISPATCH_LEVEL

--*/
{
    PLCXL_FILTER          pFilter = (PLCXL_FILTER)FilterModuleContext;
    PNET_BUFFER_LIST    CurrNbl = NetBufferLists;
    UINT                NumOfNetBufferLists = 0;
    BOOLEAN             DispatchLevel;
    ULONG               Ref;

    DEBUGP(DL_TRACE, "===>ReturnNetBufferLists, NetBufferLists is %p.\n", NetBufferLists);


    //
    // If your filter injected any receive packets into the datapath to be
    // received, you must identify their NBLs here and remove them from the 
    // chain.  Do not attempt to receive-return your NBLs down to the lower
    // layer.
    //

    //
    // If your filter has modified any NBLs (or NBs, MDLs, etc) in your
    // FilterReceiveNetBufferLists handler, you must undo the modifications here.
    // In general, NBLs must be returned in the same condition in which you had
    // you received them.  (Exceptions: the NBLs can be re-ordered on the linked
    // list, and the scratch fields are don't-care).
    //

    if (pFilter->TrackReceives)
    {
        while (CurrNbl)
        {
            NumOfNetBufferLists ++;
            CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
        }
    }

    
    // Return the received NBLs.  If you removed any NBLs from the chain, make
    // sure the chain isn't empty (i.e., NetBufferLists!=NULL).

    NdisFReturnNetBufferLists(pFilter->FilterHandle, NetBufferLists, ReturnFlags);

    if (pFilter->TrackReceives)
    {
        DispatchLevel = NDIS_TEST_RETURN_AT_DISPATCH_LEVEL(ReturnFlags);
        FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);

        pFilter->OutstandingRcvs -= NumOfNetBufferLists;
        Ref = pFilter->OutstandingRcvs;
        FILTER_LOG_RCV_REF(3, pFilter, NetBufferLists, Ref);
        FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
    }


    DEBUGP(DL_TRACE, "<===ReturnNetBufferLists.\n");


}


_Use_decl_annotations_
VOID
FilterReceiveNetBufferLists(
    NDIS_HANDLE         FilterModuleContext,
    PNET_BUFFER_LIST    NetBufferLists,
    NDIS_PORT_NUMBER    PortNumber,
    ULONG               NumberOfNetBufferLists,
    ULONG               ReceiveFlags
    )
/*++

Routine Description:

    FilerReceiveNetBufferLists is an optional function for filter drivers.
    If provided, this function processes receive indications made by underlying
    NIC or lower level filter drivers. This function  can also be called as a
    result of loopback. If this handler is NULL, NDIS will skip calling this
    filter when processing a receive indication and will call the next higher
    driver in the stack. A filter that doesn't provide a
    FilterReceiveNetBufferLists handler cannot provide a
    FilterReturnNetBufferLists handler and cannot a initiate an original receive 
    indication on its own.

Arguments:

    FilterModuleContext      - our filter context area.
    NetBufferLists           - a linked list of NetBufferLists
    PortNumber               - Port on which the receive is indicated
    ReceiveFlags             -

N.B.: It is important to check the ReceiveFlags in NDIS_TEST_RECEIVE_CANNOT_PEND.
    This controls whether the receive indication is an synchronous or 
    asynchronous function call.

--*/
{

    PLCXL_FILTER          pFilter = (PLCXL_FILTER)FilterModuleContext;
    BOOLEAN             DispatchLevel;
    ULONG               Ref;
    BOOLEAN             bFalse = FALSE;
    //修改代码，去掉DBG判定
//#if DBG
    ULONG               ReturnFlags = 0;
//#endif
    //!修改代码!
	PNET_BUFFER_LIST    pNextNBL;
    //添加代码

    ULONG               SendFlags = 0;

    //可以通过的NBL
    PNET_BUFFER_LIST    pPassHeadNBL = NULL;
    //末尾的NBL
    PNET_BUFFER_LIST    pPassLastNBL = NULL;
    ULONG               NumberOfPassNBL = 0;
    //需要丢掉的NBL
    PNET_BUFFER_LIST    pDropHeadNBL = NULL;
    PNET_BUFFER_LIST    pDropLastNBL = NULL;
    //要转发的NBL列表
    PNET_BUFFER_LIST    pSendNBLs = NULL;
    //!添加代码!


    DEBUGP(DL_TRACE, "===>ReceiveNetBufferList: NetBufferLists = %p.\n", NetBufferLists);

    //添加代码
    if (NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags)) {
        NDIS_SET_RETURN_FLAG(ReturnFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
        NDIS_SET_SEND_FLAG(SendFlags, NDIS_SEND_FLAGS_DISPATCH_LEVEL);
    }
    //!添加代码!

    do
    {

        DispatchLevel = NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags);
#if DBG
        FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);

        if (pFilter->State != FilterRunning)
        {
            FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);

            if (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags))
            {
                //移除代码
                /*
                ReturnFlags = 0;
                if (NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags))
                {
                    NDIS_SET_RETURN_FLAG(ReturnFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
                }
                */
                //!移除代码!
                NdisFReturnNetBufferLists(pFilter->FilterHandle, NetBufferLists, ReturnFlags);
            }
            break;
        }
        FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
#endif

        ASSERT(NumberOfNetBufferLists >= 1);

        //
        // If you would like to drop a received packet, then you must carefully
        // modify the NBL chain as follows:
        //
        //     if NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags):
        //         For each NBL that is NOT dropped, temporarily unlink it from
        //         the linked list, and indicate it up alone with 
        //         NdisFIndicateReceiveNetBufferLists and the
        //         NDIS_RECEIVE_FLAGS_RESOURCES flag set.  Then immediately
        //         relink the NBL back into the chain.  When all NBLs have been
        //         indicated up, you may return from this function.
        //     otherwise (NDIS_TEST_RECEIVE_CANNOT_PEND is FALSE):
        //         Divide the linked list of NBLs into two chains: one chain
        //         of packets to drop, and everything else in another chain.
        //         Return the first chain with NdisFReturnNetBufferLists, and
        //         indicate up the rest with NdisFIndicateReceiveNetBufferLists.
        //
        // Note: on the receive path for Ethernet packets, one NBL will have 
        // exactly one NB.  So (assuming you are receiving on Ethernet, or are 
        // attached above Native WiFi) you do not need to worry about dropping
        // one NB, but trying to indicate up the remaining NBs on the same NBL.
        // In other words, if the first NB should be dropped, drop the whole NBL.
        //

        //
        // If you would like to modify a packet, and can do so quickly, you may
        // do it here.  However, make sure you save enough information to undo
        // your modification in the FilterReturnNetBufferLists handler.
        //

        //
        // If necessary, queue the NetBufferLists in a local structure for later
        // processing.  However, do not queue them for "too long", or else the
        // system's performance may be degraded.  If you need to hold onto an
        // NBL for an unbounded amount of time, then allocate memory, perform a
        // deep copy, and return the original NBL.
        //

		//添加代码
		pNextNBL = NetBufferLists;
		while (pNextNBL != NULL) {
			PMDL                pMdl = NULL;
			UINT                BufferLength;
            ULONG               TotalLength;
            ULONG               Offset;
            PETHERNET_HEADER    pEthHeader = NULL;

			pMdl = NET_BUFFER_CURRENT_MDL(NET_BUFFER_LIST_FIRST_NB(pNextNBL));
			TotalLength = NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB(pNextNBL));
			Offset = NET_BUFFER_CURRENT_MDL_OFFSET(NET_BUFFER_LIST_FIRST_NB(pNextNBL));
            
            ASSERT(pMdl != NULL);
            NdisQueryMdl(
                    pMdl,
                    &pEthHeader,
                    &BufferLength,
                    NormalPagePriority);
			//各种有效性判断
            if (pEthHeader != NULL && BufferLength != 0) {
                PLCXL_ROUTE_LIST_ENTRY pRouteListEntry = NULL;

                ASSERT(BufferLength > Offset);
                //获取真正的的包数据
                BufferLength -= Offset;
                //获取帧数据头
                pEthHeader = (PETHERNET_HEADER)((PUCHAR)pEthHeader + Offset);
                
                pRouteListEntry = IfRouteNBL(pFilter, pEthHeader, BufferLength);
                //如果需要路由的话
                if (pRouteListEntry!=NULL) {
                    PETHERNET_HEADER    pSendBuffer;
                    PMDL                pMdl = NULL;
                    PNET_BUFFER_LIST    tmpNBL;
                    ULONG               BytesCopied;
                    //UCHAR               OrgDstAddr[NDIS_MAC_ADDR_LEN];
                    //是否可以Pend
                    if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags)) {
                        //到最后会一起丢弃数据包
                        
                    } else {
                        //要丢弃的数据包
                        if (pDropHeadNBL == NULL) {
                            pDropHeadNBL = pNextNBL;
                            pDropLastNBL = pNextNBL;
                        } else {
                            NET_BUFFER_LIST_NEXT_NBL(pDropLastNBL) = pNextNBL;
                        }
                    }

                    pSendBuffer = (PETHERNET_HEADER)FILTER_ALLOC_MEM(pFilter->FilterHandle, BufferLength);
                    pMdl = NdisAllocateMdl(pFilter->FilterHandle, pSendBuffer, BufferLength);
                    tmpNBL = NdisAllocateNetBufferAndNetBufferList(
                        pFilter->SendNetBufferListPool,
                        0,                              // ContextSize
                        0,                              // ContextBackfill
                        pMdl,                           // MdlChain
                        0,                              // DataOffset
                        BufferLength);                   // DataLength
                    NdisCopyFromNetBufferToNetBuffer(
                        NET_BUFFER_LIST_FIRST_NB(tmpNBL),
                        0,
                        BufferLength,
                        NET_BUFFER_LIST_FIRST_NB(pNextNBL),
                        0, 
                        &BytesCopied);
                    //设置SourceHandle
                    //A filter driver must set the SourceHandle member of each NET_BUFFER_LIST structure that it originates to the same value that it passes to the NdisFilterHandle parameter

                    tmpNBL->SourceHandle = pFilter->FilterHandle;
                    //修改目标MAC地址
                    RtlCopyMemory(&pSendBuffer->Destination, pRouteListEntry->dst_server->cur_mac_addr, sizeof(pEthHeader->Destination));
                    //
                    // The other members of NET_BUFFER_DATA structure are already initialized properly during allocation.
                    //
                    NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB(tmpNBL)) = BytesCopied;
                    //插入到转发队列中
                    if (pSendNBLs==NULL) {
                        pSendNBLs = tmpNBL;
                    } else {
                        NET_BUFFER_LIST_NEXT_NBL(pSendNBLs) = tmpNBL;
                    }
                } else {
                    if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags)) {
                        PNET_BUFFER_LIST tmpNBL;

                        tmpNBL = NET_BUFFER_LIST_NEXT_NBL(pNextNBL);
                        NET_BUFFER_LIST_NEXT_NBL(pNextNBL) = NULL;
                        //接受不丢弃的数据包
                        NdisFIndicateReceiveNetBufferLists(pFilter->FilterHandle, pNextNBL, PortNumber, 1, ReceiveFlags | NDIS_RECEIVE_FLAGS_RESOURCES);
                        NET_BUFFER_LIST_NEXT_NBL(pNextNBL) = tmpNBL;
                    } else {
                        if (pPassHeadNBL == NULL) {
                            pPassHeadNBL = pNextNBL;
                            pPassLastNBL = pNextNBL;
                        } else {
                            NET_BUFFER_LIST_NEXT_NBL(pPassLastNBL) = pNextNBL;
                        }
                        NumberOfPassNBL++;
                    }
                }
                
            } else {
                //缺代码
            }

			//获取下一个NBL
			pNextNBL = NET_BUFFER_LIST_NEXT_NBL(pNextNBL);
		}
        if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags)) {
            //不接受数据包
            NdisFReturnNetBufferLists(pFilter->FilterHandle, NetBufferLists, ReturnFlags);
        } else {
            ASSERT(pPassLastNBL!=NULL);
            ASSERT(pDropLastNBL!=NULL);
            //将两个链表的最后一项的Next域清空
            NET_BUFFER_LIST_NEXT_NBL(pPassLastNBL) = NULL;
            NET_BUFFER_LIST_NEXT_NBL(pDropLastNBL) = NULL;
            //接受PassHeadNBL
            NdisFIndicateReceiveNetBufferLists(pFilter->FilterHandle, pPassHeadNBL, PortNumber, NumberOfPassNBL, ReceiveFlags);
            //丢弃DropHeadNBL
            NdisFReturnNetBufferLists(pFilter->FilterHandle, pDropHeadNBL, ReturnFlags);
        }
        //转发数据包给真实的服务器
        if (NULL!=pSendNBLs) {
            NdisFSendNetBufferLists(pFilter->FilterHandle, pSendNBLs, PortNumber, SendFlags);
        }
		//
        if (pFilter->TrackReceives)
        {
            FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);
            pFilter->OutstandingRcvs += NumberOfNetBufferLists;
            Ref = pFilter->OutstandingRcvs;

            FILTER_LOG_RCV_REF(1, pFilter, NetBufferLists, Ref);
            FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
        }
        //移除代码
        /*
        NdisFIndicateReceiveNetBufferLists(
                   pFilter->FilterHandle,
                   NetBufferLists,
                   PortNumber,
                   NumberOfNetBufferLists,
                   ReceiveFlags);

        */
        //!移除代码!
        if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags) &&
            pFilter->TrackReceives)
        {
            FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);
            pFilter->OutstandingRcvs -= NumberOfNetBufferLists;
            Ref = pFilter->OutstandingRcvs;
            FILTER_LOG_RCV_REF(2, pFilter, NetBufferLists, Ref);
            FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
        }

    } while (bFalse);

    DEBUGP(DL_TRACE, "<===ReceiveNetBufferList: Flags = %8x.\n", ReceiveFlags);

}


_Use_decl_annotations_
VOID
FilterCancelSendNetBufferLists(
    NDIS_HANDLE             FilterModuleContext,
    PVOID                   CancelId
    )
/*++

Routine Description:

    This function cancels any NET_BUFFER_LISTs pended in the filter and then
    calls the NdisFCancelSendNetBufferLists to propagate the cancel operation.

    If your driver does not queue any send NBLs, you may omit this routine.  
    NDIS will propagate the cancelation on your behalf more efficiently.

Arguments:

    FilterModuleContext      - our filter context area.
    CancelId                 - an identifier for all NBLs that should be dequeued

Return Value:

    None

*/
{
    PLCXL_FILTER  pFilter = (PLCXL_FILTER)FilterModuleContext;

    NdisFCancelSendNetBufferLists(pFilter->FilterHandle, CancelId);
}


_Use_decl_annotations_
NDIS_STATUS
FilterSetModuleOptions(
    NDIS_HANDLE             FilterModuleContext
    )
/*++

Routine Description:

    This function set the optional handlers for the filter

Arguments:

    FilterModuleContext: The FilterModuleContext given to NdisFSetAttributes

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_FAILURE

--*/
{
   PLCXL_FILTER                               pFilter = (PLCXL_FILTER)FilterModuleContext;
   NDIS_FILTER_PARTIAL_CHARACTERISTICS      OptionalHandlers;
   NDIS_STATUS                              Status = NDIS_STATUS_SUCCESS;
   BOOLEAN                                  bFalse = FALSE;

   //
   // Demonstrate how to change send/receive handlers at runtime.
   //
   if (bFalse)
   {
       UINT      i;


       pFilter->CallsRestart++;

       i = pFilter->CallsRestart % 8;

       pFilter->TrackReceives = TRUE;
       pFilter->TrackSends = TRUE;

       NdisMoveMemory(&OptionalHandlers, &DefaultChars, sizeof(OptionalHandlers));
       OptionalHandlers.Header.Type = NDIS_OBJECT_TYPE_FILTER_PARTIAL_CHARACTERISTICS;
       OptionalHandlers.Header.Size = sizeof(OptionalHandlers);
       switch (i)
       {

            case 0:
                OptionalHandlers.ReceiveNetBufferListsHandler = NULL;
                pFilter->TrackReceives = FALSE;
                break;

            case 1:

                OptionalHandlers.ReturnNetBufferListsHandler = NULL;
                pFilter->TrackReceives = FALSE;
                break;

            case 2:
                OptionalHandlers.SendNetBufferListsHandler = NULL;
                pFilter->TrackSends = FALSE;
                break;

            case 3:
                OptionalHandlers.SendNetBufferListsCompleteHandler = NULL;
                pFilter->TrackSends = FALSE;
                break;

            case 4:
                OptionalHandlers.ReceiveNetBufferListsHandler = NULL;
                OptionalHandlers.ReturnNetBufferListsHandler = NULL;
                break;

            case 5:
                OptionalHandlers.SendNetBufferListsHandler = NULL;
                OptionalHandlers.SendNetBufferListsCompleteHandler = NULL;
                break;

            case 6:

                OptionalHandlers.ReceiveNetBufferListsHandler = NULL;
                OptionalHandlers.ReturnNetBufferListsHandler = NULL;
                OptionalHandlers.SendNetBufferListsHandler = NULL;
                OptionalHandlers.SendNetBufferListsCompleteHandler = NULL;
                break;

            case 7:
                break;
       }
       Status = NdisSetOptionalHandlers(pFilter->FilterHandle, (PNDIS_DRIVER_OPTIONAL_HANDLERS)&OptionalHandlers );
   }
   return Status;
}



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
    )
/*++

Routine Description:

    Utility routine that forms and sends an NDIS_OID_REQUEST to the
    miniport, waits for it to complete, and returns status
    to the caller.

    NOTE: this assumes that the calling routine ensures validity
    of the filter handle until this returns.

Arguments:

    FilterModuleContext - pointer to our filter module context
    RequestType - NdisRequest[Set|Query|method]Information
    Oid - the object being set/queried
    InformationBuffer - data for the request
    InformationBufferLength - length of the above
    OutputBufferLength  - valid only for method request
    MethodId - valid only for method request
    pBytesProcessed - place to return bytes read/written

Return Value:

    Status of the set/query request

--*/
{
    FILTER_REQUEST              FilterRequest;
    PNDIS_OID_REQUEST           NdisRequest = &FilterRequest.Request;
    NDIS_STATUS                 Status;
    BOOLEAN                     bFalse;


    bFalse = FALSE;
    *pBytesProcessed = 0;
    NdisZeroMemory(NdisRequest, sizeof(NDIS_OID_REQUEST));

    NdisInitializeEvent(&FilterRequest.ReqEvent);

    NdisRequest->Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
    NdisRequest->Header.Revision = NDIS_OID_REQUEST_REVISION_1;
    NdisRequest->Header.Size = sizeof(NDIS_OID_REQUEST);
    NdisRequest->RequestType = RequestType;

    switch (RequestType)
    {
        case NdisRequestQueryInformation:
             NdisRequest->DATA.QUERY_INFORMATION.Oid = Oid;
             NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
                                    InformationBuffer;
             NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
                                    InformationBufferLength;
            break;

        case NdisRequestSetInformation:
             NdisRequest->DATA.SET_INFORMATION.Oid = Oid;
             NdisRequest->DATA.SET_INFORMATION.InformationBuffer =
                                    InformationBuffer;
             NdisRequest->DATA.SET_INFORMATION.InformationBufferLength =
                                    InformationBufferLength;
            break;

        case NdisRequestMethod:
             NdisRequest->DATA.METHOD_INFORMATION.Oid = Oid;
             NdisRequest->DATA.METHOD_INFORMATION.MethodId = MethodId;
             NdisRequest->DATA.METHOD_INFORMATION.InformationBuffer =
                                    InformationBuffer;
             NdisRequest->DATA.METHOD_INFORMATION.InputBufferLength =
                                    InformationBufferLength;
             NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength = OutputBufferLength;
             break;



        default:
            FILTER_ASSERT(bFalse);
            break;
    }

    NdisRequest->RequestId = (PVOID)FILTER_REQUEST_ID;

    Status = NdisFOidRequest(FilterModuleContext->FilterHandle,
                            NdisRequest);


    if (Status == NDIS_STATUS_PENDING)
    {

        NdisWaitEvent(&FilterRequest.ReqEvent, 0);
        Status = FilterRequest.Status;
    }


    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (RequestType == NdisRequestSetInformation)
        {
            *pBytesProcessed = NdisRequest->DATA.SET_INFORMATION.BytesRead;
        }

        if (RequestType == NdisRequestQueryInformation)
        {
            *pBytesProcessed = NdisRequest->DATA.QUERY_INFORMATION.BytesWritten;
        }

        if (RequestType == NdisRequestMethod)
        {
            *pBytesProcessed = NdisRequest->DATA.METHOD_INFORMATION.BytesWritten;
        }

        //
        // The driver below should set the correct value to BytesWritten
        // or BytesRead. But now, we just truncate the value to InformationBufferLength
        //
        if (RequestType == NdisRequestMethod)
        {
            if (*pBytesProcessed > OutputBufferLength)
            {
                *pBytesProcessed = OutputBufferLength;
            }
        }
        else
        {

            if (*pBytesProcessed > InformationBufferLength)
            {
                *pBytesProcessed = InformationBufferLength;
            }
        }
    }


    return Status;
}

VOID
filterInternalRequestComplete(
    _In_ NDIS_HANDLE                  FilterModuleContext,
    _In_ PNDIS_OID_REQUEST            NdisRequest,
    _In_ NDIS_STATUS                  Status
    )
/*++

Routine Description:

    NDIS entry point indicating completion of a pended NDIS_OID_REQUEST.

Arguments:

    FilterModuleContext - pointer to filter module context
    NdisRequest - pointer to NDIS request
    Status - status of request completion

Return Value:

    None

--*/
{
    PFILTER_REQUEST              FilterRequest;


    UNREFERENCED_PARAMETER(FilterModuleContext);

    //
    //  Get at the request context.
    //
    FilterRequest = CONTAINING_RECORD(NdisRequest, FILTER_REQUEST, Request);

    //
    //  Save away the completion status.
    //
    FilterRequest->Status = Status;

    //
    //  Wake up the thread blocked for this request to complete.
    //
    NdisSetEvent(&FilterRequest->ReqEvent);
}

PLCXL_ROUTE_LIST_ENTRY IfRouteNBL(IN PLCXL_FILTER pFilter, IN PETHERNET_HEADER pEthHeader, IN UINT BufferLength)
{
    USHORT UNALIGNED *pEthType = NULL;
	USHORT EthType;
    if (BufferLength < sizeof(ETHERNET_HEADER)) {
        return NULL;
    }
    EthType = ntohs(pEthHeader->Type);
    //判断帧类型是不是8021P_TAG
    if (EthType == ETHERNET_TYPE_802_1Q) {
        if (BufferLength >= sizeof(ETHERNET_HEADER)+4) {
            pEthType = (USHORT UNALIGNED *)((PUCHAR)&pEthHeader->Type + 4);
            BufferLength -= sizeof(ETHERNET_HEADER)+4;
         } else {
            //缺代码
             return NULL;
         }
    } else {
         pEthType = &pEthHeader->Type;
		 EthType = ntohs(pEthHeader->Type);
         BufferLength -= sizeof(ETHERNET_HEADER);
    }
    if (pEthType == NULL) {
        return NULL;
    }

    switch(EthType) {
    case ETHERNET_TYPE_IPV4://IPv4协议
        if (BufferLength>=sizeof(IPV4_HEADER)){
            PIPV4_HEADER pIPHeader;

            pIPHeader = (PIPV4_HEADER)((PUCHAR)pEthType+sizeof(USHORT));
			//查看数据包的目标IP是否是虚拟IP
			if (RtlCompareMemory(&pIPHeader->DestinationAddress, &pFilter->ia_virtual_ip, sizeof(IN_ADDR))==sizeof(IN_ADDR)) {
				//pIPHeader->iaDst
				
				switch (pIPHeader->Protocol) {
				case 0x01://ICMP 

					break;
				case 0x06://TCP
                    //目前仅支持TCP
                    {
                        PTCP_HDR ptcp_header;
                        PLCXL_ROUTE_LIST_ENTRY route_info;

                        ptcp_header = (PTCP_HDR)((PUCHAR)pIPHeader+sizeof(PTCP_HDR));
                        route_info = GetRouteListEntry(pFilter, pIPHeader, ptcp_header);
                        //建立连接的阶段
                        //有TH_SYN的阶段是建立连接的阶段，这个时候就得选择路由信息
                        if ((ptcp_header->th_flags & TH_SYN) != 0) {
                                
                            PSERVER_INFO_LIST_ENTRY server;

                            //选择一个服务器
                            server = SelectServer(pFilter, pIPHeader, ptcp_header);
                            if (server==NULL) {
                                return NULL;
                            }
                            if (route_info==NULL) {
                                route_info = CreateRouteListEntry(pFilter);
                            }
                            //初始化路由信息
                            InitRouteListEntry(route_info, pIPHeader, ptcp_header, server);
                        } else {
                            if (route_info!=NULL) {
                                //如果客户端发出ACK包并且连接处于LAST_ACK状态，则更改状态为CLOSE
                                if ((ptcp_header->th_flags & TH_ACK) !=0 && (route_info->status == RS_LAST_ACK)) {
                                    route_info->status = RS_CLOSED;
                                } else if ((ptcp_header->th_flags & TH_FIN) !=0) {
                                    //如果客户端通知连接要关闭
                                     //更改路由状态为LAST_ACK
                                    route_info->status = RS_LAST_ACK;
                                } else if ((ptcp_header->th_flags & TH_RST) !=0) {
                                    //如果连接重置，直接关闭连接
                                    route_info->status = RS_CLOSED;
                                }
                            }
                        }
                        
                        return route_info;
                    }
					break;
				case 0x11://PROT_UDP
					break;
				default:
					break;
				}
			}			
        }
        break;
	case ETHERNET_TYPE_ARP:
		//ARP_OPCODE  ARP_REQUEST  ARP_RESPONSE
		break;
    case ETHERNET_TYPE_IPV6://IPv6协议
		break;
    default:
        break;
    }
    return NULL;
}

PLCXL_ROUTE_LIST_ENTRY GetRouteListEntry(IN PLCXL_FILTER pFilter, IN PIPV4_HEADER pIPHeader, IN PTCP_HDR pTcpHeader)
{
    //pFilter->route_list.
    PLIST_ENTRY Link = pFilter->route_list.list_entry.Flink;
    PLCXL_ROUTE_LIST_ENTRY route_info;
	//遍历列表
    while (Link != &pFilter->route_list.list_entry)
    {
        route_info = CONTAINING_RECORD(Link, LCXL_ROUTE_LIST_ENTRY, list_entry);
        //查看是否匹配
        if (RtlCompareMemory(&route_info->ia_src, &pIPHeader->SourceAddress, sizeof(pIPHeader->SourceAddress)) == sizeof(pIPHeader->SourceAddress) && route_info->src_port == pTcpHeader->th_sport && route_info->dst_port == pTcpHeader->th_dport ) {
            return route_info;
        }
        Link = Link->Flink;
    }
    return NULL;
}

PLCXL_ROUTE_LIST_ENTRY CreateRouteListEntry(IN PLCXL_FILTER pFilter)
{
    PLCXL_ROUTE_LIST_ENTRY route_info;
    
    ASSERT(pFilter!=NULL);
    route_info = GET_MEM_ROUTE();
    route_info->status = RS_NONE;
    InsertHeadList(&pFilter->route_list.list_entry, &route_info->list_entry);
    return route_info;
}

void InitRouteListEntry(IN OUT PLCXL_ROUTE_LIST_ENTRY route_info, IN PIPV4_HEADER pIPHeader, IN PTCP_HDR pTcpHeader, IN PSERVER_INFO_LIST_ENTRY server_info)
{
    ASSERT(route_info!=NULL);
    ASSERT(pIPHeader!=NULL);
    ASSERT(pTcpHeader!=NULL);
    ASSERT(server_info!=NULL);

    route_info->status = RS_NORMAL;
    route_info->dst_server = server_info;
    route_info->dst_port = pTcpHeader->th_dport;
    route_info->src_port = pTcpHeader->th_sport;
    route_info->ia_src = pIPHeader->SourceAddress;
}

PSERVER_INFO_LIST_ENTRY SelectServer(IN PLCXL_FILTER pFilter, IN PIPV4_HEADER pIPHeader, IN PTCP_HDR pTcpHeader)
{
    //pFilter->route_list.
    PLIST_ENTRY Link = pFilter->server_list.list_entry.Flink;
    PSERVER_INFO_LIST_ENTRY server_info;
    PSERVER_INFO_LIST_ENTRY best_server = NULL;

    UNREFERENCED_PARAMETER(pIPHeader);
    UNREFERENCED_PARAMETER(pTcpHeader);
    //遍历列表
    while (Link != &pFilter->route_list.list_entry)
    {
        server_info = CONTAINING_RECORD(Link, SERVER_INFO_LIST_ENTRY, list_entry);
        //检查服务器是否可用
        if ((server_info->server_status.Status&SS_ENABLED) !=0 && (server_info->server_status.Status&SS_ONLINE) !=0) {
            if (best_server==NULL||best_server->server_status.ProcessTime > server_info->server_status.ProcessTime) {
                best_server = server_info;
            }
        }
        Link = Link->Flink;
    }
    return best_server;
}