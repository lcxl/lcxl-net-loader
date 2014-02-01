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
    PIO_STACK_LOCATION          irp_sp;
    NTSTATUS                    status = STATUS_SUCCESS;
    PFILTER_DEVICE_EXTENSION    filter_device_extension;
    PUCHAR                      input_buffer;
    PUCHAR                      output_buffer;
    ULONG                       input_buffer_length, output_buffer_length;
    PLIST_ENTRY                 link;
    PUCHAR                      info;
    ULONG                       InfoLength = 0;
    PLCXL_FILTER                filter = NULL;
    BOOLEAN                     bFalse = FALSE;


    UNREFERENCED_PARAMETER(DeviceObject);


    irp_sp = IoGetCurrentIrpStackLocation(Irp);

    if (irp_sp->FileObject == NULL) {
        return(STATUS_UNSUCCESSFUL);
    }


    filter_device_extension = (PFILTER_DEVICE_EXTENSION)NdisGetDeviceReservedExtension(DeviceObject);

    ASSERT(filter_device_extension->Signature == 'FTDR');

    Irp->IoStatus.Information = 0;

	input_buffer = output_buffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
	input_buffer_length = output_buffer_length = irp_sp->Parameters.DeviceIoControl.InputBufferLength;

    switch (irp_sp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_FILTER_RESTART_ALL:
            break;

        case IOCTL_FILTER_RESTART_ONE_INSTANCE:
            filter = filterFindFilterModule (input_buffer, input_buffer_length);

            if (filter == NULL) {

                break;
            }
            NdisFRestartFilter(filter->filter_handle);
            break;

        case IOCTL_FILTER_ENUERATE_ALL_INSTANCES:

            info = output_buffer;
			LockLCXLLockList(&g_filter_list);
            
            link = GetListofLCXLLockList(&g_filter_list)->Flink;
			//遍历列表
			while (link != GetListofLCXLLockList(&g_filter_list)) {
                filter = CONTAINING_RECORD(link, LCXL_FILTER, filter_module_link);

                InfoLength += (filter->module_setting->filter_module_name->Length + sizeof(USHORT));

                if (InfoLength <= output_buffer_length) {
					*(PUSHORT)info = filter->module_setting->filter_module_name->Length;
                    NdisMoveMemory(info + sizeof(USHORT),
						(PUCHAR)(filter->module_setting->filter_module_name->Buffer),
						filter->module_setting->filter_module_name->Length);

					info += (filter->module_setting->filter_module_name->Length + sizeof(USHORT));
                }
                link = link->Flink;
            }
			UnlockLCXLLockList(&g_filter_list);
            if (InfoLength <= output_buffer_length) {
                status = NDIS_STATUS_SUCCESS;
            }
            //
            // Buffer is small
            //
            else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
		//添加代码
		case IOCTL_LOADER_ALL_APP_MODULE:
		{
			PAPP_MODULE_INFO cur_buf;
			PLCXL_MODULE_SETTING_LIST_ENTRY module;

			cur_buf = (PAPP_MODULE_INFO)output_buffer;
			LockLCXLLockList(&g_setting.module_list);
			module = CONTAINING_RECORD(GetListofLCXLLockList(&g_setting.module_list)->Flink, LCXL_MODULE_SETTING_LIST_ENTRY, list_entry);
			while (&module->list_entry != GetListofLCXLLockList(&g_setting.module_list)) {
				//先判断缓冲区是否足够
				if (output_buffer_length - (ULONG)((LONG_PTR)cur_buf - (LONG_PTR)output_buffer) < sizeof(APP_MODULE_INFO)) {
					status = STATUS_BUFFER_TOO_SMALL;
					break;
				}
				cur_buf->app_module_status = module->ref_count == 0 ? AMS_NO_FILTER : AMS_NORMAL;
				FILTER_ACQUIRE_LOCK(&module->lock, bFalse);
				cur_buf->real_addr = module->real_addr;
				cur_buf->virtual_ipv4 = module->virtual_ipv4;
				cur_buf->virtual_ipv6 = module->virtual_ipv6;
				LockLCXLLockList(&module->server_list);
				cur_buf->server_count = GetListCountofLCXLLockList(&module->server_list);
				UnlockLCXLLockList(&module->server_list);
				FILTER_RELEASE_LOCK(&module->lock, bFalse);
				cur_buf->miniport_net_luid = module->miniport_net_luid;

				cur_buf->filter_module_name_len = (sizeof(cur_buf->filter_module_name) < module->filter_module_name->Length) ? sizeof(cur_buf->filter_module_name) : module->filter_module_name->Length;
				RtlCopyMemory(cur_buf->filter_module_name, module->filter_module_name->Buffer, cur_buf->filter_module_name_len);

				cur_buf->miniport_friendly_name_len = (sizeof(cur_buf->miniport_friendly_name) < module->miniport_friendly_name->Length) ? sizeof(cur_buf->miniport_friendly_name) : module->miniport_friendly_name->Length;
				RtlCopyMemory(cur_buf->miniport_friendly_name, module->miniport_friendly_name->Buffer, cur_buf->miniport_friendly_name_len);

				cur_buf->miniport_name_len = (sizeof(cur_buf->miniport_name) < module->miniport_name->Length) ? sizeof(cur_buf->miniport_name) : module->miniport_name->Length;
				RtlCopyMemory(cur_buf->miniport_name, module->miniport_name->Buffer, cur_buf->miniport_name_len);

				cur_buf++;
				module = CONTAINING_RECORD(module->list_entry.Flink, LCXL_MODULE_SETTING_LIST_ENTRY, list_entry);
			}
			UnlockLCXLLockList(&g_setting.module_list);
			InfoLength = (ULONG)((ULONG_PTR)cur_buf - (ULONG_PTR)output_buffer);
		}
			break;
		case IOCTL_LOADER_SET_VIRTUAL_IP:
			if (input_buffer_length == sizeof(APP_IP)) {
				PAPP_IP ip;
				PLCXL_FILTER pFilter;

				ip = (PAPP_IP)input_buffer;
				LockLCXLLockList(&g_filter_list);
				pFilter = FindFilter(ip->miniport_net_luid);
				if (pFilter != NULL) {
					NdisAcquireSpinLock(&pFilter->module_setting->lock);
					switch (ip->ip.ip_mode) {
						//IPv4模式
					case IM_IPV4:
						pFilter->module_setting->virtual_ipv4 = ip->ip.addr.ip_4;
						break;
						//IPv6模式
					case IM_IPV6:
						pFilter->module_setting->virtual_ipv6 = ip->ip.addr.ip_6;
						break;
					default:
						status = STATUS_INVALID_PARAMETER;
						break;
					}
					NdisReleaseSpinLock(&pFilter->module_setting->lock);
				}
				
				UnlockLCXLLockList(&g_filter_list);
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_LOADER_GET_SERVER_LIST:
			if (input_buffer_length == sizeof(NET_LUID)) {
				PLCXL_FILTER pFilter;

				LockLCXLLockList(&g_filter_list); 
				pFilter = FindFilter(*(PNET_LUID)input_buffer);
				if (pFilter != NULL) {
					if (pFilter->module_setting != NULL && (pFilter->module_setting->flag & MSF_DELETE_AFTER_RESTART) == 0) {
						
					} else {
						status = STATUS_NOT_FOUND;
					}
				} else {
					status = STATUS_NOT_FOUND;
				}
				UnlockLCXLLockList(&g_filter_list);
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_LOADER_ADD_SERVER:
			if (sizeof(APP_ADD_SERVER) == input_buffer_length) {
				PAPP_ADD_SERVER app_add_server;
				PLCXL_FILTER pFilter;
				
				app_add_server = (PAPP_ADD_SERVER)input_buffer;
				LockLCXLLockList(&g_filter_list);
				pFilter = FindFilter(app_add_server->miniport_net_luid);
				if (pFilter != NULL) {
					
					LockLCXLLockList(&pFilter->module_setting->server_list);
					
					
					UnlockLCXLLockList(&pFilter->module_setting->server_list);
				}
				UnlockLCXLLockList(&g_filter_list);
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_LOADER_DEL_SERVER:
			if (sizeof(APP_DEL_SERVER) == input_buffer_length) {
				PAPP_DEL_SERVER app_del_server = (PAPP_DEL_SERVER)input_buffer;
				PLCXL_FILTER pFilter;

				LockLCXLLockList(&g_filter_list);
				pFilter = FindFilter(app_del_server->miniport_net_luid);
				UnlockLCXLLockList(&g_filter_list);
				
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		//!添加代码!
        default:
			status = STATUS_INVALID_PARAMETER;
            break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = InfoLength;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
PLCXL_FILTER
filterFindFilterModule(
    _In_reads_bytes_(BufferLength)
         PUCHAR                   Buffer,
    _In_ ULONG                    BufferLength
    )
{

   PLCXL_FILTER            pFilter;
   PLIST_ENTRY             Link;


   LockLCXLLockList(&g_filter_list);
   Link = GetListofLCXLLockList(&g_filter_list)->Flink;

   while (Link != GetListofLCXLLockList(&g_filter_list)) {
	   pFilter = CONTAINING_RECORD(Link, LCXL_FILTER, filter_module_link);

	   if (BufferLength >= pFilter->module_setting->filter_module_name->Length) {
		   if (NdisEqualMemory(Buffer, pFilter->module_setting->filter_module_name->Buffer, pFilter->module_setting->filter_module_name->Length)) {
			   UnlockLCXLLockList(&g_filter_list);
               return pFilter;
           }
       }

       Link = Link->Flink;
   }

   UnlockLCXLLockList(&g_filter_list);
   return NULL;
}




