#include "lcxl_iocp_lcxl.h"


BOOL _SendDataRec::Assign(PVOID _TotalData, ULONG _TotalLen)
{
	if (_TotalLen < sizeof(long) || *(long*)_TotalData != _TotalLen - sizeof(long)) {
		return FALSE;
	}
	mTotalData = _TotalData;
	mTotalLen = _TotalLen;

	mData = (byte*)mTotalData + sizeof(long);
	mDataLen = mTotalLen - sizeof(long);
	return TRUE;
}

void CLLSockLst::CreateSockObj(CSocketObj* &SockObj)
{
	SockObj = new CLLSockObj();
}

BOOL CLLSockObj::Init()
{
	BOOL resu;
	// 先调用父类的Init函数
	resu = CSocketObj::Init();
	// 设置为接收数据的长度
	mIsRecvAll = FALSE;
	mCurDataLen = 0;
	mBufLen = 1024;
	mBuf = malloc(mBufLen);
}

PVOID CLLSockObj::GetRecvData()
{
	if (!mIsRecvAll) {
		return NULL;
	} else {
		return mRecvData;
	}
}

long CLLSockObj::GetRecvDataLen()
{
	if (!mIsRecvAll) {
		return 0;
	} else {
		return mRecvDataLen;
	}
}

CLLSockObj::~CLLSockObj()
{
	if (mBuf != NULL) {
		free(mBuf);
	}
}

BOOL CLLSockObj::SendData(const SendDataRec &ASendDataRec)
{
	return CSocketObj::SendData(ASendDataRec.mTotalData, ASendDataRec.mTotalLen, TRUE);
}

BOOL CLLSockObj::SendData(PVOID Data, ULONG DataLen)
{
	SendDataRec SendRec;

	GetSendData(DataLen, SendRec);
	CopyMemory(SendRec.GetData(), Data, DataLen);
	return SendData(SendRec);
}

void CLLSockObj::GetSendData(ULONG DataLen, SendDataRec &ASendDataRec)
{
	/*
	var
	IsSuc: Boolean;
	begin
	SendDataRec.FTotalLen := DataLen + SizeOf(DataLen);
	SendDataRec.FTotalData := inherited GetSendData(SendDataRec.FTotalLen);
	PLongWord(SendDataRec.FTotalData)^ := DataLen;

	IsSuc := SendDataRec.Assgin(SendDataRec.FTotalData, SendDataRec.FTotalLen);
	Assert(IsSuc=True);*/
	ASendDataRec.mTotalLen = DataLen + sizeof(DataLen);
	ASendDataRec.mTotalData = CSocketObj::GetSendData(ASendDataRec.mTotalLen);
	*(PULONG)ASendDataRec.mTotalData = DataLen;

	BOOL IsSuc;
	
	IsSuc = ASendDataRec.Assign(ASendDataRec.mTotalData, ASendDataRec.mTotalLen);
	assert(IsSuc);
}

void CLLSockObj::FreeSendData(const SendDataRec &ASendDataRec)
{
	CSocketObj::FreeSendData(ASendDataRec.mTotalData);
}

