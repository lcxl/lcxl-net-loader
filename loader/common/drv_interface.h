/************************************************************************/
/* 驱动接口定义文件                                                     */
/* Author: LCXL                                                         */
/************************************************************************/
#ifndef _DRV_INTERFACE_H_
#define _DRV_INTERFACE_H_

#include "drv_interface_type.h"
//定义NDIS控制码
#define _NDIS_CONTROL_CODE(request,method) \
	CTL_CODE(FILE_DEVICE_PHYSICAL_NETCARD, request, method, FILE_ANY_ACCESS)
//重启模块
#define IOCTL_RESTART_MODULE				_NDIS_CONTROL_CODE(0x01, METHOD_BUFFERED)

//添加代码
//获取角色
//#define IOCTL_GET_ROLE						_NDIS_CONTROL_CODE(0x20, METHOD_BUFFERED)
//设置角色
#define IOCTL_SET_ROLE						_NDIS_CONTROL_CODE(0x21, METHOD_BUFFERED)

//获取所有的网卡接口序号
#define IOCTL_GET_MODULE_LIST				_NDIS_CONTROL_CODE(0x22, METHOD_BUFFERED)
//input NET_LUID                     miniport_net_luid;
//LCXL_IP
//设置虚拟IP
#define IOCTL_SET_VIRTUAL_ADDR				_NDIS_CONTROL_CODE(0x23, METHOD_BUFFERED)
//LCXL_IP
//获取后端服务器列表
//input APP_IP
//output array of LCXL_SERVER
#define IOCTL_ROUTER_GET_SERVER_LIST		_NDIS_CONTROL_CODE(0x24, METHOD_BUFFERED)
//添加或更新后端服务器
//input APP_ADD_SERVER
#define IOCTL_ROUTER_ADD_SERVER				_NDIS_CONTROL_CODE(0x25, METHOD_BUFFERED)
//删除后端服务器
//input APP_DEL_SERVER
#define IOCTL_ROUTER_DEL_SERVER				_NDIS_CONTROL_CODE(0x26, METHOD_BUFFERED)
//设置服务器检测设置
#define IOCTL_ROUTER_SET_SERVER_CHECK		_NDIS_CONTROL_CODE(0x27, METHOD_BUFFERED)
//设置负载均衡算法
#define IOCTL_ROUTER_SET_ROUTING_ALGORITHM	_NDIS_CONTROL_CODE(0x28, METHOD_BUFFERED)
//!添加代码!

#endif