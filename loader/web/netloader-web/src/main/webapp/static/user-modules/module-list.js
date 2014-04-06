/**
 * 
 */

define(function(require, exports, module){
	
	
	var $=jQuery;
	var Handlebars = require("handlebars");
	
	/*
	 * #define SA_ENABLE_IPV4	0x01//服务器启用了IPV4协议
	 * #define SA_ENABLE_IPV6	0x02//服务器启用了IPV6协议
	 */
	var SA_ENABLE_IPV4 = 0x01;
	var SA_ENABLE_IPV6 = 0x02;
	
	/**
	 * 注册模版帮助类
	 */
	Handlebars.registerHelper('ifEnableIpv6', function(ipStatus, options) {
		  if(ipStatus & SA_ENABLE_IPV6) {
			  return options.fn(this);
		  } else {
			  return options.inverse(this);
		  }
		});
	Handlebars.registerHelper('ifEnableIpv4', function(ipStatus, options) {
		  if(ipStatus & SA_ENABLE_IPV4) {
			  return options.fn(this);
		  } else {
			  return options.inverse(this);
		  }
		});
	function ModuleList(select, data) {
		var context = this;
		var tpl = require("text!template/module-list.htpl");
			
		var template = Handlebars.compile(tpl);
		$(select).html(template(data));
		$(select).on("click", "[id^=virtual-addr-setting-]", function(event) {
			if (data.data ==null) {
				return;
			}
			var luid = $(this).attr("id").substr(21);
			for (var i = 0; i < data.data.length; i++) {
				if (data.data[i].miniport_net_luid == luid) {
					context.onVirtualAddrBtnClick(data.data[i]);
					break;
				}
			}
		});
		$(select).on("click", "[id^=server-list-setting-]", function(event) {
			if (data.data ==null) {
				return;
			}
			var luid = $(this).attr("id").substr(20);
			for (var i = 0; i < data.data.length; i++) {
				if (data.data[i].miniport_net_luid == luid) {
					context.onServerListBtnClick(data.data[i]);
					break;
				}
			}
		});
		
	}
	/**
	 * 定义一个默认的服务列表按钮点击事件
	 */
	ModuleList.prototype.onServerListBtnClick = function (moduleData) {
		console.log(moduleData);
	};
	/**
	 * 定义一个默认的虚拟IP按钮点击事件
	 */
	ModuleList.prototype.onVirtualAddrBtnClick = function (moduleData) {
		console.log(moduleData);
	};
	module.exports = ModuleList;
});