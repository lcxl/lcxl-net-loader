#ifndef _REF_LOCK_LIST_H_
#define _REF_LOCK_LIST_H_
//author:LCXL
//abstract:带引用的LCXL锁列表
#include "lcxl_lock_list.h"

typedef struct _REF_LIST_ENTRY {
	LIST_ENTRY	list_entry;
	//引用计数
	LONG		ref_count;
} REF_LIST_ENTRY, *PREF_LIST_ENTRY;//带引用的列表项

//************************************
// 简介: 将LIST_ENTRY转换到REF_LIST_ENTRY
// 返回: PREF_LIST_ENTRY
// 参数: IN PLIST_ENTRY list_entry
//************************************
__inline PREF_LIST_ENTRY GetRefListEntry(IN PLIST_ENTRY	list_entry)
{
	return CONTAINING_RECORD(list_entry, REF_LIST_ENTRY, list_entry);
}
//增加列表项引用
//参数：
//list:LCXL锁列表
//list_entry:列表项，需要注意的是list_entry必须在list中
__inline LONG IncRefListEntry(IN PLCXL_LOCK_LIST list, IN PREF_LIST_ENTRY list_entry)
{
	ASSERT(list != NULL);
	ASSERT(list_entry != NULL);
	UNREFERENCED_PARAMETER(list);
	return InterlockedIncrement(&list_entry->ref_count);
}
//减少列表项引用
//参数：
//list:LCXL锁列表
//list_entry:列表项，需要注意的是list_entry必须在list中
//提示：当list_entry引用为0时，此列表项将被删除
__inline LONG DecRefListEntry(IN PLCXL_LOCK_LIST list, IN PREF_LIST_ENTRY list_entry)
{
	LONG ref_count;

	ASSERT(list != NULL);
	ASSERT(list_entry != NULL);
	ref_count = InterlockedDecrement(&list_entry->ref_count);
	if (ref_count <= 0) {
		ASSERT(ref_count == 0);
		DelFromLCXLLockList(list, &list_entry->list_entry);
	}
	return ref_count;
}


#endif