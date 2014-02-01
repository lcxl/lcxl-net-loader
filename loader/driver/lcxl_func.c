#include "precomp.h"

PUNICODE_STRING LCXLNewString(IN PUNICODE_STRING sour)
{
	PUNICODE_STRING resu;

	resu = ExAllocatePoolWithTag(NonPagedPool, sour->MaximumLength + sizeof(UNICODE_STRING), TAG_FILE_BUFFER);
	if (resu != NULL) {
		resu->Buffer = (PWCH)((PUCHAR)resu + sizeof(UNICODE_STRING));
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
