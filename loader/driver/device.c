/*++
 *
 * The file contains the routines to create a device and handle ioctls
 *
-- */

#include "precomp.h"
#include "filter.h"//filter.c

#pragma NDIS_INIT_FUNCTION(FilterRegisterDevice)


_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS
FilterRegisterDevice(
    VOID
    )
{
    NDIS_STATUS            Status = NDIS_STATUS_SUCCESS;
    UNICODE_STRING         DeviceName;
    UNICODE_STRING         DeviceLinkUnicodeString;
    PDRIVER_DISPATCH       DispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1];
    NDIS_DEVICE_OBJECT_ATTRIBUTES   DeviceAttribute;
    PFILTER_DEVICE_EXTENSION        FilterDeviceExtension;

    DEBUGP(DL_TRACE, "==>FilterRegisterDevice\n");

    NdisZeroMemory(DispatchTable, (IRP_MJ_MAXIMUM_FUNCTION+1) * sizeof(PDRIVER_DISPATCH));

    DispatchTable[IRP_MJ_CREATE] = FilterDispatch;
    DispatchTable[IRP_MJ_CLEANUP] = FilterDispatch;
    DispatchTable[IRP_MJ_CLOSE] = FilterDispatch;
    DispatchTable[IRP_MJ_DEVICE_CONTROL] = FilterDeviceIoControl;


    NdisInitUnicodeString(&DeviceName, NTDEVICE_STRING);
    NdisInitUnicodeString(&DeviceLinkUnicodeString, LINKNAME_STRING);

    //
    // Create a device object and register our dispatch handlers
    //
    NdisZeroMemory(&DeviceAttribute, sizeof(NDIS_DEVICE_OBJECT_ATTRIBUTES));

    DeviceAttribute.Header.Type = NDIS_OBJECT_TYPE_DEVICE_OBJECT_ATTRIBUTES;
    DeviceAttribute.Header.Revision = NDIS_DEVICE_OBJECT_ATTRIBUTES_REVISION_1;
    DeviceAttribute.Header.Size = sizeof(NDIS_DEVICE_OBJECT_ATTRIBUTES);

    DeviceAttribute.DeviceName = &DeviceName;
    DeviceAttribute.SymbolicName = &DeviceLinkUnicodeString;
    DeviceAttribute.MajorFunctions = &DispatchTable[0];
    DeviceAttribute.ExtensionSize = sizeof(FILTER_DEVICE_EXTENSION);

    Status = NdisRegisterDeviceEx(
                g_filter_driver_handle,
                &DeviceAttribute,
                &g_device_object,
                &g_ndis_filter_device_handle
                );


    if (Status == NDIS_STATUS_SUCCESS)
    {
        FilterDeviceExtension = NdisGetDeviceReservedExtension(g_device_object);

        FilterDeviceExtension->Signature = 'FTDR';
        FilterDeviceExtension->Handle = g_filter_driver_handle;
    }


    DEBUGP(DL_TRACE, "<==FilterRegisterDevice: %x\n", Status);

    return (Status);

}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
FilterDeregisterDevice(
    VOID
    )

{
    if (g_ndis_filter_device_handle != NULL)
    {
        NdisDeregisterDeviceEx(g_ndis_filter_device_handle);
    }

    g_ndis_filter_device_handle = NULL;

}

