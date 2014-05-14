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
