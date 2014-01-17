#include "lcxl_iocp_cmd.h"

WORD _CMDDataRec::GetCMD()
{
return *(PWORD)((unsigned char*)mTotalData + sizeof(ULONG));
}

void _CMDDataRec::SetCMD(const WORD Value)
{
	*(PWORD)((unsigned char*)mTotalData + sizeof(ULONG)) = Value;
}

BOOL _CMDDataRec::Assign(PVOID _TotalData, ULONG _TotalLen)
{
	if (_TotalLen < sizeof(ULONG)+sizeof(WORD) || _TotalData == NULL) {
		return FALSE;
	}
	if (*(PULONG)_TotalData != _TotalLen - sizeof(ULONG)) {
		return FALSE;
	}
	mTotalData = _TotalData;
	mTotalLen = _TotalLen;

	mData = (unsigned char*)mTotalData + sizeof(mDataLen)+
		sizeof(WORD);
	mDataLen = mTotalLen - sizeof(mDataLen)-sizeof(WORD);
	return TRUE;
}

void CCmdSockLst::CreateSockObj(CSocketObj* &SockObj)
{
SockObj = new CCmdSockObj();
}

BOOL CCmdSockObj::SendData(const CMDDataRec ASendDataRec)
{
return CSocketObj::SendData(ASendDataRec.mTotalData, ASendDataRec.mTotalLen, TRUE);
}

BOOL CCmdSockObj::SendData(WORD CMD, PVOID Data, ULONG DataLen)
{
	
	CMDDataRec SendRec;

	GetSendData(DataLen, SendRec);
	CopyMemory(SendRec.GetData(), Data, DataLen);
	SendRec.SetCMD(CMD);
	BOOL resu = SendData(SendRec);
	if (!resu) {
		OutputDebugStr(_T("TCmdSockObj.SendData Failed!"));
		FreeSendData(SendRec);
	}
	return resu;
}

BOOL CCmdSockObj::SendData(WORD CMD, PVOID Data[], ULONG DataLen[], INT DataCount)
{
	INT I;
	ULONG TotalDataLen = 0;
	CMDDataRec SendRec;
	unsigned char *DataPos;

	for (I = 0; I < DataCount; I++) {
		TotalDataLen += DataLen[I];
	}
	GetSendData(TotalDataLen, SendRec);
	DataPos = (unsigned char *)SendRec.GetData();
	for (I = 0; I < DataCount; I++) {
		CopyMemory(DataPos, Data[I], DataLen[I]);
		DataPos += DataLen[I];
	}
	SendRec.SetCMD(CMD);
	return SendData(SendRec);
}

void CCmdSockObj::GetSendData(ULONG DataLen, CMDDataRec &ASendDataRec)
{
	BOOL IsSuc;

	ASendDataRec.mTotalLen = DataLen + sizeof(DataLen)+sizeof(WORD);
	ASendDataRec.mTotalData = CSocketObj::GetSendData(ASendDataRec.mTotalLen);
	*(PULONG)ASendDataRec.mTotalData = DataLen + sizeof(WORD);

	IsSuc = ASendDataRec.Assign(ASendDataRec.mTotalData, ASendDataRec.mTotalLen);
	assert(IsSuc = TRUE);
}

void CCmdSockObj::FreeSendData(const CMDDataRec &ASendDataRec)
{
	CSocketObj::FreeSendData(ASendDataRec.mTotalData);
}

void CCmdSockObj::GetSendDataFromOverlapped(PIOCPOverlapped Overlapped, CMDDataRec &ASendDataRec)
{
	assert(Overlapped->OverlappedType == otSend);
	ASendDataRec.Assign(Overlapped->SendData, Overlapped->SendDataLen);
}

void CIOCPCMDList::OnIOCPEvent(IocpEventEnum EventType, CLLSockObj *SockObj, PIOCPOverlapped Overlapped)
{
	if (mIOCPEvent.IsAvaliable()) {
		TRIGGER_DELEGATE(mIOCPEvent)(EventType, static_cast<CCmdSockObj*>(SockObj), Overlapped);
	}
}

void CIOCPCMDList::OnListenEvent(ListenEventEnum EventType, CSocketLst *SockLst)
{
	if (mListenEvent.IsAvaliable()) {
		TRIGGER_DELEGATE(mListenEvent)(EventType, static_cast<CCmdSockLst*>(SockLst));
	}
}
