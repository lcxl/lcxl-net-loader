#ifndef _LCXL_LOCK_LIST_H_
#define _LCXL_LOCK_LIST_H_
//author:LCXL
//abstract:驱动中所使用的列表结构，用于提高锁定列表后的性能
#include <wdm.h>
#define TAG_TO_BE 'TOBE'
//删除列表项回调函数。
//注意：此函数运行在DISPATCH_LEVEL 下
typedef VOID (*DEL_LIST_ENTRY_FUNC) (IN PLIST_ENTRY list_entry);

//要添加/删除的列表项
typedef struct _LCXL_TO_BE_LIST {
	LIST_ENTRY list_entry;

	//要添加/删除的列表项
	PLIST_ENTRY to_be_list_entry;
} LCXL_TO_BE_LIST, *PLCXL_TO_BE_LIST;

typedef struct _LCXL_LOCK_LIST {
	//列表，只有当锁定的时候才能访问，通过GetListofLCXLLockList获得
	LIST_ENTRY				list;
	//列表项数量，只有当锁定的时候才能访问，通过GetListCountofLCXLLockList获得
	INT						list_count;
	//待添加列表，LCXL_TO_BE_LIST
	LIST_ENTRY				to_be_add_list;
	//待删除列表，LCXL_TO_BE_LIST
	LIST_ENTRY				to_be_del_list;
	//内存管理器
	NPAGED_LOOKASIDE_LIST	to_be_mem_mgr;
	//自旋锁
	KSPIN_LOCK				lock;
	//被锁定的次数
	BOOLEAN					lock_count;
	//删除列表项回调函数。
	//注意：此函数运行在DISPATCH_LEVEL 下
	DEL_LIST_ENTRY_FUNC		del_func;
} LCXL_LOCK_LIST, *PLCXL_LOCK_LIST;//LCXL锁列表

//初始化LCXL锁列表
//lock_list:锁列表结构指针
//del_func:删除对调函数，当从LCXL锁列表删除列表项时会调用del_func函数来释放列表项指针
__inline VOID InitLCXLLockList(IN OUT PLCXL_LOCK_LIST lock_list, IN DEL_LIST_ENTRY_FUNC del_func)
{
	ASSERT(lock_list != NULL && del_func != NULL);
	lock_list->lock_count = 0;
	lock_list->list_count = 0;
	InitializeListHead(&lock_list->list);
	InitializeListHead(&lock_list->to_be_add_list);
	InitializeListHead(&lock_list->to_be_del_list);
	lock_list->del_func = del_func;
	KeInitializeSpinLock(&lock_list->lock);
	ExInitializeNPagedLookasideList(&lock_list->to_be_mem_mgr, NULL, NULL, 0, sizeof(LCXL_TO_BE_LIST), TAG_TO_BE, 0);
}

//锁定LCXL列表项，锁定之后列表项的数量不会更改，并且不会更改当前的中断请求级别（IRQL）
__inline VOID LockLCXLLockList(IN PLCXL_LOCK_LIST lock_list)
{
	//保存之前的IRQL
	KLOCK_QUEUE_HANDLE		queue_handle;
	KeAcquireInStackQueuedSpinLock(&lock_list->lock, &queue_handle);
	lock_list->lock_count++;
	KeReleaseInStackQueuedSpinLock(&queue_handle);
}

//获取列表，必须在锁定的时候获取
__inline PLIST_ENTRY GetListofLCXLLockList(IN PLCXL_LOCK_LIST lock_list)
{
	ASSERT(lock_list->lock_count > 0);
	return &lock_list->list;
}

//获取列表数量，必须在锁定的时候获取
__inline INT GetListCountofLCXLLockList(IN PLCXL_LOCK_LIST lock_list)
{
	ASSERT(lock_list->lock_count > 0);
	return lock_list->list_count;
}

