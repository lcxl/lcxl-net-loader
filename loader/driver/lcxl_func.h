#ifndef _LCXL_FUNC_H_
#define _LCXL_FUNC_H_
/*
author:
LCX
abstract:
 一些基本的函数
*/
#define TAG_FILE_BUFFER				'FISR'
//添加代码

PUNICODE_STRING LCXLNewString(IN PUNICODE_STRING sour);
VOID LCXLFreeString(PUNICODE_STRING dest);

__inline PUCHAR LCXLReadFromBuf(IN PUCHAR cur_buf, IN OUT PVOID data, IN INT datalen)
{
	RtlCopyMemory(data, cur_buf, datalen);
	return cur_buf + datalen;
}
//从缓冲区中读取字符串，注意，这里会调用LCXLNewString来生成str，当用完之后需要使用LCXLFreeString释放str
__inline PUCHAR LCXLReadStringFromBuf(IN PUCHAR cur_buf, OUT PUNICODE_STRING *str)
{
	UNICODE_STRING data;
	cur_buf = LCXLReadFromBuf(cur_buf, &data.Length, sizeof(data.Length));
	data.MaximumLength = data.Length;
	data.Buffer = (PWCH)cur_buf;
	*str = LCXLNewString(&data);
	return cur_buf + data.Length;
}
//跳过读取字符串
_inline PUCHAR LCXLSkipReadStringFromBuf(IN PUCHAR cur_buf)
{
	USHORT str_len;

	cur_buf = LCXLReadFromBuf(cur_buf, &str_len, sizeof(str_len));
	return cur_buf + str_len;
}
//写入数据到缓冲区中
__inline PUCHAR LCXLWriteToBuf(IN PUCHAR cur_buf, IN PVOID data, IN INT datalen)
{
	RtlCopyMemory(cur_buf, data, datalen);
	return cur_buf + datalen;
}

//写入字符串到缓冲区中
__inline PUCHAR LCXLWriteStringToBuf(IN PUCHAR cur_buf, IN PUNICODE_STRING data)
{
	if (data != NULL) {
		return LCXLWriteToBuf(LCXLWriteToBuf(cur_buf, &data->Length, sizeof(data->Length)), data->Buffer, data->Length);
	} else {
		USHORT Length = 0;
		return LCXLWriteToBuf(cur_buf, &Length, sizeof(Length));
	}
	
}

//VOID LCXLTransARP

#endif