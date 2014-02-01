#ifndef _LCXL_HTTP_COMM_H_
#define _LCXL_HTTP_COMM_H_
#include <string>
#include <list>
struct _URLRec;
typedef struct _URLRec  URLRec, *PURLRec;

URLRec DecodeURL(const std::string &url);
std::string EncodeURL(const URLRec &url_rec, bool IsPostMethod);

struct _URLRec {
	// 协议名称，本类只支持HTTP
	std::string Scheme;
	// 如www.baidu.com
	std::string Host;
	// 端口号，默认为80
	int Port;
	// 查询路径，如/sss/sss/ssd.asp
	std::string Query;
	// 参数，如sss=ssd&sdfs=sdr，在POST方法中，此为空
	std::string Fragment;
public:
	_URLRec();
public:
	void SetQueryAndFragment(const std::string &Value);
	std::string GetQueryAndFragment();
public:
	_URLRec & operator = (const std::string &Source){
		*this = DecodeURL(Source);
		return *this;
	}
	std::string ToString(bool IsPostMethod = false) {
		return EncodeURL(*this, IsPostMethod);
	}
};

typedef struct _HeadRec {
	std::string Key;
	std::string Value;
} HeadRec, *PHeadRec;

class CHeadList :public std::list<HeadRec> {


	///	<summary>
	///	  保存到字符串中
	///	</summary>
	///	<param name="Separator">
	///	  分隔符，默认为": "（冒号加空格）
	///	</param>
	///	<param name="LineBreak">
	///	  换行符，默认为#13#10
	///	</param>
	///	<returns>
	///	  格式化好的请求/响应字符串列表，字符串末尾以换行符结尾
	///	</returns>
	std::string SaveToString(const std::string Separator, const std::string LineBreak);


	///	<summary>
	///	  从字符串加载请求/响应头
	///	</summary>
	///	<param name="AStr">
	///	  字符串，末尾需要以LinkBreak结尾
	///	</param>
	///	<param name="Separator">
	///	  分隔符，为了保证最大兼容性，此处应该只能是":"
	///	</param>
	///	<param name="LineBreak">
	///	  换行符，一般是#13#10
	///	</param>
	bool LoadFromString(const std::string AStr, const std::string Separator, const std::string LineBreak);

	std::string GetHeadItems(const std::string Index);
	void SetHeadItems(const std::string Index, const std::string Value);
};
#endif