_Use_decl_annotations_
NTSTATUS
FilterDispatch(
    PDEVICE_OBJECT       DeviceObject,
    PIRP                 Irp
    )
{
    PIO_STACK_LOCATION       IrpStack;
    NTSTATUS                 Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(DeviceObject);

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            break;

        case IRP_MJ_CLEANUP:
            break;

        case IRP_MJ_CLOSE:
            break;

        default:
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

_Use_decl_annotations_
NTSTATUS
FilterDeviceIoControl(
    PDEVICE_OBJECT        DeviceObject,
    PIRP                  Irp
    )
{
    PIO_STACK_LOCATION          IrpSp;
    NTSTATUS                    Status = STATUS_SUCCESS;
    PFILTER_DEVICE_EXTENSION    FilterDeviceExtension;
    PUCHAR                      InputBuffer;
    PUCHAR                      OutputBuffer;
    ULONG                       InputBufferLength, OutputBufferLength;
    PLIST_ENTRY                 Link;
    PUCHAR                      pInfo;
    ULONG                       InfoLength = 0;
    PLCXL_FILTER                pFilter = NULL;
    BOOLEAN                     bFalse = FALSE;


    UNREFERENCED_PARAMETER(DeviceObject);


    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (IrpSp->FileObject == NULL) {
        return(STATUS_UNSUCCESSFUL);
    }


    FilterDeviceExtension = (PFILTER_DEVICE_EXTENSION)NdisGetDeviceReservedExtension(DeviceObject);

    ASSERT(FilterDeviceExtension->Signature == 'FTDR');

    Irp->IoStatus.Information = 0;

	InputBuffer = OutputBuffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
	InputBufferLength = OutputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_FILTER_RESTART_ALL:
            break;

        case IOCTL_FILTER_RESTART_ONE_INSTANCE:
            pFilter = filterFindFilterModule (InputBuffer, InputBufferLength);

            if (pFilter == NULL) {

                break;
            }
            NdisFRestartFilter(pFilter->FilterHandle);
            break;

        case IOCTL_FILTER_ENUERATE_ALL_INSTANCES:

            pInfo = OutputBuffer;
            FILTER_ACQUIRE_LOCK(&g_filter_list_lock, bFalse);
            Link = g_filter_list.Flink;
			//遍历列表
            while (Link != &g_filter_list) {
                pFilter = CONTAINING_RECORD(Link, LCXL_FILTER, FilterModuleLink);

                InfoLength += (pFilter->module_setting->filter_module_name->Length + sizeof(USHORT));

                if (InfoLength <= OutputBufferLength) {
					*(PUSHORT)pInfo = pFilter->module_setting->filter_module_name->Length;
                    NdisMoveMemory(pInfo + sizeof(USHORT),
						(PUCHAR)(pFilter->module_setting->filter_module_name->Buffer),
						pFilter->module_setting->filter_module_name->Length);

					pInfo += (pFilter->module_setting->filter_module_name->Length + sizeof(USHORT));
                }
                Link = Link->Flink;
            }

            FILTER_RELEASE_LOCK(&g_filter_list_lock, bFalse);
            if (InfoLength <= OutputBufferLength) {
                Status = NDIS_STATUS_SUCCESS;
            }
            //
            // Buffer is small
            //
            else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
		//添加代码
		case IOCTL_LOADER_ALL_APP_MODULE:
		{
			PAPP_MODULE_INFO cur_buf;

			cur_buf = (PAPP_MODULE_INFO)OutputBuffer;

			PLCXL_MODULE_SETTING_LIST_ENTRY module;

			FILTER_ACQUIRE_LOCK(&g_Setting.module_list_lock, bFalse);
			module = &g_Setting.module_list;
			while (module = CONTAINING_RECORD(module->list_entry.Flink, LCXL_MODULE_SETTING_LIST_ENTRY, list_entry), module != &g_Setting.module_list) {
				//先判断缓冲区是否足够
				if (OutputBufferLength - (ULONG)((LONG_PTR)cur_buf - (LONG_PTR)OutputBuffer) < sizeof(APP_MODULE_INFO)) {
					Status = STATUS_BUFFER_TOO_SMALL;
					break;
				}
				cur_buf->app_module_status = module->ref_count == 0 ? AMS_NO_FILTER : AMS_NORMAL;
				FILTER_ACQUIRE_LOCK(&module->module_setting_lock, bFalse);
				cur_buf->real_addr = module->real_addr;
				cur_buf->virtual_ipv4 = module->virtual_ipv4;
				cur_buf->virtual_ipv6 = module->virtual_ipv6;
				cur_buf->server_count = module->server_count;
				FILTER_RELEASE_LOCK(&module->module_setting_lock, bFalse);
				cur_buf->miniport_net_luid = module->miniport_net_luid;

				cur_buf->filter_module_name_len = (sizeof(cur_buf->filter_module_name) < module->filter_module_name->Length) ? sizeof(cur_buf->filter_module_name) : module->filter_module_name->Length;
				RtlCopyMemory(cur_buf->filter_module_name, module->filter_module_name->Buffer, cur_buf->filter_module_name_len);

				cur_buf->miniport_friendly_name_len = (sizeof(cur_buf->miniport_friendly_name) < module->miniport_friendly_name->Length) ? sizeof(cur_buf->miniport_friendly_name) : module->miniport_friendly_name->Length;
				RtlCopyMemory(cur_buf->miniport_friendly_name, module->miniport_friendly_name->Buffer, cur_buf->miniport_friendly_name_len);

				cur_buf->miniport_name_len = (sizeof(cur_buf->miniport_name) < module->miniport_name->Length) ? sizeof(cur_buf->miniport_name) : module->miniport_name->Length;
				RtlCopyMemory(cur_buf->miniport_name, module->miniport_name->Buffer, cur_buf->miniport_name_len);

				cur_buf++;
			}
			FILTER_RELEASE_LOCK(&g_Setting.module_list_lock, bFalse);
			InfoLength = (ULONG)((ULONG_PTR)cur_buf - (ULONG_PTR)OutputBuffer);
		}
			break;
		case IOCTL_LOADER_GET_VIRTUAL_IP:
			
			break;
		case IOCTL_LOADER_SET_VIRTUAL_IP:
			break;
		case IOCTL_LOADER_GET_SERVER_LIST:
			if (InputBufferLength == sizeof(NET_LUID)) {
				PLCXL_FILTER pFilter = FindAndLockFilter(*(PNET_LUID)InputBuffer);
				if (pFilter != NULL) {
					if (pFilter->module_setting != NULL && (pFilter->module_setting->flag & ML_DELETE_AFTER_RESTART) == 0) {
						
					} else {
						Status = STATUS_NOT_FOUND;
					}
					UnlockFilter(pFilter);
				} else {
					Status = STATUS_NOT_FOUND;
				}
			} else {
				Status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_LOADER_ADD_SERVER:
			if (sizeof(APP_ADD_SERVER) == InputBufferLength) {
				PAPP_ADD_SERVER app_add_server = (PAPP_ADD_SERVER)InputBuffer;
				
				PLCXL_FILTER pFilter = FindAndLockFilter(app_add_server->miniport_net_luid);
				if (pFilter != NULL) {

					UnlockFilter(pFilter);
				}
			} else {
				Status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_LOADER_DEL_SERVER:
			if (sizeof(APP_DEL_SERVER) == InputBufferLength) {
				PAPP_DEL_SERVER app_del_server = (PAPP_DEL_SERVER)InputBuffer;

				PLCXL_FILTER pFilter = FindAndLockFilter(app_del_server->miniport_net_luid);
				if (pFilter != NULL) {

					UnlockFilter(pFilter);
				}
			} else {
				Status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		//!添加代码!
        default:
			Status = STATUS_INVALID_PARAMETER;
            break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = InfoLength;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
PLCXL_FILTER
filterFindFilterModule(
    _In_reads_bytes_(BufferLength)
         PUCHAR                   Buffer,
    _In_ ULONG                    BufferLength
    )
{

   PLCXL_FILTER              pFilter;
   PLIST_ENTRY             Link;
   BOOLEAN                  bFalse = FALSE;

   FILTER_ACQUIRE_LOCK(&g_filter_list_lock, bFalse);

   Link = g_filter_list.Flink;

   while (Link != &g_filter_list) {
       pFilter = CONTAINING_RECORD(Link, LCXL_FILTER, FilterModuleLink);

	   if (BufferLength >= pFilter->module_setting->filter_module_name->Length) {
		   if (NdisEqualMemory(Buffer, pFilter->module_setting->filter_module_name->Buffer, pFilter->module_setting->filter_module_name->Length)) {
               FILTER_RELEASE_LOCK(&g_filter_list_lock, bFalse);
               return pFilter;
           }
       }

       Link = Link->Flink;
   }

   FILTER_RELEASE_LOCK(&g_filter_list_lock, bFalse);
   return NULL;
}




