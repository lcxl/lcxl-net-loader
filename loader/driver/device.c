﻿/*++
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

                InfoLength += (filter->module.filter_module_name->Length + sizeof(USHORT));

                if (InfoLength <= output_buffer_length) {
					*(PUSHORT)info = filter->module.filter_module_name->Length;
                    NdisMoveMemory(info + sizeof(USHORT),
						(PUCHAR)(filter->module.filter_module_name->Buffer),
						filter->module.filter_module_name->Length);

					info += (filter->module.filter_module_name->Length + sizeof(USHORT));
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
		case IOCTL_GET_ROLE:
		{
			if (output_buffer_length == sizeof(INT)) {
				
				*(PINT)output_buffer = g_setting.lcxl_role;
				InfoLength = sizeof(INT);
			}
		}
			break;
		case IOCTL_SET_ROLE:
			if (input_buffer_length == sizeof(INT)) {
				INT lcxl_role = *(PINT)input_buffer;
				switch (lcxl_role) {
				case LCXL_ROLE_UNKNOWN:
					break;
				case LCXL_ROLE_ROUTER:
					if (g_setting.lcxl_role != LCXL_ROLE_UNKNOWN) {
						//参数非法
						status = STATUS_INVALID_PARAMETER;
					}
					break;
				case LCXL_ROLE_SERVER:
					if (g_setting.lcxl_role != LCXL_ROLE_UNKNOWN) {
						//参数非法
						status = STATUS_INVALID_PARAMETER;
					}
					break;
				}
				if (NT_SUCCESS(status)) {
					g_setting.lcxl_role = lcxl_role;
				}
			}
			break;
		case IOCTL_GET_MODULE_LIST:
		{
			PAPP_MODULE cur_buf;

			cur_buf = (PAPP_MODULE)output_buffer;
			//锁定列表数量
			LockLCXLLockList(&g_filter_list);
			//先判断缓冲区是否足够
			if (output_buffer_length >= GetListCountofLCXLLockList(&g_filter_list)*sizeof(APP_MODULE)) {

				filter = CONTAINING_RECORD(GetListofLCXLLockList(&g_filter_list)->Flink, LCXL_FILTER, filter_module_link);
				while (&filter->filter_module_link != GetListofLCXLLockList(&g_filter_list)) {
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

					//需要线程保护的数据
					
					LockFilter(filter, &lock_handle);
					cur_buf->virtual_addr = module->virtual_addr;
					LockLCXLLockList(&module->server_list);
					cur_buf->role.loader.server_count = GetListCountofLCXLLockList(&module->server_list);
					UnlockLCXLLockList(&module->server_list);
					UnlockFilter(&lock_handle);

					cur_buf++;
					filter = CONTAINING_RECORD(filter->filter_module_link.Flink, LCXL_FILTER, filter_module_link);
				}
			} else {
				status = STATUS_BUFFER_TOO_SMALL;
			}
			UnlockLCXLLockList(&g_filter_list);
			InfoLength = (ULONG)((ULONG_PTR)cur_buf - (ULONG_PTR)output_buffer);
		}
			break;
		case IOCTL_SET_VIRTUAL_ADDR:
			if (input_buffer_length == sizeof(APP_VIRTUAL_IP)) {
				PAPP_VIRTUAL_IP ip;

				ip = (PAPP_VIRTUAL_IP)input_buffer;
				LockLCXLLockList(&g_filter_list);
				filter = FindFilter(ip->miniport_net_luid);
				if (filter != NULL) {
					KLOCK_QUEUE_HANDLE lock_handle;
					LockFilter(filter, &lock_handle);
					filter->module.virtual_addr = ip->addr;
					UnlockFilter(&lock_handle);
				} else {
					status = STATUS_NOT_FOUND;
				}
				
				UnlockLCXLLockList(&g_filter_list);
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_LOADER_GET_SERVER_LIST:
			if (g_setting.lcxl_role != LCXL_ROLE_ROUTER) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (input_buffer_length == sizeof(NET_LUID)) {
				PLCXL_SERVER cur_buf;

				cur_buf = (PLCXL_SERVER)output_buffer;
				LockLCXLLockList(&g_filter_list); 
				filter = FindFilter(*(PNET_LUID)input_buffer);
				if (filter != NULL) {
					PLCXL_MODULE_SETTING_INFO module = &filter->module;
					LockLCXLLockList(&module->server_list);
					//先判断缓冲区是否足够
					if (output_buffer_length >= GetListCountofLCXLLockList(&g_filter_list)*sizeof(LCXL_SERVER)) {
						PLIST_ENTRY server_entry = GetListofLCXLLockList(&module->server_list)->Flink;
						while (server_entry != GetListofLCXLLockList(&module->server_list)) {
							PSERVER_INFO_LIST_ENTRY server = GetServerbyListEntry(server_entry);

							*cur_buf = server->info;

							cur_buf++;
							server_entry = server_entry->Flink;
						}
					} else {
						status = STATUS_BUFFER_TOO_SMALL;
					}
					UnlockLCXLLockList(&module->server_list);
				} else {
					status = STATUS_NOT_FOUND;
				}
				UnlockLCXLLockList(&g_filter_list);
				InfoLength = (ULONG)((ULONG_PTR)cur_buf - (ULONG_PTR)output_buffer);
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_LOADER_ADD_SERVER:
			if (g_setting.lcxl_role != LCXL_ROLE_ROUTER) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (sizeof(APP_ADD_SERVER) == input_buffer_length) {
				PAPP_ADD_SERVER app_add_server;
				
				app_add_server = (PAPP_ADD_SERVER)input_buffer;
				//寻找module
				LockLCXLLockList(&g_filter_list);
				filter = FindFilter(app_add_server->miniport_net_luid);
				if (filter != NULL) {
					PSERVER_INFO_LIST_ENTRY server = NULL;
					//锁住server列表
					LockLCXLLockList(&filter->module.server_list);
					//查找server列表
					server = FindServer(&filter->module.server_list, &app_add_server->server.mac_addr);
					//找不到？
					if (server == NULL) {
						//添加服务器
						server = AllocServer();
						server->info = app_add_server->server;
						AddtoLCXLLockList(&filter->module.server_list, &server->list_entry.list_entry);
					} else {
						KLOCK_QUEUE_HANDLE lock_handle;
						//更改服务器配置
						LockServer(server, &lock_handle);
						server->info = app_add_server->server;
						UnLockServer(&lock_handle);
					}
					UnlockLCXLLockList(&filter->module.server_list);
				} else {
					status = STATUS_NOT_FOUND;
				}
				UnlockLCXLLockList(&g_filter_list);
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_LOADER_DEL_SERVER:
			if (g_setting.lcxl_role != LCXL_ROLE_ROUTER) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (sizeof(APP_DEL_SERVER) == input_buffer_length) {
				PAPP_DEL_SERVER app_del_server = (PAPP_DEL_SERVER)input_buffer;

				LockLCXLLockList(&g_filter_list);
				filter = FindFilter(app_del_server->miniport_net_luid);
				if (filter != NULL) {
					PSERVER_INFO_LIST_ENTRY server = NULL;
					LockLCXLLockList(&filter->module.server_list);
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
				UnlockLCXLLockList(&g_filter_list);
				
			} else {
				status = STATUS_INFO_LENGTH_MISMATCH;
			}
			break;
		case IOCTL_SERVER_SET_ROUTER_MAC_ADDR://服务器角色：设置负载均衡器的mac地址
			if (g_setting.lcxl_role != LCXL_ROLE_SERVER) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (sizeof(APP_SET_ROUTER_MAC_ADDR) == input_buffer_length) {
				PAPP_SET_ROUTER_MAC_ADDR router_mac_addr = (PAPP_SET_ROUTER_MAC_ADDR)input_buffer;

				LockLCXLLockList(&g_filter_list);
				filter = FindFilter(router_mac_addr->miniport_net_luid);
				if (filter != NULL) {
					KLOCK_QUEUE_HANDLE lock_handle;

					LockFilter(filter, &lock_handle);
					filter->module.router_mac_addr = router_mac_addr->mac_addr;
					UnlockFilter(&lock_handle);
				}
				UnlockLCXLLockList(&g_filter_list);
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

	   if (BufferLength >= pFilter->module.filter_module_name->Length) {
		   if (NdisEqualMemory(Buffer, pFilter->module.filter_module_name->Buffer, pFilter->module.filter_module_name->Length)) {
			   UnlockLCXLLockList(&g_filter_list);
               return pFilter;
           }
       }

       Link = Link->Flink;
   }

   UnlockLCXLLockList(&g_filter_list);
   return NULL;
}