void CCustomIOCPLCXLList::OnIOCPEvent(IocpEventEnum EventType, CSocketObj *SockObj, PIOCPOverlapped Overlapped)
{
	/*
	var
	LLSockObj: TLLSockObj absolute SockObj;
	begin
	case EventType of
	ieRecvAll:
	begin

	// 重新申请内存
	if LLSockObj.FCurDataLen + Overlapped.GetRecvDataLen > LLSockObj.FBufLen then
	begin
	LLSockObj.FBufLen := LLSockObj.FCurDataLen + Overlapped.GetRecvDataLen;
	ReallocMem(LLSockObj.FBuf, LLSockObj.FBufLen);
	end;
	CopyMemory(PByte(LLSockObj.FBuf) + LLSockObj.FCurDataLen, Overlapped.GetRecvData,
	Overlapped.GetRecvDataLen);
	LLSockObj.FCurDataLen := LLSockObj.FCurDataLen + Overlapped.GetRecvDataLen;
	while (LLSockObj.FCurDataLen >= SizeOf(LongWord)) and
	(PLongWord(LLSockObj.FBuf)^ >= LLSockObj.FCurDataLen - SizeOf(LongWord)) do
	begin

	LLSockObj.FRecvData := LLSockObj.FBuf;
	LLSockObj.FRecvDataLen := PLongWord(LLSockObj.FBuf)^ + SizeOf(LongWord);
	LLSockObj.FIsRecvAll := True;
	OnIOCPEvent(ieRecvAll, LLSockObj, Overlapped);

	LLSockObj.FIsRecvAll := False;
	MoveMemory(LLSockObj.FBuf, PByte(LLSockObj.FBuf) + LLSockObj.FRecvDataLen,
	LLSockObj.FCurDataLen - LLSockObj.FRecvDataLen);

	LLSockObj.FCurDataLen := LLSockObj.FCurDataLen - LLSockObj.FRecvDataLen;

	end;
	if LLSockObj.FCurDataLen > 0 then
	begin
	OnIOCPEvent(ieRecvPart, LLSockObj, Overlapped);
	end;
	end;
	else
	OnIOCPEvent(EventType, LLSockObj, Overlapped);
	end;
	*/
	CLLSockObj *LLSockObj = static_cast<CLLSockObj *>(SockObj);
	switch (EventType) {
	case ieRecvAll:
		// 重新申请内存
		if (LLSockObj->mCurDataLen + Overlapped->GetRecvDataLen() > LLSockObj->mBufLen) {
			LLSockObj->mBufLen = LLSockObj->mCurDataLen + Overlapped->GetRecvDataLen();
			LLSockObj->mBuf = realloc(LLSockObj->mBuf, LLSockObj->mBufLen);
		}
		CopyMemory((unsigned char*)LLSockObj->mBuf + LLSockObj->mCurDataLen, Overlapped->GetRecvData(),
			Overlapped->GetRecvDataLen());
		LLSockObj->mCurDataLen = LLSockObj->mCurDataLen + Overlapped->GetRecvDataLen();
		while (LLSockObj->mCurDataLen >= sizeof(ULONG) &&
			*(PULONG)LLSockObj->mBuf >= LLSockObj->mCurDataLen - sizeof(ULONG)) {


			LLSockObj->mRecvData = LLSockObj->mBuf;
			LLSockObj->mRecvDataLen = *(PULONG)LLSockObj->mBuf + sizeof(ULONG);
			LLSockObj->mIsRecvAll = TRUE;
			OnIOCPEvent(ieRecvAll, LLSockObj, Overlapped);

			LLSockObj->mIsRecvAll = FALSE;
			MoveMemory(LLSockObj->mBuf, (unsigned char*)LLSockObj->mBuf + LLSockObj->mRecvDataLen,
				LLSockObj->mCurDataLen - LLSockObj->mRecvDataLen);

			LLSockObj->mCurDataLen = LLSockObj->mCurDataLen - LLSockObj->mRecvDataLen;
		}
		if (LLSockObj->mCurDataLen > 0){
			OnIOCPEvent(ieRecvPart, LLSockObj, Overlapped);
		}
		break;
	default:
		OnIOCPEvent(EventType, LLSockObj, Overlapped);
		break;
	}
}

void CCustomIOCPLCXLList::OnIOCPEvent(IocpEventEnum EventType, CLLSockObj *SockObj, PIOCPOverlapped Overlapped)
{
	//nothing
}

void CIOCPLCXLList::OnIOCPEvent(IocpEventEnum EventType, CLLSockObj *SockObj, PIOCPOverlapped Overlapped)
{
	if (mIOCPEvent.IsAvaliable()) {
		TRIGGER_DELEGATE(mIOCPEvent)(EventType, SockObj, Overlapped);
	}
}

void CIOCPLCXLList::OnListenEvent(ListenEventEnum EventType, CSocketLst *SockLst)
{
	if (mListenEvent.IsAvaliable()) {
		TRIGGER_DELEGATE(mListenEvent)(EventType, static_cast<CLLSockLst*>(SockLst));
	}
}