//添加列表项到LCXL锁定列表中，如果被锁定，则会在UnlockLCXLLockList函数执行时进行添加
__inline VOID AddtoLCXLLockList(IN PLCXL_LOCK_LIST lock_list, IN PLIST_ENTRY list_entry)
{
	//保存之前的IRQL
	KLOCK_QUEUE_HANDLE		queue_handle;
	KeAcquireInStackQueuedSpinLock(&lock_list->lock, &queue_handle);
	
	if (lock_list->lock_count > 0) {
		PLCXL_TO_BE_LIST new_list_entry;
		//如果被锁定，则将列表项添加到待添加列表中
		new_list_entry = (PLCXL_TO_BE_LIST)ExAllocateFromNPagedLookasideList(&lock_list->to_be_mem_mgr);
		new_list_entry->to_be_list_entry = list_entry;
		InsertTailList(&lock_list->to_be_add_list, &new_list_entry->list_entry);
	} else {
		//没锁住，则直接添加
		lock_list->list_count++;
		InsertTailList(&lock_list->list, list_entry);
	}
	KeReleaseInStackQueuedSpinLock(&queue_handle);
}
//从LCXL锁定列表删除指定的列表项，如果被锁定，则会在UnlockLCXLLockList函数执行时进行删除
__inline VOID DelFromLCXLLockList(IN PLCXL_LOCK_LIST lock_list, IN PLIST_ENTRY list_entry)
{
	//保存之前的IRQL
	KLOCK_QUEUE_HANDLE		queue_handle;
	KeAcquireInStackQueuedSpinLock(&lock_list->lock, &queue_handle);

	if (lock_list->lock_count > 0) {
		PLCXL_TO_BE_LIST del_list_entry;
		//如果被锁定，则将列表项添加到待删除列表中
		del_list_entry = (PLCXL_TO_BE_LIST)ExAllocateFromNPagedLookasideList(&lock_list->to_be_mem_mgr);
		del_list_entry->to_be_list_entry = list_entry;
		InsertTailList(&lock_list->to_be_del_list, &del_list_entry->list_entry);
	} else {
		//没锁住，则直接删除
		lock_list->list_count--;
		RemoveEntryList(list_entry);
		lock_list->del_func(list_entry);
	}

	KeReleaseInStackQueuedSpinLock(&queue_handle);
}

//解锁列表项，如果有待添加的列表项或待删除的列表项，则会在这里进行添加/删除
__inline VOID UnlockLCXLLockList(IN PLCXL_LOCK_LIST lock_list)
{
	//保存之前的IRQL
	KLOCK_QUEUE_HANDLE		queue_handle;
	KeAcquireInStackQueuedSpinLock(&lock_list->lock, &queue_handle);

	lock_list->lock_count--;
	if (lock_list->lock_count == 0) {
		//PLCXL_TO_BE_LIST list_entry;
		PLIST_ENTRY list_entry;

		list_entry = lock_list->to_be_del_list.Flink;
		while (list_entry != &lock_list->to_be_del_list) {
			PLCXL_TO_BE_LIST tobe_list_entry;
			
			tobe_list_entry = CONTAINING_RECORD(list_entry, LCXL_TO_BE_LIST, list_entry);
			list_entry = list_entry->Flink;
			
			//从列表中删除待删除的项
			lock_list->list_count--;
			RemoveEntryList(tobe_list_entry->to_be_list_entry);
			lock_list->del_func(tobe_list_entry->to_be_list_entry);
			//移除本身
			RemoveEntryList(&tobe_list_entry->list_entry);
			ExFreeToNPagedLookasideList(&lock_list->to_be_mem_mgr, tobe_list_entry);
		}

		list_entry = lock_list->to_be_add_list.Flink;
		while (list_entry != &lock_list->to_be_add_list) {

			PLCXL_TO_BE_LIST tobe_list_entry;

			tobe_list_entry = CONTAINING_RECORD(list_entry, LCXL_TO_BE_LIST, list_entry);
			list_entry = list_entry->Flink;

			//将待添加的项插入到列表中
			lock_list->list_count++;
			InsertTailList(&lock_list->list, tobe_list_entry->to_be_list_entry);
			
			RemoveEntryList(&tobe_list_entry->list_entry);
			ExFreeToNPagedLookasideList(&lock_list->to_be_mem_mgr, tobe_list_entry);
		}
	}
	KeReleaseInStackQueuedSpinLock(&queue_handle);
}
//删除LCXL锁定列表
__inline VOID DelLCXLLockList(IN PLCXL_LOCK_LIST lock_list)
{
#ifdef DBG
	KLOCK_QUEUE_HANDLE		queue_handle;
	KeAcquireInStackQueuedSpinLock(&lock_list->lock, &queue_handle);
	ASSERT(lock_list->lock_count == 0);
	ASSERT(lock_list->list_count == 0);
	KeReleaseInStackQueuedSpinLock(&queue_handle);
#endif // DBG

	

	ASSERT(lock_list != NULL);
	ExDeleteNPagedLookasideList(&lock_list->to_be_mem_mgr);
}
#endif