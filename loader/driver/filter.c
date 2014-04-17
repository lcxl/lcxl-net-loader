/*++

Copyright (c) Microsoft Corporation

Module Name:

    Filter.c

Abstract:

    Sample NDIS Lightweight filter driver

--*/

#include "precomp.h"
#include "filter.h"//filter.c

#define __FILENUMBER    'PNPF'

// This directive puts the DriverEntry function into the INIT segment of the
// driver.  To conserve memory, the code will be discarded when the driver's
// DriverEntry function returns.  You can declare other functions used only
// during initialization here.
#pragma NDIS_INIT_FUNCTION(DriverEntry)

//
// Global variables
//
NDIS_HANDLE         g_filter_driver_handle; // NDIS handle for filter driver
NDIS_HANDLE         g_filter_driver_object;
NDIS_HANDLE         g_ndis_filter_device_handle = NULL;
PDEVICE_OBJECT      g_device_object = NULL;
//过滤模块列表
LCXL_LOCK_LIST		g_filter_list;

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
	KdPrint(("SYS:DriverEntry\n"));
    g_filter_driver_object = DriverObject;

    do
    {
        //添加代码
		InitServerMemMgr();
		InitRouteMemMgr();
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

        g_filter_driver_handle = NULL;

        //
        // Initialize spin locks
        //
		InitLCXLLockList(&g_filter_list, DelLCXLFilterCallBack);

		
        Status = NdisFRegisterFilterDriver(DriverObject,
                                           (NDIS_HANDLE)g_filter_driver_object,
                                           &FChars,
                                           &g_filter_driver_handle);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, "Register filter driver failed.\n");

			DelLCXLLockList(&g_filter_list);
            break;
        }

        Status = FilterRegisterDevice();

        if (Status != NDIS_STATUS_SUCCESS)
        {
            NdisFDeregisterFilterDriver(g_filter_driver_handle);
            
			DelLCXLLockList(&g_filter_list);
            DEBUGP(DL_WARN, "Register device for the filter driver failed.\n");
            break;
        }

		KeQueryPerformanceCounter(&g_setting.frequency);
    }
    while(bFalse);
	//注册系统初始化完成事件
	IoRegisterDriverReinitialization(DriverObject, DriverReinitialize, NULL);

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

    ASSERT(NdisFilterDriverHandle == g_filter_driver_handle);
    ASSERT(FilterDriverContext == (NDIS_HANDLE)g_filter_driver_object);

    if ((NdisFilterDriverHandle != (NDIS_HANDLE)g_filter_driver_handle) ||
        (FilterDriverContext != (NDIS_HANDLE)g_filter_driver_object))
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
    PLCXL_FILTER			pFilter = NULL;
    NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;
    NDIS_FILTER_ATTRIBUTES	FilterAttributes;
    ULONG					Size;
    BOOLEAN					bFalse = FALSE;
    //添加代码
    NET_BUFFER_LIST_POOL_PARAMETERS PoolParameters;
    //!添加代码!
    DEBUGP(DL_TRACE, "===>FilterAttach: NdisFilterHandle %p\n", NdisFilterHandle);
	KdPrint(("SYS:FilterAttach:miniport:%I64u %wZ %wZ %wZ\n", AttachParameters->BaseMiniportNetLuid, AttachParameters->FilterModuleGuidName, AttachParameters->BaseMiniportInstanceName, AttachParameters->BaseMiniportName));
    do
    {
        ASSERT(FilterDriverContext == (NDIS_HANDLE)g_filter_driver_object);
        if (FilterDriverContext != (NDIS_HANDLE)g_filter_driver_object)
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

        Size = sizeof(LCXL_FILTER);

        pFilter = (PLCXL_FILTER)FILTER_ALLOC_MEM(NdisFilterHandle, Size);
        if (pFilter == NULL)
        {
            DEBUGP(DL_WARN, "Failed to allocate context structure.\n");
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pFilter, sizeof(LCXL_FILTER));
		//pFilter->attach_paramters = AttachParameters;
        
		LoadModuleSetting(&pFilter->module, AttachParameters);
		//初始化服务器列表
		//InitializeListHead(&pFilter->server_list.list_entry);
		//初始化路由列表
		InitializeListHead(&pFilter->route_list);
        //初始化NBL发送池
		NdisZeroMemory(&PoolParameters, sizeof(PoolParameters));
        PoolParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        PoolParameters.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
		PoolParameters.Header.Size = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        PoolParameters.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT;
		//ContextSize必须是MEMORY_ALLOCATION_ALIGNMENT的倍数
		ASSERT(sizeof(NPROT_SEND_NETBUFLIST_RSVD) % MEMORY_ALLOCATION_ALIGNMENT == 0);
		PoolParameters.ContextSize = sizeof(NPROT_SEND_NETBUFLIST_RSVD);
        PoolParameters.fAllocateNetBuffer = TRUE;
        PoolParameters.PoolTag = TAG_SEND_NBL;
        pFilter->send_net_buffer_list_pool = NdisAllocateNetBufferListPool( NdisFilterHandle, &PoolParameters); 

		//!添加代码!
        //
        // The filter should initialize TrackReceives and TrackSends properly. For this
        // driver, since its default characteristic has both a send and a receive handler,
        // these fields are initialized to TRUE.
        //
        pFilter->track_receives = TRUE;
        pFilter->track_sends = TRUE;
        pFilter->filter_handle = NdisFilterHandle;


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

		InitLCXLLockList(&pFilter->module.server_list, DelServerCallBack);
        pFilter->state = FilterPaused;
		AddtoLCXLLockList(&g_filter_list, &pFilter->filter_module_link);
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
    PLCXL_FILTER			pFilter = (PLCXL_FILTER)(FilterModuleContext);
    NDIS_STATUS				Status;
    KLOCK_QUEUE_HANDLE		lock_handle;

    UNREFERENCED_PARAMETER(PauseParameters);

    DEBUGP(DL_TRACE, "===>NDISLWF FilterPause: FilterInstance %p\n", FilterModuleContext);
	KdPrint(("SYS:FilterPause:miniport:%I64u %wZ %wZ %wZ\n", pFilter->module.miniport_net_luid.Value, pFilter->module.filter_module_name, pFilter->module.miniport_friendly_name, pFilter->module.miniport_name));

    //
    // Set the flag that the filter is going to pause
    //
    FILTER_ASSERT(pFilter->state == FilterRunning);

   
	LockFilter(pFilter, &lock_handle);
    pFilter->state = FilterPausing;
	UnlockFilter(&lock_handle);

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

    pFilter->state = FilterPaused;

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
	KdPrint(("SYS:FilterRestart:miniport:%I64u %wZ %wZ %wZ\n", pFilter->module.miniport_net_luid.Value, pFilter->module.filter_module_name, pFilter->module.miniport_friendly_name, pFilter->module.miniport_name));

    FILTER_ASSERT(pFilter->state == FilterPaused);

    ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
    ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
    ConfigObject.Header.Size = sizeof(NDIS_CONFIGURATION_OBJECT);
    ConfigObject.NdisHandle = g_filter_driver_handle;
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
    pFilter->state = FilterRunning; // when successful


    Status = NDIS_STATUS_SUCCESS;


    //
    // Ensure the state is Paused if restart failed.
    //

    if (Status != NDIS_STATUS_SUCCESS)
    {
        pFilter->state = FilterPaused;
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
	PLIST_ENTRY list_entry;
    DEBUGP(DL_TRACE, "===>FilterDetach:    FilterInstance %p\n", FilterModuleContext);

	KdPrint(("SYS:FilterDetach:miniport:%I64u %wZ %wZ %wZ\n", pFilter->module.miniport_net_luid.Value, pFilter->module.filter_module_name, pFilter->module.miniport_friendly_name, pFilter->module.miniport_name));
	//
    // Filter must be in paused state
    //
    FILTER_ASSERT(pFilter->state == FilterPaused);


    //
    // Detach must not fail, so do not put any code here that can possibly fail.
    //


    //添加代码
    if (pFilter->send_net_buffer_list_pool!=NULL) {
        NdisFreeNetBufferListPool(pFilter->send_net_buffer_list_pool);
	}
	//删除路由表
	list_entry = pFilter->route_list.Flink;
	while (list_entry != &pFilter->route_list) {
		PLCXL_ROUTE_LIST_ENTRY route_info;

		route_info = CONTAINING_RECORD(list_entry, LCXL_ROUTE_LIST_ENTRY, list_entry);
		list_entry = list_entry->Flink;
		DeleteRouteListEntry(route_info, &pFilter->module.server_list);
	}
	
	
	//服务器列表每个服务器项引用-1
	LockLCXLLockList(&pFilter->module.server_list);
	list_entry = GetListofLCXLLockList(&pFilter->module.server_list)->Flink;
	if (list_entry != GetListofLCXLLockList(&pFilter->module.server_list)) {
		PSERVER_INFO_LIST_ENTRY server_info;
		
		server_info = GetServerbyListEntry(list_entry);
		DecRefListEntry(&pFilter->module.server_list, &server_info->list_entry);
		//此时引用计数应该为0
		ASSERT(server_info->list_entry.ref_count == 0);
		list_entry = list_entry->Flink;
	}
	UnlockLCXLLockList(&pFilter->module.server_list);
	
	DelLCXLLockList(&pFilter->module.server_list);
    //!添加代码!
	//删除
	DelFromLCXLLockList(&g_filter_list, &pFilter->filter_module_link);

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

    UNREFERENCED_PARAMETER(DriverObject);

    DEBUGP(DL_TRACE, "===>FilterUnload\n");

    //
    // Should free the filter context list
    //
    FilterDeregisterDevice();
    NdisFDeregisterFilterDriver(g_filter_driver_handle);

#if DBG
	LockLCXLLockList(&g_filter_list);
    
    ASSERT(IsListEmpty(GetListofLCXLLockList(&g_filter_list)));

	UnlockLCXLLockList(&g_filter_list);

#endif
	//删除列表
	DelLCXLLockList(&g_filter_list);

    DEBUGP(DL_TRACE, "<===FilterUnload\n");
    //添加代码
	DelRouteMemMgr();
	DelServerMemMgr();
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
        Status = NdisAllocateCloneOidRequest(pFilter->filter_handle,
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

        pFilter->pending_oid_request = ClonedRequest;

		switch (ClonedRequest->RequestType) {
		case NdisRequestSetInformation:
			switch (ClonedRequest->DATA.SET_INFORMATION.Oid) {
			case OID_GEN_CURRENT_PACKET_FILTER:
				if (*(NDIS_OID*)ClonedRequest->DATA.SET_INFORMATION.InformationBuffer == NDIS_PACKET_TYPE_PROMISCUOUS) {
					KdPrint(("SYS:FilterOidRequest:set NDIS_PACKET_TYPE_PROMISCUOUS\n"));
				} else {
					KdPrint(("SYS:FilterOidRequest:cancel NDIS_PACKET_TYPE_PROMISCUOUS\n"));
				}
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
        Status = NdisFOidRequest(pFilter->filter_handle, ClonedRequest);

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
    PLCXL_FILTER						filter = (PLCXL_FILTER)FilterModuleContext;
    PNDIS_OID_REQUEST                   Request = NULL;
    PFILTER_REQUEST_CONTEXT             Context;
    PNDIS_OID_REQUEST                   OriginalRequest = NULL;
	KLOCK_QUEUE_HANDLE					lock_handle;

	LockFilter(filter, &lock_handle);

    Request = filter->pending_oid_request;

    if (Request != NULL)
    {
        Context = (PFILTER_REQUEST_CONTEXT)(&Request->SourceReserved[0]);

        OriginalRequest = (*Context);
    }

    if ((OriginalRequest != NULL) && (OriginalRequest->RequestId == RequestId))
    {
		UnlockFilter(&lock_handle);

        NdisFCancelOidRequest(filter->filter_handle, RequestId);
    }
    else
    {
		UnlockFilter(&lock_handle);
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
	KLOCK_QUEUE_HANDLE					lock_handle;

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


    LockFilter(pFilter, &lock_handle);

    ASSERT(pFilter->pending_oid_request == Request);
    pFilter->pending_oid_request = NULL;

	UnlockFilter(&lock_handle);


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

    NdisFreeCloneOidRequest(pFilter->filter_handle, Request);

    NdisFOidRequestComplete(pFilter->filter_handle, OriginalRequest, Status);

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
	KLOCK_QUEUE_HANDLE			lock_handle;
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
	LockFilter(pFilter, &lock_handle);
    ASSERT(pFilter->bIndicating == FALSE);
    pFilter->bIndicating = TRUE;
	UnlockFilter(&lock_handle);
#endif // DBG

    NdisFIndicateStatus(pFilter->filter_handle, StatusIndication);

#if DBG
	LockFilter(pFilter, &lock_handle);
    ASSERT(pFilter->bIndicating == TRUE);
    pFilter->bIndicating = FALSE;
	UnlockFilter(&lock_handle);
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

    NdisFDevicePnPEventNotify(pFilter->filter_handle, NetDevicePnPEvent);

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

    Status = NdisFNetPnPEvent(pFilter->filter_handle, NetPnPEventNotification);

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
    //BOOLEAN            DispatchLevel;
	KLOCK_QUEUE_HANDLE lock_handle;
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

    if (pFilter->track_sends)
    {
        CurrNbl = NetBufferLists;
        while (CurrNbl)
        {
            NumOfSendCompletes++;
            CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);

        }
        //DispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendCompleteFlags);
        //FILTER_ACQUIRE_LOCK(&pFilter->lock, DispatchLevel);
		LockFilter(pFilter, &lock_handle);
        pFilter->outstanding_sends -= NumOfSendCompletes;
        FILTER_LOG_SEND_REF(2, pFilter, PrevNbl, pFilter->outstanding_sends);
		UnlockFilter(&lock_handle);
    }
    //添加代码
    //Note  A filter driver should keep track of send requests that 
    //it originates and make sure that it does NOT call the NdisFSendNetBufferListsComplete function 
    //when such requests are complete.
    CurrNbl = NetBufferLists;
    PrepNbl = NULL;
    while (CurrNbl != NULL) {
        //如果是本驱动发出来的NBL，将此NBL脱离当前的NBL链
        if (CurrNbl->SourceHandle == pFilter->filter_handle) {
            PNET_BUFFER_LIST	drop_nbl;
			PMDL				send_mdl = NULL;
			PVOID				send_buffer;
			ULONG				buffer_length;

            drop_nbl = CurrNbl;
			CurrNbl = NET_BUFFER_LIST_NEXT_NBL(drop_nbl);
            //判断是否是链表头
			if (drop_nbl != NetBufferLists) {
                //不是链表头
				
                ASSERT(PrepNbl!=NULL);
                NET_BUFFER_LIST_NEXT_NBL(PrepNbl) = CurrNbl;
            } else {
                //是链表头
				NetBufferLists = CurrNbl;
				
            }
            //断开和原始链的关联
            NET_BUFFER_LIST_NEXT_NBL(drop_nbl) = NULL;

			send_mdl = NET_BUFFER_FIRST_MDL(NET_BUFFER_LIST_FIRST_NB(drop_nbl));
			ASSERT(send_mdl != NULL);
			//查询mdl
			NdisQueryMdl(
				send_mdl,
				&send_buffer,
				&buffer_length,
				NormalPagePriority);

			ASSERT(send_buffer != NULL);
			//释放mdl
			NdisFreeMdl(send_mdl);
			//释放数据包内存
			FILTER_FREE_MEM(send_buffer);
            //释放NBL
            NdisFreeNetBufferList(drop_nbl);
        } else {
            PrepNbl = CurrNbl;
            CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
        }
    }
    //!添加代码!

    // Send complete the NBLs.  If you removed any NBLs from the chain, make
    // sure the chain isn't empty (i.e., NetBufferLists!=NULL).
	if (NetBufferLists!=NULL) {
		NdisFSendNetBufferListsComplete(pFilter->filter_handle, NetBufferLists, SendCompleteFlags);
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
    PLCXL_FILTER		filter = (PLCXL_FILTER)FilterModuleContext;
    PNET_BUFFER_LIST	current_nbl;
    BOOLEAN				dispatch_level;
	KLOCK_QUEUE_HANDLE	lock_handle;
	//可以通过的NBL
	PNET_BUFFER_LIST    pass_nbl_head = NULL;
	//末尾的NBL
	PNET_BUFFER_LIST    pass_nbl_tail = NULL;
	//ULONG               number_of_pass_nbl = 0;
	//需要丢掉的NBL
	PNET_BUFFER_LIST    drop_nbl_head = NULL;
	PNET_BUFFER_LIST    drop_nbl_tail = NULL;

    DEBUGP(DL_TRACE, "===>SendNetBufferList: NBL = %p.\n", NetBufferLists);

	dispatch_level = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags);
	//#if DBG
	//
	// we should never get packets to send if we are not in running state
	//

	LockFilter(filter, &lock_handle);
	//
	// If the filter is not in running state, fail the send
	//
	if (filter->state != FilterRunning)
	{
		UnlockFilter(&lock_handle);

		current_nbl = NetBufferLists;
		while (current_nbl) {
			NET_BUFFER_LIST_STATUS(current_nbl) = NDIS_STATUS_PAUSED;
			current_nbl = NET_BUFFER_LIST_NEXT_NBL(current_nbl);
		}
		NdisFSendNetBufferListsComplete(filter->filter_handle,
			NetBufferLists,
			dispatch_level ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0);
		
		return;
	}
	UnlockFilter(&lock_handle);
	//#endif

	current_nbl = NetBufferLists;
	while (current_nbl)
	{
		UINT				buffer_length = 0;
		PETHERNET_HEADER	eth_header;
		PROCESS_NBL_RESULT	return_data = { 0 };

		eth_header = GetEthernetHeader(current_nbl, &buffer_length);

		if (eth_header != NULL) {
			INT lcxl_role;

			lcxl_role = g_setting.lcxl_role;
			ProcessNBL(filter, FALSE, lcxl_role, eth_header, buffer_length, &return_data);
		}
		switch (return_data.code) {
		case PNRC_PASS:case PNRC_MODIFY:
			if (pass_nbl_head == NULL) {
				pass_nbl_head = current_nbl;
				pass_nbl_tail = current_nbl;
			} else {
				NET_BUFFER_LIST_NEXT_NBL(pass_nbl_tail) = current_nbl;
				pass_nbl_tail = current_nbl;
			}
			if (return_data.code == PNRC_MODIFY) {
				//更改源地址mac
				IF_PHYSICAL_ADDRESS router_mac_addr;
				KLOCK_QUEUE_HANDLE lock_handle;
				//获取router的mac地址
				LockFilter(filter, &lock_handle);
				router_mac_addr = filter->module.router_mac_addr;
				UnlockFilter(&lock_handle);
				//将源MAC地址更改为router的mac地址
				RtlCopyMemory(&eth_header->Source, router_mac_addr.Address, sizeof(eth_header->Source));
			}
			break;
		case PNRC_DROP:
			//拦截此NBL
			if (drop_nbl_head == NULL) {
				drop_nbl_head = current_nbl;
				drop_nbl_tail = current_nbl;
			} else {
				NET_BUFFER_LIST_NEXT_NBL(drop_nbl_tail) = current_nbl;
				drop_nbl_tail = current_nbl;
			}
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		if (filter->track_sends)
		{
			LockFilter(filter, &lock_handle);
			filter->outstanding_sends++;
			FILTER_LOG_SEND_REF(1, filter, current_nbl, filter->outstanding_sends);
			UnlockFilter(&lock_handle);
		}
		
		current_nbl = NET_BUFFER_LIST_NEXT_NBL(current_nbl);
	}

	if (drop_nbl_head != NULL) {
		NET_BUFFER_LIST_NEXT_NBL(drop_nbl_tail) = NULL;
		NdisFSendNetBufferListsComplete(filter->filter_handle,
			drop_nbl_head,
			dispatch_level ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0);
	}

	if (pass_nbl_head != NULL) {
		NET_BUFFER_LIST_NEXT_NBL(pass_nbl_tail) = NULL;
		NdisFSendNetBufferLists(filter->filter_handle, pass_nbl_head, PortNumber, SendFlags);
	}

	//
	// If necessary, queue the NetBufferLists in a local structure for later
	// processing.  However, do not queue them for "too long", or else the
	// system's performance may be degraded.  If you need to hold onto an
	// NBL for an unbounded amount of time, then allocate memory, perform a
	// deep copy, and complete the original NBL.
	//

	//NdisFSendNetBufferLists(filter->filter_handle, NetBufferLists, PortNumber, SendFlags);
    
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

    if (pFilter->track_receives)
    {
        while (CurrNbl)
        {
            NumOfNetBufferLists ++;
            CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
        }
    }

    
    // Return the received NBLs.  If you removed any NBLs from the chain, make
    // sure the chain isn't empty (i.e., NetBufferLists!=NULL).

    NdisFReturnNetBufferLists(pFilter->filter_handle, NetBufferLists, ReturnFlags);

    if (pFilter->track_receives)
    {
        //DispatchLevel = NDIS_TEST_RETURN_AT_DISPATCH_LEVEL(ReturnFlags);
		KLOCK_QUEUE_HANDLE lock_handle;
		LockFilter(pFilter, &lock_handle);

        pFilter->outstanding_rcvs -= NumOfNetBufferLists;
        Ref = pFilter->outstanding_rcvs;
        FILTER_LOG_RCV_REF(3, pFilter, NetBufferLists, Ref);
		UnlockFilter(&lock_handle);
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

    PLCXL_FILTER        filter = (PLCXL_FILTER)FilterModuleContext;
    BOOLEAN             dispatch_level;
    ULONG               rcv_ref;
    //修改代码，去掉DBG判定
//#if DBG
    ULONG               return_flags = 0;
//#endif
    //!修改代码!
	PNET_BUFFER_LIST    current_nbl;
    //添加代码

    ULONG               send_flags = 0;

    //可以通过的NBL
    PNET_BUFFER_LIST    pass_nbl_head = NULL;
    //末尾的NBL
    PNET_BUFFER_LIST    pass_nbl_tail = NULL;
    ULONG               number_of_pass_nbl = 0;
    //需要丢掉的NBL
    PNET_BUFFER_LIST    drop_nbl_head = NULL;
    PNET_BUFFER_LIST    drop_nbl_tail = NULL;
    //要转发的NBL列表
    PNET_BUFFER_LIST	send_nbl_head = NULL;
	PNET_BUFFER_LIST	send_nbl_tail = NULL;
    //!添加代码!
	KLOCK_QUEUE_HANDLE	lock_handle;

    DEBUGP(DL_TRACE, "===>ReceiveNetBufferList: NetBufferLists = %p.\n", NetBufferLists);
	dispatch_level = NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags);
	if (dispatch_level) {
        NDIS_SET_RETURN_FLAG(return_flags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
        NDIS_SET_SEND_FLAG(send_flags, NDIS_SEND_FLAGS_DISPATCH_LEVEL);
    }

	LockFilter(filter, &lock_handle);
	//如果没有在运行或者模块信息没有加载
	if (filter->state != FilterRunning) {
		UnlockFilter(&lock_handle);

		if (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags)) {
			NdisFReturnNetBufferLists(filter->filter_handle, NetBufferLists, return_flags);
		}
		return;
	}
	UnlockFilter(&lock_handle);

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
	current_nbl = NetBufferLists;
	while (current_nbl != NULL) {
		UINT					data_length = 0;
		PETHERNET_HEADER		ethernet_header = NULL;
		//是否此数据包已处理
		
		PROCESS_NBL_RESULT	return_data = { 0 };
		
		ethernet_header = GetEthernetHeader(current_nbl, &data_length);
		
		//各种有效性判断
		if (ethernet_header != NULL) {
			INT lcxl_role;
			
			lcxl_role  = g_setting.lcxl_role;
			ProcessNBL(filter, TRUE, lcxl_role, ethernet_header, data_length, &return_data);
		}
		switch (return_data.code) {
		case PNRC_DROP:case PNRC_ROUTER:
			//如果要拦截此NBL
			//是否可以Pend
			if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags)) {
				//不作处理

			} else {
				//虚拟IP的数据包不能传到上层驱动，需要丢弃
				if (drop_nbl_head == NULL) {
					drop_nbl_head = current_nbl;
					drop_nbl_tail = current_nbl;
				} else {
					NET_BUFFER_LIST_NEXT_NBL(drop_nbl_tail) = current_nbl;
					drop_nbl_tail = current_nbl;
				}
			}
			//如果需要路由此数据包
			if (return_data.code == PNRC_ROUTER) {
				PETHERNET_HEADER    send_buffer;
				PMDL                send_mdl = NULL;
				PNET_BUFFER_LIST    send_nbl;
				ULONG               bytes_copied;
				NDIS_STATUS			status;
				ASSERT(return_data.data.route != NULL);
				//创建一个NBL
				send_buffer = (PETHERNET_HEADER)FILTER_ALLOC_MEM(filter->filter_handle, data_length);
				

				send_mdl = NdisAllocateMdl(filter->filter_handle, send_buffer, data_length);
				send_nbl = NdisAllocateNetBufferAndNetBufferList(
					filter->send_net_buffer_list_pool,
					sizeof(NPROT_SEND_NETBUFLIST_RSVD),// ContextSize
					0,                              // ContextBackfill
					send_mdl,                           // MdlChain
					0,                              // DataOffset
					data_length);                   // DataLength
				
				status = NdisCopyFromNetBufferToNetBuffer(
					NET_BUFFER_LIST_FIRST_NB(send_nbl),
					0,
					data_length,
					NET_BUFFER_LIST_FIRST_NB(current_nbl),
					0,
					&bytes_copied);
					
				if (!NT_SUCCESS(status)) {
					KdPrint(("NdisCopyFromNetBufferToNetBuffer failed.error code=%08x\n", status));
				} else {
					ASSERT(bytes_copied == data_length);
				}
				
				//NdisMoveMemory(send_buffer, ethernet_header, data_length);
				//设置SourceHandle
				//A filter driver must set the SourceHandle member of each NET_BUFFER_LIST structure that it originates to the same value that it passes to the NdisFilterHandle parameter

				send_nbl->SourceHandle = filter->filter_handle;
				//修改目标MAC地址
				//RtlCopyMemory(&send_buffer->Source, &send_buffer->Destination, sizeof(ethernet_header->Source));
				NdisMoveMemory(&send_buffer->Destination, return_data.data.route->dst_server->info.mac_addr.Address, sizeof(send_buffer->Destination));
				
				//
				// The other members of NET_BUFFER_DATA structure are already initialized properly during allocation.
				//
				NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB(send_nbl)) = data_length;
				//插入到转发队列中
				if (send_nbl_head == NULL) {
					send_nbl_head = send_nbl;
					send_nbl_tail = send_nbl;
				} else {
					NET_BUFFER_LIST_NEXT_NBL(send_nbl_tail) = send_nbl;
					send_nbl_tail = send_nbl;
				}
				if (return_data.data.route->status == RS_CLOSED) {
					DeleteRouteListEntry(return_data.data.route, &filter->module.server_list);
				}
			}
			break;
		case PNRC_PASS:
			//如果数据包未被处理，则发送给上层驱动
			if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags)) {
				PNET_BUFFER_LIST tmpNBL;

				tmpNBL = NET_BUFFER_LIST_NEXT_NBL(current_nbl);
				NET_BUFFER_LIST_NEXT_NBL(current_nbl) = NULL;
				//接受不丢弃的数据包
				NdisFIndicateReceiveNetBufferLists(filter->filter_handle, current_nbl, PortNumber, 1, ReceiveFlags | NDIS_RECEIVE_FLAGS_RESOURCES);
				NET_BUFFER_LIST_NEXT_NBL(current_nbl) = tmpNBL;
			} else {
				if (pass_nbl_head == NULL) {
					pass_nbl_head = current_nbl;
					pass_nbl_tail = current_nbl;
				} else {
					NET_BUFFER_LIST_NEXT_NBL(pass_nbl_tail) = current_nbl;
					pass_nbl_tail = current_nbl;
				}
				number_of_pass_nbl++;
			}
			break;
		}
		//获取下一个NBL
		current_nbl = NET_BUFFER_LIST_NEXT_NBL(current_nbl);
	}
	if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags)) {
		//不接受的数据包
		//NDIS_TEST_RECEIVE_CANNOT_PEND为TRUE时，不要做任何操作，直接返回
	} else {
		ASSERT(pass_nbl_tail != NULL || drop_nbl_tail != NULL);
		//将两个链表的最后一项的Next域清空
		if (pass_nbl_tail != NULL) {
			NET_BUFFER_LIST_NEXT_NBL(pass_nbl_tail) = NULL;
			//接受PassHeadNBL
			NdisFIndicateReceiveNetBufferLists(filter->filter_handle, pass_nbl_head, PortNumber, number_of_pass_nbl, ReceiveFlags);
		}
		if (drop_nbl_tail != NULL) {
			NET_BUFFER_LIST_NEXT_NBL(drop_nbl_tail) = NULL;
			//丢弃DropHeadNBL
			NdisFReturnNetBufferLists(filter->filter_handle, drop_nbl_head, return_flags);
		}

	}
	//转发数据包给真实的服务器
	if (NULL != send_nbl_head) {
		NET_BUFFER_LIST_NEXT_NBL(send_nbl_tail) = NULL;
		NdisFSendNetBufferLists(filter->filter_handle, send_nbl_head, /*NDIS_DEFAULT_PORT_NUMBER*/PortNumber, send_flags);
	}
	//
	if (filter->track_receives)
	{
		LockFilter(filter, &lock_handle);
		filter->outstanding_rcvs += NumberOfNetBufferLists;
		rcv_ref = filter->outstanding_rcvs;

		FILTER_LOG_RCV_REF(1, filter, NetBufferLists, rcv_ref);
		UnlockFilter(&lock_handle);
	}
	if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags) &&
		filter->track_receives)
	{
		LockFilter(filter, &lock_handle);
		filter->outstanding_rcvs -= NumberOfNetBufferLists;
		rcv_ref = filter->outstanding_rcvs;
		FILTER_LOG_RCV_REF(2, filter, NetBufferLists, rcv_ref);
		UnlockFilter(&lock_handle);
	}

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

    NdisFCancelSendNetBufferLists(pFilter->filter_handle, CancelId);
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


       pFilter->calls_restart++;

       i = pFilter->calls_restart % 8;

       pFilter->track_receives = TRUE;
       pFilter->track_sends = TRUE;

       NdisMoveMemory(&OptionalHandlers, &DefaultChars, sizeof(OptionalHandlers));
       OptionalHandlers.Header.Type = NDIS_OBJECT_TYPE_FILTER_PARTIAL_CHARACTERISTICS;
       OptionalHandlers.Header.Size = sizeof(OptionalHandlers);
       switch (i)
       {

            case 0:
                OptionalHandlers.ReceiveNetBufferListsHandler = NULL;
                pFilter->track_receives = FALSE;
                break;

            case 1:

                OptionalHandlers.ReturnNetBufferListsHandler = NULL;
                pFilter->track_receives = FALSE;
                break;

            case 2:
                OptionalHandlers.SendNetBufferListsHandler = NULL;
                pFilter->track_sends = FALSE;
                break;

            case 3:
                OptionalHandlers.SendNetBufferListsCompleteHandler = NULL;
                pFilter->track_sends = FALSE;
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
       Status = NdisSetOptionalHandlers(pFilter->filter_handle, (PNDIS_DRIVER_OPTIONAL_HANDLERS)&OptionalHandlers );
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

    Status = NdisFOidRequest(FilterModuleContext->filter_handle,
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

VOID DriverReinitialize(
	_In_      struct _DRIVER_OBJECT *DriverObject,
	_In_opt_  PVOID Context,
	_In_      ULONG Count
	)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(Count);
	KdPrint(("SYS:DriverReinitialize begin\n"));
	KdPrint(("SYS:DriverReinitialize end\n"));
}

PLCXL_ROUTE_LIST_ENTRY RouteTCPNBL(IN PLCXL_FILTER filter, IN INT ipMode, IN PVOID pIPHeader) 
{
	PTCP_HDR ptcp_header = NULL;
	PLCXL_ROUTE_LIST_ENTRY route_info;

	switch (ipMode) {
	case IM_IPV4:
		ptcp_header = (PTCP_HDR)((PUCHAR)pIPHeader + Ip4HeaderLengthInBytes((PIPV4_HEADER)pIPHeader));
		break;
	case IM_IPV6:
		ptcp_header = (PTCP_HDR)((PUCHAR)pIPHeader + sizeof(IPV6_HEADER));
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	route_info = GetRouteListEntry(&filter->route_list, filter->module.route_timeout, &filter->module.server_list, ipMode, pIPHeader, ptcp_header);
	//建立连接的阶段
	//有TH_SYN的阶段是建立连接的阶段，这个时候就得选择路由信息
	if ((ptcp_header->th_flags & TH_SYN) != 0) {
		PSERVER_INFO_LIST_ENTRY server;

		//选择一个服务器
		server = SelectBestServer(&filter->module.server_list, ipMode, pIPHeader, ptcp_header);
		if (server == NULL) {
			KdPrint(("SYS:route_info TH_SYN server = NULL, return\n"));
			return NULL;
		}
		if (route_info == NULL) {
			route_info = CreateRouteListEntry(&filter->route_list);
			KdPrint(("SYS:create route_info\n"));
		} else {
			DecRefListEntry(&filter->module.server_list, &route_info->dst_server->list_entry);
			KdPrint(("SYS:reuse route_info\n"));
		}
		//初始化路由信息
		InitRouteListEntry(route_info, ipMode, pIPHeader, ptcp_header, server);
		KdPrint((
			"SYS:init route_info tomac=%02x:%02x:%02x:%02x:%02x:%02x\n",
			route_info->dst_server->info.mac_addr.Address[0],
			route_info->dst_server->info.mac_addr.Address[1],
			route_info->dst_server->info.mac_addr.Address[2],
			route_info->dst_server->info.mac_addr.Address[3],
			route_info->dst_server->info.mac_addr.Address[4],
			route_info->dst_server->info.mac_addr.Address[5]));
	} else {
		if (route_info != NULL) {
			//如果客户端发出ACK包并且连接处于LAST_ACK状态，则更改状态为CLOSE
			if ((ptcp_header->th_flags & TH_ACK) != 0 && (route_info->status == RS_LAST_ACK)) {
				route_info->status = RS_CLOSED;
				KdPrint(("SYS:route_info RS_CLOSED\n"));
			}
			else if ((ptcp_header->th_flags & TH_FIN) != 0) {
				//如果客户端通知连接要关闭
				//更改路由状态为LAST_ACK
				route_info->status = RS_LAST_ACK;
				KdPrint(("SYS:route_info RS_LAST_ACK\n"));
			}
			else if ((ptcp_header->th_flags & TH_RST) != 0) {
				//如果连接重置，直接关闭连接
				route_info->status = RS_CLOSED;
				KdPrint(("SYS:route_info RS_CLOSED TH_RST\n"));
			}
		} else {
			KdPrint(("SYS:route_info(No TH_SYN) == NULL\n"));
		}
	}

	return route_info;
}
//广播MAC
static DL_EUI48 AnycastMacAddr = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

BOOLEAN CheckTCPNBLMacAddr(IN PIF_PHYSICAL_ADDRESS mac_addr, IN BOOLEAN is_recv, IN PETHERNET_HEADER pEthHeader)
{
	if (is_recv) {
		if (RtlCompareMemory(pEthHeader->Destination.Byte, mac_addr->Address, sizeof(pEthHeader->Destination.Byte)) < sizeof(pEthHeader->Destination.Byte) &&
			RtlCompareMemory(pEthHeader->Destination.Byte, AnycastMacAddr.Byte, sizeof(pEthHeader->Destination.Byte)) < sizeof(pEthHeader->Destination.Byte)){
			KdPrint((
				"SYS:CheckTCPNBLMacAddr recv pEthHeader->Destination(%02x:%02x:%02x:%02x:%02x:%02x) != filter->module.mac_addr.Address\n",
				pEthHeader->Destination.Byte[0],
				pEthHeader->Destination.Byte[1],
				pEthHeader->Destination.Byte[2],
				pEthHeader->Destination.Byte[3],
				pEthHeader->Destination.Byte[4],
				pEthHeader->Destination.Byte[5]));
			return FALSE;
		}
	} else {
		if (RtlCompareMemory(pEthHeader->Source.Byte, mac_addr->Address, sizeof(pEthHeader->Source.Byte)) < sizeof(pEthHeader->Source.Byte) &&
			RtlCompareMemory(pEthHeader->Source.Byte, AnycastMacAddr.Byte, sizeof(pEthHeader->Source.Byte)) < sizeof(pEthHeader->Source.Byte)) {
			KdPrint((
				"SYS:CheckTCPNBLMacAddr send pEthHeader->Source(%02x:%02x:%02x:%02x:%02x:%02x) != filter->module.mac_addr.Address\n",
				pEthHeader->Source.Byte[0],
				pEthHeader->Source.Byte[1],
				pEthHeader->Source.Byte[2],
				pEthHeader->Source.Byte[3],
				pEthHeader->Source.Byte[4],
				pEthHeader->Source.Byte[5]));
			return FALSE;
		}
	}
	return TRUE;
}

VOID ProcessNBL(IN PLCXL_FILTER filter, IN BOOLEAN is_recv, IN INT lcxl_role, IN PETHERNET_HEADER pEthHeader, IN UINT data_length, IN OUT PPROCESS_NBL_RESULT return_data)
{
	PVOID ethernet_data;
	USHORT ethernet_type;
	KLOCK_QUEUE_HANDLE lock_handle;
	LCXL_ADDR_INFO virtual_addr;

	ASSERT(return_data != NULL && filter != NULL && pEthHeader != NULL);
	RtlZeroMemory(return_data, sizeof(PROCESS_NBL_RESULT));
	
	ethernet_data = GetEthernetData(pEthHeader, data_length, &ethernet_type, &data_length);
	if (ethernet_data == NULL) {
		return;
	}

	LockFilter(filter, &lock_handle);
	virtual_addr = filter->module.virtual_addr;
	UnlockFilter(&lock_handle);

	switch (ethernet_type) {
	case ETHERNET_TYPE_ARP:
		//ARP_OPCODE  ARP_REQUEST  ARP_RESPONSE
		if (data_length >= sizeof(ARP_HEADER)) {
			if (virtual_addr.status & SA_ENABLE_IPV4) {
				PARP_HEADER arp_header;

				arp_header = (PARP_HEADER)ethernet_data;

				ProcessARP(filter, is_recv, lcxl_role, arp_header, &virtual_addr, return_data);
			}
		}
		break;
	case ETHERNET_TYPE_IPV4://IPv4协议
		if (data_length >= sizeof(IPV4_HEADER)){
			//查看数据包的目标IP是否是虚拟IP
			if (virtual_addr.status & SA_ENABLE_IPV4) {
				PIPV4_HEADER ip_header;

				ip_header = (PIPV4_HEADER)ethernet_data;

				switch (ip_header->Protocol) {
				case 0x01://ICMP 数据包

					break;
				case 0x06://TCP数据包
					if (CheckTCPNBLMacAddr(&filter->module.mac_addr, is_recv, pEthHeader)) {
						ProcessTCP(filter, is_recv, lcxl_role, ip_header, IM_IPV4, &virtual_addr, return_data);
					}
					
					break;
				case 0x11://PROT_UDP 数据包
					break;
				default:
					break;
				}
			}
		}
		break;

	case ETHERNET_TYPE_IPV6://IPv6协议
		if (data_length >= sizeof(IPV6_HEADER)){
			//虚拟IP是否启用
			if (virtual_addr.status & SA_ENABLE_IPV6) {
				PIPV6_HEADER ip_header;

				ip_header = (PIPV6_HEADER)ethernet_data;

				switch (ip_header->NextHeader) {
				case 0x3A://ICMPv6数据包
					ProcessICMPv6(filter, is_recv, lcxl_role, ip_header, &virtual_addr, return_data);
					break;
				case 0x06://TCP数据包
					if (CheckTCPNBLMacAddr(&filter->module.mac_addr, is_recv, pEthHeader)) {
						ProcessTCP(filter, is_recv, lcxl_role, ip_header, IM_IPV6, &virtual_addr, return_data);
					}
					break;
				case 0x11://PROT_UDP 数据包
					ProcessUDP(filter, is_recv, lcxl_role, ip_header, IM_IPV6, &virtual_addr, return_data);
					break;
				}
			}
		}
		break;
	default:
		break;
	}
}

VOID ProcessARP(IN PLCXL_FILTER filter, IN BOOLEAN is_recv, IN INT lcxl_role, IN PARP_HEADER arp_header, IN PLCXL_ADDR_INFO virtual_addr, IN OUT PPROCESS_NBL_RESULT return_data)
{
	LCXL_ARP_ETHERNET lcxl_arp_ethernet;

	ASSERT(virtual_addr->status && SA_ENABLE_IPV4);
	UNREFERENCED_PARAMETER(virtual_addr);
	UNREFERENCED_PARAMETER(return_data);
	UNREFERENCED_PARAMETER(lcxl_role);
	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(is_recv);
	LCXLReadARPEthernet(arp_header, &lcxl_arp_ethernet);

	//如果不是针对IPv4的ARP协议，则跳过
	if (lcxl_arp_ethernet.ProtocolAddressSpace != ETHERNET_TYPE_IPV4) {
		return;
	}
	
	KdPrint(("SYS:recv=%d, ARP:HAS=%d, \
PAS=%d, \
OC=%d, \
SHA=%02x:%02x:%02x:%02x:%02x:%02x, \
SPA=%d.%d.%d.%d, \
THA=%02x:%02x:%02x:%02x:%02x:%02x, \
TPA=%d.%d.%d.%d\n",
			 is_recv,
			 lcxl_arp_ethernet.HardwareAddressSpace,
			 lcxl_arp_ethernet.ProtocolAddressSpace,
			 lcxl_arp_ethernet.Opcode,
			 lcxl_arp_ethernet.SenderHardwareAddress.Address[0],
			 lcxl_arp_ethernet.SenderHardwareAddress.Address[1],
			 lcxl_arp_ethernet.SenderHardwareAddress.Address[2],
			 lcxl_arp_ethernet.SenderHardwareAddress.Address[3],
			 lcxl_arp_ethernet.SenderHardwareAddress.Address[4],
			 lcxl_arp_ethernet.SenderHardwareAddress.Address[5],
			 lcxl_arp_ethernet.SenderProtocolAddress.S_un.S_un_b.s_b1,
			 lcxl_arp_ethernet.SenderProtocolAddress.S_un.S_un_b.s_b2,
			 lcxl_arp_ethernet.SenderProtocolAddress.S_un.S_un_b.s_b3,
			 lcxl_arp_ethernet.SenderProtocolAddress.S_un.S_un_b.s_b4,
			 lcxl_arp_ethernet.TargetHardwareAddress.Address[0],
			 lcxl_arp_ethernet.TargetHardwareAddress.Address[1],
			 lcxl_arp_ethernet.TargetHardwareAddress.Address[2],
			 lcxl_arp_ethernet.TargetHardwareAddress.Address[3],
			 lcxl_arp_ethernet.TargetHardwareAddress.Address[4],
			 lcxl_arp_ethernet.TargetHardwareAddress.Address[5],
			 lcxl_arp_ethernet.TargetProtocolAddress.S_un.S_un_b.s_b1,
			 lcxl_arp_ethernet.TargetProtocolAddress.S_un.S_un_b.s_b2,
			 lcxl_arp_ethernet.TargetProtocolAddress.S_un.S_un_b.s_b3,
			 lcxl_arp_ethernet.TargetProtocolAddress.S_un.S_un_b.s_b4));
	switch (lcxl_role) {
	case LCXL_ROLE_SERVER:
		switch (lcxl_arp_ethernet.Opcode)
		{
		case ARP_REQUEST:
			if (is_recv) {
				//如果收到虚拟ipv4的请求
				if (RtlCompareMemory(&lcxl_arp_ethernet.TargetProtocolAddress, &virtual_addr->ipv4, sizeof(virtual_addr->ipv4)) == sizeof(virtual_addr->ipv4)) {
					KdPrint(("SYS:recv ARP ARP_REQUEST TargetProtocolAddress PNRC_DROP\n"));
					return_data->code = PNRC_DROP;
				}

				if (RtlCompareMemory(&lcxl_arp_ethernet.SenderProtocolAddress, &virtual_addr->ipv4, sizeof(virtual_addr->ipv4)) == sizeof(virtual_addr->ipv4)) {
					KdPrint(("SYS:recv ARP ARP_REQUEST SenderProtocolAddress PNRC_DROP\n"));
					return_data->code = PNRC_DROP;
				}

			} else {
				//如果发送虚拟ipv4请求
				if (RtlCompareMemory(&lcxl_arp_ethernet.TargetProtocolAddress, &virtual_addr->ipv4, sizeof(virtual_addr->ipv4)) == sizeof(virtual_addr->ipv4)) {
					KdPrint(("SYS:send ARP ARP_REQUEST TargetProtocolAddress PNRC_DROP\n"));
					return_data->code = PNRC_DROP;
				}

				if (RtlCompareMemory(&lcxl_arp_ethernet.SenderProtocolAddress, &virtual_addr->ipv4, sizeof(virtual_addr->ipv4)) == sizeof(virtual_addr->ipv4)) {
					KdPrint(("SYS:send ARP ARP_REQUEST SenderProtocolAddress PNRC_DROP\n"));
					return_data->code = PNRC_DROP;
				}
			}
			break;
		case ARP_RESPONSE:
			if (is_recv) {
				//如果收到虚拟ipv4的回复
				if (RtlCompareMemory(&lcxl_arp_ethernet.SenderProtocolAddress, &virtual_addr->ipv4, sizeof(virtual_addr->ipv4)) == sizeof(virtual_addr->ipv4)) {
					KdPrint(("SYS:recv ARP ARP_RESPONSE PNRC_DROP\n"));
					return_data->code = PNRC_DROP;
				}
			} else {
				//如果发送虚拟ipv4回复
				if (RtlCompareMemory(&lcxl_arp_ethernet.SenderProtocolAddress, &virtual_addr->ipv4, sizeof(virtual_addr->ipv4)) == sizeof(virtual_addr->ipv4)) {
					KdPrint(("SYS:send ARP ARP_RESPONSE PNRC_DROP\n"));
					return_data->code = PNRC_DROP;
				}
			}
			break;
		default:
			break;
		}
		break;
	case LCXL_ROLE_ROUTER:
		break;
	default:
		break;
	}
}

VOID ProcessICMPv6(IN PLCXL_FILTER filter, IN BOOLEAN is_recv, IN INT lcxl_role, IN PIPV6_HEADER ip_header, IN PLCXL_ADDR_INFO virtual_addr, IN OUT PPROCESS_NBL_RESULT return_data)
{
	PICMPV6_MESSAGE icmpv6_message = (PICMPV6_MESSAGE)((PUCHAR)ip_header + sizeof(IPV6_HEADER));

	UNREFERENCED_PARAMETER(filter);
	ASSERT(virtual_addr->status && SA_ENABLE_IPV6);
	KdPrint(("SYS:recv=%d, ICMPv6:Type=%d, Code=%d, Checksum=%d, sa=%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x, da=%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
		is_recv,
		icmpv6_message->Header.Type,
		icmpv6_message->Header.Code,
		icmpv6_message->Header.Checksum,

		ntohs(ip_header->SourceAddress.u.Word[0]),
		ntohs(ip_header->SourceAddress.u.Word[1]),
		ntohs(ip_header->SourceAddress.u.Word[2]),
		ntohs(ip_header->SourceAddress.u.Word[3]),
		ntohs(ip_header->SourceAddress.u.Word[4]),
		ntohs(ip_header->SourceAddress.u.Word[5]),
		ntohs(ip_header->SourceAddress.u.Word[6]),
		ntohs(ip_header->SourceAddress.u.Word[7]),

		ntohs(ip_header->DestinationAddress.u.Word[0]),
		ntohs(ip_header->DestinationAddress.u.Word[1]),
		ntohs(ip_header->DestinationAddress.u.Word[2]),
		ntohs(ip_header->DestinationAddress.u.Word[3]),
		ntohs(ip_header->DestinationAddress.u.Word[4]),
		ntohs(ip_header->DestinationAddress.u.Word[5]),
		ntohs(ip_header->DestinationAddress.u.Word[6]),
		ntohs(ip_header->DestinationAddress.u.Word[7])
		));

	switch (lcxl_role) {
	case LCXL_ROLE_SERVER:
		//阻止服务器收到负载均衡器对虚拟IP的通告
		switch (icmpv6_message->Header.Type) {
		case 133://路由器请求（RS）
			if (RtlCompareMemory(&ip_header->SourceAddress, &virtual_addr->ipv6, sizeof(ip_header->SourceAddress)) == sizeof(ip_header->SourceAddress)) {
				//如果本机发送基于虚拟IPv6的请求，则拦截
				if (!is_recv) {
					return_data->code = PNRC_DROP;
					KdPrint(("SYS:ICMPv6 RS PNRC_DROP\n"));
				}
			}
		case 135://邻居请求(NS)
			//这里要处理请求节点多播地址(solicited-node multicast address) 
			if (!is_recv) {
				//如果要发送邻居请求
				IN6_ADDR solicited_node_multicast_address;

				RtlZeroMemory(&solicited_node_multicast_address, sizeof(solicited_node_multicast_address));
				//计算虚拟IPv6地址的被请求-节点多播地址
				//前缀为 FF02::1:FF00:0/104
				solicited_node_multicast_address.u.Word[0] = ntohs(0xFF02);
				solicited_node_multicast_address.u.Word[5] = ntohs(0x0001);
				solicited_node_multicast_address.u.Word[6] = ntohs(0xFF00 | ntohs(virtual_addr->ipv6.u.Word[6]));
				solicited_node_multicast_address.u.Word[7] = virtual_addr->ipv6.u.Word[7];


				if (RtlCompareMemory(&ip_header->DestinationAddress, &solicited_node_multicast_address, sizeof(ip_header->DestinationAddress)) == sizeof(ip_header->DestinationAddress) ||
					RtlCompareMemory(&ip_header->DestinationAddress, &virtual_addr->ipv6, sizeof(ip_header->DestinationAddress)) == sizeof(ip_header->DestinationAddress)) {
					//如果请求多播地址是虚拟IPv6的多播地址，则禁止发送
					return_data->code = PNRC_DROP;
					KdPrint(("SYS:ICMPv6 NS PNRC_DROP\n"));
				}
			} else {
				//如果收到了别的主机邻居请求
				IN6_ADDR solicited_node_multicast_address;

				RtlZeroMemory(&solicited_node_multicast_address, sizeof(solicited_node_multicast_address));
				//计算虚拟IPv6地址的被请求-节点多播地址
				//前缀为 FF02::1:FF00:0/104
				solicited_node_multicast_address.u.Word[0] = ntohs(0xFF02);
				solicited_node_multicast_address.u.Word[5] = ntohs(0x0001);
				solicited_node_multicast_address.u.Word[6] = ntohs(0xFF00 | ntohs(virtual_addr->ipv6.u.Word[6]));
				solicited_node_multicast_address.u.Word[7] = virtual_addr->ipv6.u.Word[7];
				if (RtlCompareMemory(&ip_header->DestinationAddress, &solicited_node_multicast_address, sizeof(ip_header->DestinationAddress)) == sizeof(ip_header->DestinationAddress)||
					RtlCompareMemory(&ip_header->DestinationAddress, &virtual_addr->ipv6, sizeof(ip_header->DestinationAddress)) == sizeof(ip_header->DestinationAddress)) {
					//如果请求多播地址是虚拟IPv6的多播地址，则禁止接收
					return_data->code = PNRC_DROP;
					KdPrint(("SYS:ICMPv6 NS PNRC_DROP\n"));
				}
			}
			break;
		case 136://邻居宣告（NA）
			//如果发现有别的主机对虚拟IP进行宣告
			if (RtlCompareMemory(&ip_header->SourceAddress, &virtual_addr->ipv6, sizeof(ip_header->SourceAddress)) == sizeof(ip_header->SourceAddress)) {
				if (is_recv) {
					//发现有别的主机对虚拟IP进行宣告，则阻止此数据包传达到系统中
					return_data->code = PNRC_DROP;
					KdPrint(("SYS:ICMPv6 NA PNRC_DROP\n"));
				} else {
					//发现本机发送虚拟IP的宣告，则拦截
					return_data->code = PNRC_DROP;
					KdPrint(("SYS:ICMPv6 NA PNRC_DROP\n"));
				}
			}
			break;

		}
		break;
	}
}

VOID ProcessTCP(IN PLCXL_FILTER filter, IN BOOLEAN is_recv, IN INT lcxl_role, IN PVOID ip_header, IN INT ipMode, IN PLCXL_ADDR_INFO virtual_addr, IN OUT PPROCESS_NBL_RESULT return_data)
{
	PVOID destination_address = NULL;
	SIZE_T destination_address_len = 0;
	PVOID source_address = NULL;
	SIZE_T source_address_len = 0;

	PVOID virtual_ip = NULL;

	switch (ipMode)
	{
	case IM_IPV4:
		destination_address = &((PIPV4_HEADER)ip_header)->DestinationAddress;
		destination_address_len = sizeof(((PIPV4_HEADER)ip_header)->DestinationAddress);

		source_address = &((PIPV4_HEADER)ip_header)->SourceAddress;
		source_address_len = sizeof(((PIPV4_HEADER)ip_header)->SourceAddress);
		virtual_ip = &virtual_addr->ipv4;
		break;
	case IM_IPV6:
		destination_address = &((PIPV6_HEADER)ip_header)->DestinationAddress;
		destination_address_len = sizeof(((PIPV6_HEADER)ip_header)->DestinationAddress);

		source_address = &((PIPV6_HEADER)ip_header)->SourceAddress;
		source_address_len = sizeof(((PIPV6_HEADER)ip_header)->SourceAddress);
		virtual_ip = &virtual_addr->ipv6;
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	switch (lcxl_role) {
	case LCXL_ROLE_ROUTER://如果角色是路由

		if (is_recv) {
			//查看接收到的数据包的目标IP是否是虚拟IP
			if (RtlCompareMemory(destination_address, virtual_ip, destination_address_len) == destination_address_len) {
				// 创建路由表
				return_data->data.route = RouteTCPNBL(filter, ipMode, ip_header);
				// 如果路由表不为空
				if (return_data->data.route != NULL) {
					//设置此数据包状态为路由
					return_data->code = PNRC_ROUTER;
					//KdPrint(("SYS:TCP PNRC_ROUTER\n"));
				}
			}

		}
		break;
	case LCXL_ROLE_SERVER:
		if (!is_recv) {
			//如果要发送的数据包的源IP地址是虚拟IP地址
			if (RtlCompareMemory(source_address, virtual_ip, source_address_len) == source_address_len) {
				//更改源MAC地址为router的mac地址
				return_data->code = PNRC_MODIFY;
				//KdPrint(("SYS:TCP PNRC_MODIFY\n"));
			}
		}
		break;
	}
}

VOID ProcessUDP(IN PLCXL_FILTER filter, IN BOOLEAN is_recv, IN INT lcxl_role, IN PVOID ip_header, IN INT ipMode, IN PLCXL_ADDR_INFO virtual_addr, IN OUT PPROCESS_NBL_RESULT return_data)
{
	PVOID destination_address = NULL;
	SIZE_T destination_address_len = 0;
	PVOID source_address = NULL;
	SIZE_T source_address_len = 0;

	PVOID virtual_ip = NULL;

	UNREFERENCED_PARAMETER(filter);
	switch (ipMode)
	{
	case IM_IPV4:
		destination_address = &((PIPV4_HEADER)ip_header)->DestinationAddress;
		destination_address_len = sizeof(((PIPV4_HEADER)ip_header)->DestinationAddress);

		source_address = &((PIPV4_HEADER)ip_header)->SourceAddress;
		source_address_len = sizeof(((PIPV4_HEADER)ip_header)->SourceAddress);
		virtual_ip = &virtual_addr->ipv4;
		break;
	case IM_IPV6:
		destination_address = &((PIPV6_HEADER)ip_header)->DestinationAddress;
		destination_address_len = sizeof(((PIPV6_HEADER)ip_header)->DestinationAddress);

		source_address = &((PIPV6_HEADER)ip_header)->SourceAddress;
		source_address_len = sizeof(((PIPV6_HEADER)ip_header)->SourceAddress);
		virtual_ip = &virtual_addr->ipv6;
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	switch (lcxl_role) {
	case LCXL_ROLE_ROUTER://如果角色是路由
		break;
	case LCXL_ROLE_SERVER:
		if (!is_recv) {
			//如果要发送的数据包的源IP地址是虚拟IP地址
			if (RtlCompareMemory(source_address, virtual_ip, source_address_len) == source_address_len) {
				//更改源MAC地址为router的mac地址
				return_data->code = PNRC_DROP;
				KdPrint(("SYS:send UDP PNRC_DROP\n"));
			}
		} else {
			//如果要接受的数据包的目标IP地址是虚拟IP地址
			if (RtlCompareMemory(destination_address, virtual_ip, destination_address_len) == destination_address_len) {
				//更改源MAC地址为router的mac地址
				return_data->code = PNRC_DROP;
				KdPrint(("SYS:recv UDP PNRC_DROP\n"));
			}
		}
		break;
	}
}

PLCXL_FILTER FindFilter(IN NET_LUID miniport_net_luid)
{
	PLCXL_FILTER filter = NULL;
	BOOLEAN bFound = FALSE;
	
	ASSERT(g_filter_list.lock_count > 0);
	filter = CONTAINING_RECORD(GetListofLCXLLockList(&g_filter_list)->Flink, LCXL_FILTER, filter_module_link);
	while (&filter->filter_module_link != GetListofLCXLLockList(&g_filter_list)) {
		if (miniport_net_luid.Value == filter->module.miniport_net_luid.Value) {
			
			bFound = TRUE;
			break;
		}

		filter = CONTAINING_RECORD(filter->filter_module_link.Flink, LCXL_FILTER, filter_module_link);
	}
	if (!bFound) {
		filter = NULL;
	}
	return filter;
}

void DelLCXLFilterCallBack(PLIST_ENTRY filter)
{
	//
	// Free the memory allocated
	FILTER_FREE_MEM(CONTAINING_RECORD(filter, LCXL_FILTER, filter_module_link));
}




