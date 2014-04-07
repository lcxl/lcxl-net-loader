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
		context._data_ = data;
		
		var tpl = require("text!template/module-list.htpl");
		var template = Handlebars.compile(tpl);
		$(select).html(template(data));
		$(select).on("click", "[id^=virtual-addr-setting-]", function(event) {
			if (context._data_.module_list ==null) {
				return;
			}
			var luid = $(this).attr("id").substr(21);
			var module = context.findModuleByLuid(luid);
			if (module) {
				context.onVirtualAddrBtnClick(module);
			}
		});
		$(select).on("click", "[id^=server-list-setting-]", function(event) {
			if (context._data_.module_list ==null) {
				return;
			}
			var luid = $(this).attr("id").substr(20);
			var module = context.findModuleByLuid(luid);
			
			if (module) {
				context.onServerListBtnClick(module);	
			}
			
		});
		
	}
	ModuleList.prototype._data_ = null;
	ModuleList.prototype.findModuleByLuid = function(luid) {
		for (var i = 0; i < this._data_.module_list.length; i++) {
			if (this._data_.module_list[i].miniport_net_luid == luid) {
				return this._data_.module_list[i];
			}
		}
	};
	ModuleList.prototype.refreshUI = function (luid) {
		var module = this.findModuleByLuid(luid);
		if (module) {
			//设置ipv4
			$("#virtual-ipv4-td-"+luid).attr("class", (module.virtual_addr.status&SA_ENABLE_IPV4)?"success":"danger");
			$("#virtual-ipv4-td-"+luid+" span:first").html(module.virtual_addr.ipv4);
			$("#virtual-ipv4-td-"+luid+" span:last").html((module.virtual_addr.status&SA_ENABLE_IPV4)?"已启用":"已禁用");
			//设置ipv6
			$("#virtual-ipv6-td-"+luid).attr("class", (module.virtual_addr.status&SA_ENABLE_IPV6)?"success":"danger");
			$("#virtual-ipv6-td-"+luid+" span:first").html(module.virtual_addr.ipv6);
			$("#virtual-ipv6-td-"+luid+" span:last").html((module.virtual_addr.status&SA_ENABLE_IPV6)?"已启用":"已禁用");
		}
	};
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