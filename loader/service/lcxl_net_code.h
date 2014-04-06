#ifndef _LCXL_NET_CODE_H_
#define _LCXL_NET_CODE_H_

#define JSON_CODE "code"
#define JC_NONE 0x00
//获取模块列表命令
#define JC_MODULE_LIST 0x01
//获取服务器列表
#define JC_SERVER_LIST 0x03
//设置虚拟IP地址
#define JC_SET_VIRTUAL_ADDR 0x04


#define JSON_DATA "data"

#define JSON_STATUS "status"
#define JS_SUCCESS 0x00
#define JS_FAIL 0x01
#define JS_JSON_DATA_NOT_FOUND 0x02
#define JS_JSON_CODE_NOT_FOUND 0x03

#endif