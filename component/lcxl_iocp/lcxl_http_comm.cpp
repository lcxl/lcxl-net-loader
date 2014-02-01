#include "lcxl_http_comm.h"
#include "lcxl_string.h"
#include <assert.h>
URLRec DecodeURL(const std::string &url)
{
	URLRec resu;

	std::string tmpstr = url;
	size_t i = tmpstr.find("://");
	if (i != url.npos) {
		resu.Scheme = tmpstr.substr(0, i);
		tmpstr.erase(0, i + 3);
	} else {
		resu.Scheme = "http";
	}
	i = tmpstr.find("/");
	if (i != url.npos) {
		resu.Host = tmpstr.substr(0, i);
		tmpstr.erase(0, i);
		i = resu.Host.find(":");
		if (i != url.npos) {
			std::string port_str = resu.Host.substr(i + 1);
			try
			{
				resu.Port = std::stoi(port_str);
			}
			catch (...)
			{
				resu.Port = 80;
			}
			if (resu.Port < 0 || resu.Port > 65535) {
				resu.Port = 80;
			}
			resu.Host.erase(i);
		}
		resu.SetQueryAndFragment(tmpstr);
	} else {
		resu.Host  = tmpstr;
		resu.Port  = 80;
		resu.Query  = "/";
		resu.Fragment  = "";
	}
	return resu;
}

std::string EncodeURL(const URLRec &url_rec, bool IsPostMethod)
{
	std::string resu;

	return resu;
}

_URLRec::_URLRec()
{
	Port = 0;
}

void _URLRec::SetQueryAndFragment(const std::string &Value)
{
	std::string tmpstr = Value;
	if (tmpstr.empty()) {
		tmpstr = "/";
	}
	if (tmpstr[0] != '/') {
		throw std::exception("Invalid Parameter");
	}
	size_t i = tmpstr.find("?");
	if (i == tmpstr.npos) {
		Query = tmpstr;
		Fragment = "";
	} else {
		Query = tmpstr.substr(0, i - 1);
		Fragment = tmpstr.substr(i + 1);
	}
}

std::string _URLRec::GetQueryAndFragment()
{
	if (!Fragment.empty()) {
		return Query + "?" + Fragment;
	} else {
		return Query;
	}
}



std::string CHeadList::SaveToString(const std::string Separator, const std::string LineBreak)
{
	std::string resu;
	assert(!Separator.empty() && !LineBreak.empty());
	CHeadList::iterator it;
	for (it = this->begin(); it != this->end(); it++) {
		resu.append((*it).Key + Separator + (*it).Value + LineBreak);
	}
	return resu;
}

bool CHeadList::LoadFromString(const std::string AStr, const std::string Separator, const std::string LineBreak)
{
	size_t BIndex, EIndex;
	size_t I;
	std::string TmpStr;
	HeadRec NewHeadRec;

	if (AStr.empty() || Separator.empty() || LineBreak.empty() || AStr.length() >= LineBreak.length()) {
		return false;
	}
	clear();
	BIndex = 0;
	EIndex = AStr.find(LineBreak, BIndex);
	while (EIndex != AStr.npos){
		TmpStr = AStr.substr(BIndex, EIndex - BIndex);
		I = TmpStr.find(Separator);
		if (I != TmpStr.npos) {
			NewHeadRec.Key = trim(TmpStr.substr(0, I));
			NewHeadRec.Value = trim(TmpStr.substr(I + 1));
			push_back(NewHeadRec);
		}
		BIndex += LineBreak.length();
		EIndex = AStr.find(LineBreak, BIndex);
	}
	
	return true;
}

std::string CHeadList::GetHeadItems(const std::string Index)
{
	std::string resu;
	CHeadList::iterator it;
	for (it = begin(); it != end(); it++) {
		if (it->Key == Index) {
			resu = it->Value;
		}
		break;
	}
	return resu;
}

void CHeadList::SetHeadItems(const std::string Index, const std::string Value)
{
	bool Found = false;
	CHeadList::iterator it;
	for (it = begin(); it != end(); it++) {
		if (it->Key == Index) {
			it->Value = Value;
			Found = true;
			break;
		}
	}
	if (!Found) {
		HeadRec TmpHead = { Index , Value};
		push_back(TmpHead);
	}
}
