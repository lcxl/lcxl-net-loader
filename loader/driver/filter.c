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

		FILTER_INIT_LOCK(&g_setting.lock);
		InitLCXLLockList(&g_setting.module_list, DelModuleSettingCallBack);
        Status = NdisFRegisterFilterDriver(DriverObject,
                                           (NDIS_HANDLE)g_filter_driver_object,
                                           &FChars,
                                           &g_filter_driver_handle);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, "Register filter driver failed.\n");

			DelLCXLLockList(&g_setting.module_list);
			DelLCXLLockList(&g_filter_list);
            break;
        }

        Status = FilterRegisterDevice();

        if (Status != NDIS_STATUS_SUCCESS)
        {
            NdisFDeregisterFilterDriver(g_filter_driver_handle);
            
			DelLCXLLockList(&g_setting.module_list);
			DelLCXLLockList(&g_filter_list);
            DEBUGP(DL_WARN, "Register device for the filter driver failed.\n");
            break;
        }


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
		pFilter->attach_paramters = AttachParameters;
        
        pFilter->miniport_if_index = AttachParameters->BaseMiniportIfIndex;
        //添加代码
		pFilter->miniport_net_luid = AttachParameters->BaseMiniportNetLuid;
        //保存MAC地址（一个问题，当用户手动修改了MAC地址，会怎样- -）
		pFilter->mac_addr.Length = sizeof(pFilter->mac_addr.Address)<AttachParameters->MacAddressLength ? sizeof(pFilter->mac_addr.Address) : AttachParameters->MacAddressLength;

		NdisMoveMemory(pFilter->mac_addr.Address, AttachParameters->CurrentMacAddress, pFilter->mac_addr.Length);
		//初始化服务器列表
		//InitializeListHead(&pFilter->server_list.list_entry);
		//初始化路由列表
		InitializeListHead(&pFilter->route_list);
        //初始化NBL发送池
        PoolParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        PoolParameters.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
		PoolParameters.Header.Size = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        PoolParameters.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT;
        //PoolParameters.ContextSize = sizeof(NPROT_SEND_NETBUFLIST_RSVD);
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


        pFilter->state = FilterPaused;
		//加载配置文件
		pFilter->module_setting = LoadModuleSetting(AttachParameters);
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
    PLCXL_FILTER          pFilter = (PLCXL_FILTER)(FilterModuleContext);
    NDIS_STATUS         Status;
    BOOLEAN               bFalse = FALSE;

    UNREFERENCED_PARAMETER(PauseParameters);

    DEBUGP(DL_TRACE, "===>NDISLWF FilterPause: FilterInstance %p\n", FilterModuleContext);
	KdPrint(("SYS:FilterPause:miniport:%I64u %wZ %wZ %wZ\n", pFilter->miniport_net_luid.Value, pFilter->attach_paramters->FilterModuleGuidName, pFilter->attach_paramters->BaseMiniportInstanceName, pFilter->attach_paramters->BaseMiniportName));

    //
    // Set the flag that the filter is going to pause
    //
    FILTER_ASSERT(pFilter->state == FilterRunning);

    FILTER_ACQUIRE_LOCK(&pFilter->lock, bFalse);
    pFilter->state = FilterPausing;
    FILTER_RELEASE_LOCK(&pFilter->lock, bFalse);

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
	KdPrint(("SYS:FilterRestart:miniport:%I64u %wZ %wZ %wZ\n", pFilter->miniport_net_luid.Value, pFilter->attach_paramters->FilterModuleGuidName, pFilter->attach_paramters->BaseMiniportInstanceName, pFilter->attach_paramters->BaseMiniportName));

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

    DEBUGP(DL_TRACE, "===>FilterDetach:    FilterInstance %p\n", FilterModuleContext);

	KdPrint(("SYS:FilterDetach:miniport:%I64u %wZ %wZ %wZ\n", pFilter->miniport_net_luid.Value, pFilter->attach_paramters->FilterModuleGuidName, pFilter->attach_paramters->BaseMiniportInstanceName, pFilter->attach_paramters->BaseMiniportName));

	//
    // Filter must be in paused state
    //
    FILTER_ASSERT(pFilter->state == FilterPaused);


    //
    // Detach must not fail, so do not put any code here that can possibly fail.
    //

	pFilter->module_setting->ref_count--;

    //添加代码
    if (pFilter->send_net_buffer_list_pool!=NULL) {
        NdisFreeNetBufferListPool(pFilter->send_net_buffer_list_pool);
	}
	
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
    PLCXL_FILTER                          pFilter = (PLCXL_FILTER)FilterModuleContext;
    PNDIS_OID_REQUEST                   Request = NULL;
    PFILTER_REQUEST_CONTEXT             Context;
    PNDIS_OID_REQUEST                   OriginalRequest = NULL;
    BOOLEAN                             bFalse = FALSE;

    FILTER_ACQUIRE_LOCK(&pFilter->lock, bFalse);

    Request = pFilter->pending_oid_request;

    if (Request != NULL)
    {
        Context = (PFILTER_REQUEST_CONTEXT)(&Request->SourceReserved[0]);

        OriginalRequest = (*Context);
    }

    if ((OriginalRequest != NULL) && (OriginalRequest->RequestId == RequestId))
    {
        FILTER_RELEASE_LOCK(&pFilter->lock, bFalse);

        NdisFCancelOidRequest(pFilter->filter_handle, RequestId);
    }
    else
    {
        FILTER_RELEASE_LOCK(&pFilter->lock, bFalse);
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


    FILTER_ACQUIRE_LOCK(&pFilter->lock, bFalse);

    ASSERT(pFilter->pending_oid_request == Request);
    pFilter->pending_oid_request = NULL;

    FILTER_RELEASE_LOCK(&pFilter->lock, bFalse);


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
    FILTER_ACQUIRE_LOCK(&pFilter->lock, bFalse);
    ASSERT(pFilter->bIndicating == FALSE);
    pFilter->bIndicating = TRUE;
    FILTER_RELEASE_LOCK(&pFilter->lock, bFalse);
#endif // DBG

    NdisFIndicateStatus(pFilter->filter_handle, StatusIndication);

#if DBG
    FILTER_ACQUIRE_LOCK(&pFilter->lock, bFalse);
    ASSERT(pFilter->bIndicating == TRUE);
    pFilter->bIndicating = FALSE;
    FILTER_RELEASE_LOCK(&pFilter->lock, bFalse);
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

    if (pFilter->track_sends)
    {
        CurrNbl = NetBufferLists;
        while (CurrNbl)
        {
            NumOfSendCompletes++;
            CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);

        }
        DispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendCompleteFlags);
        FILTER_ACQUIRE_LOCK(&pFilter->lock, DispatchLevel);
        pFilter->outstanding_sends -= NumOfSendCompletes;
        FILTER_LOG_SEND_REF(2, pFilter, PrevNbl, pFilter->outstanding_sends);
        FILTER_RELEASE_LOCK(&pFilter->lock, DispatchLevel);
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

        FILTER_ACQUIRE_LOCK(&pFilter->lock, DispatchLevel);
        //
        // If the filter is not in running state, fail the send
        //
        if (pFilter->state != FilterRunning)
        {
            FILTER_RELEASE_LOCK(&pFilter->lock, DispatchLevel);

            CurrNbl = NetBufferLists;
            while (CurrNbl)
            {
                NET_BUFFER_LIST_STATUS(CurrNbl) = NDIS_STATUS_PAUSED;
                CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
            }
            NdisFSendNetBufferListsComplete(pFilter->filter_handle,
                        NetBufferLists,
                        DispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0);
            break;

        }
        FILTER_RELEASE_LOCK(&pFilter->lock, DispatchLevel);
