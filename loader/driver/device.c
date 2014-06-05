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
    ULONG                       InfoLength = 0;
    PLCXL_FILTER                filter = NULL;


    UNREFERENCED_PARAMETER(DeviceObject);


    irp_sp = IoGetCurrentIrpStackLocation(Irp);

    if (irp_sp->FileObject == NULL) {
        return(STATUS_UNSUCCESSFUL);
    }


    filter_device_extension = (PFILTER_DEVICE_EXTENSION)NdisGetDeviceReservedExtension(DeviceObject);

    ASSERT(filter_device_extension->Signature == 'FTDR');

    Irp->IoStatus.Information = 0;

	input_buffer = output_buffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
	input_buffer_length = irp_sp->Parameters.DeviceIoControl.InputBufferLength;
	output_buffer_length = irp_sp->Parameters.DeviceIoControl.OutputBufferLength;
    switch (irp_sp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_RESTART_MODULE:
			if (input_buffer_length == sizeof(NET_LUID)) {
				PNET_LUID miniport_net_luid = (PNET_LUID)input_buffer;

				LockLcxlList(&g_filter_list);
				filter = FindFilter(*miniport_net_luid);
				if (filter != NULL) {
					//重启模块
					NdisFRestartFilter(filter->filter_handle);
				} else {
					//设备不存在
					status = STATUS_DEVICE_DOES_NOT_EXIST;
				}
				UnlockLcxlList(&g_filter_list);
			}
            break;

		case IOCTL_SET_ROLE:
			if (input_buffer_length == sizeof(APP_SET_ROLE)) {
				PAPP_SET_ROLE set_role = (PAPP_SET_ROLE)input_buffer;

				//锁定列表数量
				LockLcxlList(&g_filter_list);
				filter = FindFilter(set_role->miniport_net_luid);
				if (filter != NULL) {
					KLOCK_QUEUE_HANDLE lock_handle;
					LockFilter(filter, &lock_handle);
					if (set_role->lcxl_role != filter->module.lcxl_role) {
						switch (set_role->lcxl_role) {
						case LCXL_ROLE_UNKNOWN:
							break;
						case LCXL_ROLE_ROUTER:
							if (filter->module.lcxl_role != LCXL_ROLE_UNKNOWN) {
								//参数非法
								status = STATUS_INVALID_PARAMETER;
							}
							break;
						case LCXL_ROLE_SERVER:
							if (filter->module.lcxl_role != LCXL_ROLE_UNKNOWN) {
								//参数非法
								status = STATUS_INVALID_PARAMETER;
							}
							break;
						}
						if (NT_SUCCESS(status)) {
							filter->module.lcxl_role = set_role->lcxl_role;
						}
					}
					
					UnlockFilter(&lock_handle);
				} else {
					status = STATUS_NOT_FOUND;
				}
				UnlockLcxlList(&g_filter_list);
			}
			break;
		case IOCTL_GET_MODULE_LIST:
		{
			PAPP_MODULE cur_buf;

			cur_buf = (PAPP_MODULE)output_buffer;
			//锁定列表数量
			LockLcxlList(&g_filter_list);
			//先判断缓冲区是否足够
			if (output_buffer_length >= GetLcxlListCount(&g_filter_list)*sizeof(APP_MODULE)) {

				filter = CONTAINING_RECORD(GetLcxlListHead(&g_filter_list)->Flink, LCXL_FILTER, filter_module_link);
				while (&filter->filter_module_link != GetLcxlListHead(&g_filter_list)) {
					PLCXL_MODULE_SETTING_INFO module = &filter->module;
					INT buflen;
					KLOCK_QUEUE_HANDLE lock_handle;

					//只读数据
					cur_buf->miniport_ifindex = module->miniport_ifindex;
					cur_buf->miniport_net_luid = module->miniport_net_luid;
					cur_buf->mac_addr = module->mac_addr;

					buflen = (sizeof(cur_buf->filter_module_name) - sizeof(WCHAR) < module->filter_module_name->Length) ? sizeof(cur_buf->filter_module_name) - sizeof(WCHAR) : module->filter_module_name->Length;
					RtlCopyMemory(cur_buf->filter_module_name, module->filter_module_name->Buffer, buflen);
					cur_buf->filter_module_name[buflen / sizeof(WCHAR)] = L'\0';

					buflen = (sizeof(cur_buf->miniport_friendly_name - sizeof(WCHAR)) < module->miniport_friendly_name->Length) ? sizeof(cur_buf->miniport_friendly_name) - sizeof(WCHAR) : module->miniport_friendly_name->Length;
					RtlCopyMemory(cur_buf->miniport_friendly_name, module->miniport_friendly_name->Buffer, buflen);
					cur_buf->miniport_friendly_name[buflen / sizeof(WCHAR)] = L'\0';

					buflen = (sizeof(cur_buf->miniport_name) < module->miniport_name->Length - sizeof(WCHAR)) ? sizeof(cur_buf->miniport_name) - sizeof(WCHAR) : module->miniport_name->Length;
					RtlCopyMemory(cur_buf->miniport_name, module->miniport_name->Buffer, buflen);
					cur_buf->miniport_name[buflen / sizeof(WCHAR)] = L'\0';

					//原子数据
					cur_buf->lcxl_role = module->lcxl_role;
					cur_buf->route_timeout = module->route_timeout;
					cur_buf->routing_algorithm = module->routing_algorithm;
					//需要线程保护的数据
					
					LockFilter(filter, &lock_handle);
					cur_buf->virtual_addr = module->virtual_addr;
					cur_buf->server_check = module->server_check;
					LockLcxlList(&module->server_list);
					cur_buf->server_count = GetLcxlListCount(&module->server_list);
					UnlockLcxlList(&module->server_list);
					UnlockFilter(&lock_handle);

					cur_buf++;
					filter = CONTAINING_RECORD(filter->filter_module_link.Flink, LCXL_FILTER, filter_module_link);
				}
			} else {
				status = STATUS_BUFFER_TOO_SMALL;
			}
			UnlockLcxlList(&g_filter_list);
			InfoLength = (ULONG)((ULONG_PTR)cur_buf - (ULONG_PTR)output_buffer);
		}
			break;
		case IOCTL_SET_VIRTUAL_ADDR:
			if (input_buffer_length == sizeof(APP_SET_VIRTUAL_ADDR)) {
				PAPP_SET_VIRTUAL_ADDR ip;

				ip = (PAPP_SET_VIRTUAL_ADDR)input_buffer;
				LockLcxlList(&g_filter_list);
				filter = FindFilter(ip->miniport_net_luid);
				if (filter != NULL) {
					KLOCK_QUEUE_HANDLE lock_handle;
					LockFilter(filter, &lock_handle);
					filter->module.virtual_addr = ip->addr;
					UnlockFilter(&lock_handle);
				} else {
					status = STATUS_NOT_FOUND;
				}
				
				UnlockLcxlList(&g_filter_list);
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_ROUTER_GET_SERVER_LIST:
			if (input_buffer_length == sizeof(NET_LUID)) {
				PLCXL_SERVER cur_buf;

				cur_buf = (PLCXL_SERVER)output_buffer;
				LockLcxlList(&g_filter_list); 
				filter = FindFilter(*(PNET_LUID)input_buffer);
				if (filter != NULL) {
					PLCXL_MODULE_SETTING_INFO module = &filter->module;
					LockLcxlList(&module->server_list);
					//先判断缓冲区是否足够
					if (output_buffer_length >= GetLcxlListCount(&g_filter_list)*sizeof(LCXL_SERVER)) {
						PLIST_ENTRY server_entry = GetLcxlListHead(&module->server_list)->Flink;
						while (server_entry != GetLcxlListHead(&module->server_list)) {
							PSERVER_INFO_LIST_ENTRY server = GetServerbyListEntry(server_entry);

							*cur_buf = server->info;

							cur_buf++;
							server_entry = server_entry->Flink;
						}
					} else {
						status = STATUS_BUFFER_TOO_SMALL;
					}
					UnlockLcxlList(&module->server_list);
				} else {
					status = STATUS_NOT_FOUND;
				}
				UnlockLcxlList(&g_filter_list);
				InfoLength = (ULONG)((ULONG_PTR)cur_buf - (ULONG_PTR)output_buffer);
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_ROUTER_ADD_SERVER:
			if (sizeof(APP_ADD_SERVER) == input_buffer_length) {
				PAPP_ADD_SERVER app_add_server;
				
				app_add_server = (PAPP_ADD_SERVER)input_buffer;
				//寻找module
				LockLcxlList(&g_filter_list);
				filter = FindFilter(app_add_server->miniport_net_luid);
				if (filter != NULL) {
					PSERVER_INFO_LIST_ENTRY server = NULL;
					//锁住server列表
					LockLcxlList(&filter->module.server_list);
					//查找server列表
					server = FindServer(&filter->module.server_list, &app_add_server->server.mac_addr);
					//找不到？
					if (server == NULL) {
						//添加服务器
						server = AllocServer();
						server->info = app_add_server->server;
						AddEntrytoLcxlList(&filter->module.server_list, &server->list_entry.list_entry);
					} else {
						KLOCK_QUEUE_HANDLE lock_handle;
						//更改服务器配置
						LockServer(server, &lock_handle);
						server->info = app_add_server->server;
						UnLockServer(&lock_handle);
					}
					UnlockLcxlList(&filter->module.server_list);
				} else {
					status = STATUS_NOT_FOUND;
				}
				UnlockLcxlList(&g_filter_list);
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_ROUTER_DEL_SERVER:
			if (sizeof(APP_DEL_SERVER) == input_buffer_length) {
				PAPP_DEL_SERVER app_del_server = (PAPP_DEL_SERVER)input_buffer;

				LockLcxlList(&g_filter_list);
				filter = FindFilter(app_del_server->miniport_net_luid);
				if (filter != NULL) {
					PSERVER_INFO_LIST_ENTRY server = NULL;
					LockLcxlList(&filter->module.server_list);
					server = FindServer(&filter->module.server_list, &app_del_server->mac_addr);

					if (server != NULL) {
						KLOCK_QUEUE_HANDLE lock_handle;

						LockServer(server, &lock_handle);
						if ((server->info.status & SS_DELETED) == 0) {
							//标识此服务器已被删除
							server->info.status |= SS_DELETED;
							DecRefListEntry(&filter->module.server_list, &server->list_entry);
						} else {
							//服务器已经被删除
							status = STATUS_FILE_DELETED;
						}
						UnLockServer(&lock_handle);
					}

				} else {
					status = STATUS_NOT_FOUND;
				}
				UnlockLcxlList(&g_filter_list);
				
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
			
		case IOCTL_ROUTER_SET_SERVER_CHECK://负载均衡器角色
			if (sizeof(APP_SET_SERVER_CHECK) == input_buffer_length) {
				PAPP_SET_SERVER_CHECK set_server_check = (PAPP_SET_SERVER_CHECK)input_buffer;

				LockLcxlList(&g_filter_list);
				filter = FindFilter(set_server_check->miniport_net_luid);
				if (filter != NULL) {
					KLOCK_QUEUE_HANDLE lock_handle;

					LockFilter(filter, &lock_handle);
					filter->module.server_check = set_server_check->server_check;
					UnlockFilter(&lock_handle);
				}
				UnlockLcxlList(&g_filter_list);
			}
			break;
		case IOCTL_ROUTER_SET_ROUTING_ALGORITHM://负载均衡器角色
			if (sizeof(APP_SET_ROUTING_ALGORITHM) == input_buffer_length) {
				PAPP_SET_ROUTING_ALGORITHM set_routing_algorithm = (PAPP_SET_ROUTING_ALGORITHM)input_buffer;

				LockLcxlList(&g_filter_list);
				filter = FindFilter(set_routing_algorithm->miniport_net_luid);
				if (filter != NULL) {
					KLOCK_QUEUE_HANDLE lock_handle;

					LockFilter(filter, &lock_handle);
					filter->module.routing_algorithm = set_routing_algorithm->routing_algorithm;
					UnlockFilter(&lock_handle);
				}
				UnlockLcxlList(&g_filter_list);
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
FindModuleByName(
    _In_reads_bytes_(buffer_length)
         PUCHAR                   buffer,
    _In_ ULONG                    buffer_length
    )
{

   PLCXL_FILTER            filter;
   PLIST_ENTRY             link;


   LockLcxlList(&g_filter_list);
   link = GetLcxlListHead(&g_filter_list)->Flink;

   while (link != GetLcxlListHead(&g_filter_list)) {
	   filter = CONTAINING_RECORD(link, LCXL_FILTER, filter_module_link);

	   if (buffer_length >= filter->module.filter_module_name->Length) {
		   if (NdisEqualMemory(buffer, filter->module.filter_module_name->Buffer, filter->module.filter_module_name->Length)) {
			   UnlockLcxlList(&g_filter_list);
               return filter;
           }
       }

       link = link->Flink;
   }

   UnlockLcxlList(&g_filter_list);
   return NULL;
}




