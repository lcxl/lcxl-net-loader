/**
 * 
 */

define(function(require, exports, module){
	
	require("jquery");
	var $=jQuery;
	var Handlebars = require("handlebars");
	//加载handlebars辅助器
	require("handlebars-helper");
	
	
	var tpl = require("text!template/module-list.htpl");
	var template = Handlebars.compile(tpl);
	
	function ModuleList(select, data) {
		var context = this;
		context._data_ = data;
		context._select_ = select;
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
	ModuleList.prototype._select_ = null;
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
			$("#virtual-ipv4-td-"+luid+" span:first").html(module.virtual_addr.ipv4+"("+module.virtual_addr.ipv4_onlink_prefix_length+")");
			$("#virtual-ipv4-td-"+luid+" span:last").html((module.virtual_addr.status&SA_ENABLE_IPV4)?"已启用":"已禁用");
			//设置ipv6
			$("#virtual-ipv6-td-"+luid).attr("class", (module.virtual_addr.status&SA_ENABLE_IPV6)?"success":"danger");
			$("#virtual-ipv6-td-"+luid+" span:first").html(module.virtual_addr.ipv6+"("+module.virtual_addr.ipv6_onlink_prefix_length+")");
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