#endif
        if (pFilter->track_sends)
        {
            FILTER_ACQUIRE_LOCK(&pFilter->lock, DispatchLevel);
            CurrNbl = NetBufferLists;
            while (CurrNbl)
            {
                pFilter->outstanding_sends++;
                FILTER_LOG_SEND_REF(1, pFilter, CurrNbl, pFilter->outstanding_sends);

                CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
            }
            FILTER_RELEASE_LOCK(&pFilter->lock, DispatchLevel);
        }
        
        //
        // If necessary, queue the NetBufferLists in a local structure for later
        // processing.  However, do not queue them for "too long", or else the
        // system's performance may be degraded.  If you need to hold onto an
        // NBL for an unbounded amount of time, then allocate memory, perform a
        // deep copy, and complete the original NBL.
        //
        
        NdisFSendNetBufferLists(pFilter->filter_handle, NetBufferLists, PortNumber, SendFlags);


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
        DispatchLevel = NDIS_TEST_RETURN_AT_DISPATCH_LEVEL(ReturnFlags);
        FILTER_ACQUIRE_LOCK(&pFilter->lock, DispatchLevel);

        pFilter->outstanding_rcvs -= NumOfNetBufferLists;
        Ref = pFilter->outstanding_rcvs;
        FILTER_LOG_RCV_REF(3, pFilter, NetBufferLists, Ref);
        FILTER_RELEASE_LOCK(&pFilter->lock, DispatchLevel);
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
	PNET_BUFFER_LIST    next_nbl;
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
    PNET_BUFFER_LIST    send_nbl_list = NULL;
    //!添加代码!


    DEBUGP(DL_TRACE, "===>ReceiveNetBufferList: NetBufferLists = %p.\n", NetBufferLists);
	dispatch_level = NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags);
	if (dispatch_level) {
        NDIS_SET_RETURN_FLAG(return_flags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
        NDIS_SET_SEND_FLAG(send_flags, NDIS_SEND_FLAGS_DISPATCH_LEVEL);
    }

	FILTER_ACQUIRE_LOCK(&filter->lock, dispatch_level);
	//如果没有在运行或者模块信息没有加载
	if (filter->state != FilterRunning) {
		FILTER_RELEASE_LOCK(&filter->lock, dispatch_level);

		if (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags)) {
			NdisFReturnNetBufferLists(filter->filter_handle, NetBufferLists, return_flags);
		}
		return;
	}
	FILTER_RELEASE_LOCK(&filter->lock, dispatch_level);
	ASSERT(filter->module_setting != NULL);
	//如果没有加载配置信息，则放过所有的NBL
	if ((filter->module_setting->flag & MSF_ENABLED) == 0) {
		NdisFIndicateReceiveNetBufferLists(filter->filter_handle, NetBufferLists, PortNumber, NumberOfNetBufferLists, ReceiveFlags);
		return;
	}

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
	next_nbl = NetBufferLists;
	while (next_nbl != NULL) {
		PMDL                current_mdl = NULL;
		UINT                buffer_length;
		ULONG               total_length;
		ULONG               offset;
		PETHERNET_HEADER    ethernet_header = NULL;
		//是否此数据包已处理
		BOOLEAN				is_processed_nbl = FALSE;

		current_mdl = NET_BUFFER_CURRENT_MDL(NET_BUFFER_LIST_FIRST_NB(next_nbl));
		total_length = NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB(next_nbl));
		offset = NET_BUFFER_CURRENT_MDL_OFFSET(NET_BUFFER_LIST_FIRST_NB(next_nbl));

		ASSERT(current_mdl != NULL);
		NdisQueryMdl(
			current_mdl,
			&ethernet_header,
			&buffer_length,
			NormalPagePriority);
		//各种有效性判断
		if (ethernet_header != NULL && buffer_length != 0) {
			PLCXL_ROUTE_LIST_ENTRY route = NULL;

			ASSERT(buffer_length > offset);
			//获取真正的的包数据
			buffer_length -= offset;
			//获取帧数据头
			ethernet_header = (PETHERNET_HEADER)((PUCHAR)ethernet_header + offset);

			is_processed_nbl = ProcessNBL(filter, ethernet_header, buffer_length, &route);
			//如果此数据包已处理，则不需要提交到上层驱动中
			if (is_processed_nbl) {
				PETHERNET_HEADER    send_buffer;
				PMDL                send_mdl = NULL;
				PNET_BUFFER_LIST    send_nbl;
				ULONG               bytes_copied;
				
				//是否可以Pend
				if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags)) {
					//不作处理

				} else {
					//虚拟IP的数据包不能传到上层驱动，需要丢弃
					if (drop_nbl_head == NULL) {
						drop_nbl_head = next_nbl;
						drop_nbl_tail = next_nbl;
					} else {
						NET_BUFFER_LIST_NEXT_NBL(drop_nbl_tail) = next_nbl;
					}
				}
				//如果需要路由此数据包
				if (route != NULL) {
					//创建一个NBL
					send_buffer = (PETHERNET_HEADER)FILTER_ALLOC_MEM(filter->filter_handle, buffer_length);
					send_mdl = NdisAllocateMdl(filter->filter_handle, send_buffer, buffer_length);
					send_nbl = NdisAllocateNetBufferAndNetBufferList(
						filter->send_net_buffer_list_pool,
						0,                              // ContextSize
						0,                              // ContextBackfill
						send_mdl,                           // MdlChain
						0,                              // DataOffset
						buffer_length);                   // DataLength
					NdisCopyFromNetBufferToNetBuffer(
						NET_BUFFER_LIST_FIRST_NB(send_nbl),
						0,
						buffer_length,
						NET_BUFFER_LIST_FIRST_NB(next_nbl),
						0,
						&bytes_copied);
					//设置SourceHandle
					//A filter driver must set the SourceHandle member of each NET_BUFFER_LIST structure that it originates to the same value that it passes to the NdisFilterHandle parameter

					send_nbl->SourceHandle = filter->filter_handle;
					//修改目标MAC地址
					RtlCopyMemory(&send_buffer->Destination, route->dst_server->info.addr.mac_addr.Address, sizeof(ethernet_header->Destination));
					//
					// The other members of NET_BUFFER_DATA structure are already initialized properly during allocation.
					//
					NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB(send_nbl)) = bytes_copied;
					//插入到转发队列中
					if (send_nbl_list == NULL) {
						send_nbl_list = send_nbl;
					} else {
						NET_BUFFER_LIST_NEXT_NBL(send_nbl_list) = send_nbl;
					}
					if (route->status == RS_CLOSED) {
						DeleteRouteListEntry(route, &filter->module_setting->server_list);
					}
				}
			} else {

			}

		} else {
			//如果无法获取到数据包信息，则

		}
		//如果数据包未被处理，则发送给上层驱动
		if (!is_processed_nbl) {

			if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags)) {
				PNET_BUFFER_LIST tmpNBL;

				tmpNBL = NET_BUFFER_LIST_NEXT_NBL(next_nbl);
				NET_BUFFER_LIST_NEXT_NBL(next_nbl) = NULL;
				//接受不丢弃的数据包
				NdisFIndicateReceiveNetBufferLists(filter->filter_handle, next_nbl, PortNumber, 1, ReceiveFlags | NDIS_RECEIVE_FLAGS_RESOURCES);
				NET_BUFFER_LIST_NEXT_NBL(next_nbl) = tmpNBL;
			} else {
				if (pass_nbl_head == NULL) {
					pass_nbl_head = next_nbl;
					pass_nbl_tail = next_nbl;
				} else {
					NET_BUFFER_LIST_NEXT_NBL(pass_nbl_tail) = next_nbl;
				}
				number_of_pass_nbl++;
			}
		}
		//获取下一个NBL
		next_nbl = NET_BUFFER_LIST_NEXT_NBL(next_nbl);
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
	if (NULL != send_nbl_list) {
		NdisFSendNetBufferLists(filter->filter_handle, send_nbl_list, PortNumber, send_flags);
	}
	//
	if (filter->track_receives)
	{
		FILTER_ACQUIRE_LOCK(&filter->lock, dispatch_level);
		filter->outstanding_rcvs += NumberOfNetBufferLists;
		rcv_ref = filter->outstanding_rcvs;

		FILTER_LOG_RCV_REF(1, filter, NetBufferLists, rcv_ref);
		FILTER_RELEASE_LOCK(&filter->lock, dispatch_level);
	}
	if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags) &&
		filter->track_receives)
	{
		FILTER_ACQUIRE_LOCK(&filter->lock, dispatch_level);
		filter->outstanding_rcvs -= NumberOfNetBufferLists;
		rcv_ref = filter->outstanding_rcvs;
		FILTER_LOG_RCV_REF(2, filter, NetBufferLists, rcv_ref);
		FILTER_RELEASE_LOCK(&filter->lock, dispatch_level);
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
	LoadSetting();
	UpdateFiltersSetting();
	KdPrint(("SYS:DriverReinitialize end\n"));
}

