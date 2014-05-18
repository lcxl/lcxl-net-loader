#include "precomp.h"

PUNICODE_STRING LCXLNewString(IN PUNICODE_STRING sour)
{
	PUNICODE_STRING resu;

	resu = ExAllocatePoolWithTag(NonPagedPool, sour->MaximumLength + sizeof(UNICODE_STRING), TAG_FILE_BUFFER);
	if (resu != NULL) {
		resu->Buffer = (PWCH)((PUCHAR)resu + sizeof(UNICODE_STRING));
		resu->MaximumLength = sour->MaximumLength;
		RtlCopyUnicodeString(resu, sour);
	}
	return resu;
}

VOID LCXLFreeString(PUNICODE_STRING dest)
{
	if (dest != NULL) {
		ExFreePoolWithTag(dest, TAG_FILE_BUFFER);
	}
}

PLIST_ENTRY GetListEntryByIndex(IN PLIST_ENTRY head_entry, IN INT index)
{
	INT i;
	PLIST_ENTRY entry = head_entry->Flink;
	for (i = 0; i < index; i++) {
		if (entry == head_entry) {
			return NULL;
		} else {
			entry = entry->Flink;
		}
	}
	if (entry == head_entry) {
		return NULL;
	} else {
		return entry;
	}
}

INT GetListEntryIndex(IN PLIST_ENTRY head_entry, IN PLIST_ENTRY list_entry) 
{
	INT i = 0;
	PLIST_ENTRY curr_entry = head_entry->Flink;
	while (curr_entry != head_entry) {
		if (curr_entry == list_entry) {
			break;
		}
		curr_entry = curr_entry->Flink;
		i++;
	}
	return (curr_entry != head_entry) ? i : -1;
}