PLCXL_ROUTE_LIST_ENTRY RouteTCPNBL(IN PLCXL_FILTER pFilter, IN INT ipMode, IN PVOID pIPHeader) 
{
	PTCP_HDR ptcp_header = NULL;
	PLCXL_ROUTE_LIST_ENTRY route_info;

	switch (ipMode) {
	case IM_IPV4:
		//ptcp_header = (PTCP_HDR)((PUCHAR)pIPHeader + sizeof(IPV4_HEADER));
		ptcp_header = (PTCP_HDR)((PUCHAR)pIPHeader + Ip4HeaderLengthInBytes((PIPV4_HEADER)pIPHeader));
	case IM_IPV6:
		ptcp_header = (PTCP_HDR)((PUCHAR)pIPHeader + sizeof(IPV6_HEADER));
	default:
		ASSERT(FALSE);
		break;
	}
	route_info = GetRouteListEntry(&pFilter->route_list, ipMode, pIPHeader, ptcp_header);
	//建立连接的阶段
	//有TH_SYN的阶段是建立连接的阶段，这个时候就得选择路由信息
	if ((ptcp_header->th_flags & TH_SYN) != 0) {
		PSERVER_INFO_LIST_ENTRY server;

		//选择一个服务器
		server = SelectBestServer(&pFilter->module_setting->server_list, ipMode, pIPHeader, ptcp_header);
		if (server == NULL) {
			return NULL;
		}
		if (route_info == NULL) {
			route_info = CreateRouteListEntry(&pFilter->route_list);
		} else {
			DecRefListEntry(&pFilter->module_setting->server_list, &route_info->dst_server->list_entry);
		}
		//初始化路由信息
		InitRouteListEntry(route_info, ipMode, pIPHeader, ptcp_header, server);
	}
	else {
		if (route_info != NULL) {
			//如果客户端发出ACK包并且连接处于LAST_ACK状态，则更改状态为CLOSE
			if ((ptcp_header->th_flags & TH_ACK) != 0 && (route_info->status == RS_LAST_ACK)) {
				route_info->status = RS_CLOSED;
			}
			else if ((ptcp_header->th_flags & TH_FIN) != 0) {
				//如果客户端通知连接要关闭
				//更改路由状态为LAST_ACK
				route_info->status = RS_LAST_ACK;
			}
			else if ((ptcp_header->th_flags & TH_RST) != 0) {
				//如果连接重置，直接关闭连接
				route_info->status = RS_CLOSED;
			}
		}
	}

	return route_info;
}

BOOLEAN ProcessNBL(IN PLCXL_FILTER pFilter, IN PETHERNET_HEADER pEthHeader, IN UINT BufferLength, OUT PLCXL_ROUTE_LIST_ENTRY *route)
{
    USHORT UNALIGNED *ethernet_type_ptr = NULL;
	USHORT ethernet_type;
	BOOLEAN is_processed = FALSE;

	ASSERT(route != NULL && pFilter != NULL && pEthHeader != NULL);
	*route = NULL;
    if (BufferLength < sizeof(ETHERNET_HEADER)) {
        return FALSE;
    }
    ethernet_type = ntohs(pEthHeader->Type);
    //判断帧类型是不是8021P_TAG
    if (ethernet_type == ETHERNET_TYPE_802_1Q) {
        if (BufferLength >= sizeof(ETHERNET_HEADER)+4) {
            ethernet_type_ptr = (USHORT UNALIGNED *)((PUCHAR)&pEthHeader->Type + 4);
            BufferLength -= sizeof(ETHERNET_HEADER)+4;
         } else {
            //缺代码
			 return FALSE;
         }
    } else {
         ethernet_type_ptr = &pEthHeader->Type;
		 ethernet_type = ntohs(pEthHeader->Type);
         BufferLength -= sizeof(ETHERNET_HEADER);
    }
    if (ethernet_type_ptr == NULL) {
		return FALSE;
    }

	switch (ethernet_type) {
	case ETHERNET_TYPE_ARP:
		//ARP_OPCODE  ARP_REQUEST  ARP_RESPONSE
		if (BufferLength >= sizeof(ARP_HEADER)) {
			PARP_HEADER arp_header;
			LCXL_ARP_ETHERNET lcxl_arp_ethernet;

			arp_header = (PARP_HEADER)((PUCHAR)ethernet_type_ptr + sizeof(USHORT));
			LCXLReadARPEthernet(arp_header, &lcxl_arp_ethernet);

			KdPrint(("SYS:ARP:HardwareAddressSpace=%d, ProtocolAddressSpace=%d, Opcode=%d, SenderHardwareAddress=%02x:%02x:%02x:%02x:%02x:%02x, SenderProtocolAddress=%d.%d.%d.%d, SenderHardwareAddress=%02x:%02x:%02x:%02x:%02x:%02x, SenderProtocolAddress=%d.%d.%d.%d\n",
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

			if (lcxl_arp_ethernet.ProtocolAddressSpace != ETHERNET_TYPE_IPV4) {
				return FALSE;
			}

			switch (lcxl_arp_ethernet.Opcode)
			{
			case ARP_REQUEST:

				break;
			case ARP_RESPONSE:

				break;
			default:
				break;
			}
		}
		break;
	case ETHERNET_TYPE_IPV4://IPv4协议
		if (BufferLength >= sizeof(IPV4_HEADER)){
			PIPV4_HEADER ip_header;

			ip_header = (PIPV4_HEADER)((PUCHAR)ethernet_type_ptr + sizeof(USHORT));
			//查看数据包的目标IP是否是虚拟IP
			if (RtlCompareMemory(&ip_header->DestinationAddress, &pFilter->module_setting->virtual_ipv4, sizeof(ip_header->DestinationAddress)) == sizeof(ip_header->DestinationAddress)) {
				//pIPHeader->iaDst

				switch (ip_header->Protocol) {
				case 0x01://ICMP 

					break;
				case 0x06://TCP
					//目前仅支持TCP
					*route = RouteTCPNBL(pFilter, IM_IPV4, ip_header);
					is_processed = *route != NULL;
					break;
				case 0x11://PROT_UDP
					break;
				default:
					break;
				}
			}
		}
		break;

	case ETHERNET_TYPE_IPV6://IPv6协议
		if (BufferLength >= sizeof(IPV6_HEADER)){
			PIPV6_HEADER ip_header;

			ip_header = (PIPV6_HEADER)((PUCHAR)ethernet_type_ptr + sizeof(USHORT));
			//查看数据包的目标IP是否是虚拟IP
			if (RtlCompareMemory(&ip_header->DestinationAddress, &pFilter->module_setting->virtual_ipv6, sizeof(ip_header->DestinationAddress)) == sizeof(ip_header->DestinationAddress)) {
				switch (ip_header->NextHeader) {
				case 0x3A://ICMPv6
				{
					PICMPV6_MESSAGE pICMPv6Message = (PICMPV6_MESSAGE)((PUCHAR)ip_header + sizeof(IPV6_HEADER));
					KdPrint(("SYS:ICMPv6:Type=%d, Code=%d, Checksum=%d\n", pICMPv6Message->Header.Type, pICMPv6Message->Header.Code, pICMPv6Message->Header.Checksum));
				}
					break;
				case 0x06://TCP
					//目前仅支持TCP
					*route = RouteTCPNBL(pFilter, IM_IPV6, ip_header);
					is_processed = *route != NULL;
					break;
				case 0x11://PROT_UDP
					break;
				}
			}
		}
		break;
	default:
		break;
	}
	return is_processed;
}

PLCXL_FILTER FindFilter(IN NET_LUID miniport_net_luid)
{
	PLCXL_FILTER filter = NULL;
	BOOLEAN bFound = FALSE;
	
	ASSERT(g_filter_list.lock_count > 0);
	filter = CONTAINING_RECORD(GetListofLCXLLockList(&g_filter_list)->Flink, LCXL_FILTER, filter_module_link);
	while (&filter->filter_module_link != GetListofLCXLLockList(&g_filter_list)) {
		if (miniport_net_luid.Value == filter->miniport_net_luid.Value) {
			
			bFound = TRUE;
			break;
		}

		filter = CONTAINING_RECORD(filter->filter_module_link.Flink, LCXL_FILTER, filter_module_link);
	}
	if (!bFound) {
		UnlockLCXLLockList(&g_filter_list);
		filter = NULL;
	}
	return filter;
}

void UpdateFiltersSetting()
{
	PLCXL_FILTER filter;

	LockLCXLLockList(&g_filter_list);
	filter = CONTAINING_RECORD(GetListofLCXLLockList(&g_filter_list)->Flink, LCXL_FILTER, filter_module_link);
	while (&filter->filter_module_link != GetListofLCXLLockList(&g_filter_list)) {
		PLCXL_MODULE_SETTING_LIST_ENTRY module_setting;

		LockLCXLLockList(&g_setting.module_list);
		module_setting = CONTAINING_RECORD(GetListofLCXLLockList(&g_setting.module_list)->Flink, LCXL_MODULE_SETTING_LIST_ENTRY, list_entry);
		while (&module_setting->list_entry != GetListofLCXLLockList(&g_setting.module_list)) {
			if (filter->module_setting == NULL && module_setting->miniport_net_luid.Value == filter->miniport_net_luid.Value) {
				filter->module_setting = module_setting;
				break;
			}
			module_setting = CONTAINING_RECORD(module_setting->list_entry.Flink, LCXL_MODULE_SETTING_LIST_ENTRY, list_entry);
		}
		UnlockLCXLLockList(&g_setting.module_list);
		filter = CONTAINING_RECORD(filter->filter_module_link.Flink, LCXL_FILTER, filter_module_link);
	}
	UnlockLCXLLockList(&g_filter_list);
}

void DelLCXLFilterCallBack(PLIST_ENTRY filter)
{
	//
	// Free the memory allocated
	FILTER_FREE_MEM(CONTAINING_RECORD(filter, LCXL_FILTER, filter_module_link));
}




