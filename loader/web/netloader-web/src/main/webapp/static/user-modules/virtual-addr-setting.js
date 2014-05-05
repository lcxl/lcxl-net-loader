/**
 * 
 */

define(function(require, exports, module){
	require("jquery");
	var $=jQuery;
	var Handlebars = require("handlebars");
	
	function VirtualAddrSetting(select, data) {
		var context = this;
		var tpl = require("text!template/virtual-addr-setting.htpl");
		var template = Handlebars.compile(tpl);
		$(select).html(template(data));
		context._select_ = select;
		context._data_ = data;
	}
	VirtualAddrSetting.prototype._select_ = null;
	VirtualAddrSetting.prototype._data_ = null;
	/**
	 * 提交post请求
	 * @param url
	 * @returns
	 */
	VirtualAddrSetting.prototype.ajaxSubmit = function(url) {
		var context = this;
		//序列化参数
		var formdata = $(context._select_+">form").serialize();
		return $.post(url, formdata, null, "json");
	};
	/**
	 * 将页面中的数据回写
	 */
	VirtualAddrSetting.prototype.syncToData = function() {
		var context = this;
		
		context._data_.virtual_addr.ipv4 = $(context._select_+">form").find("input[name='ipv4']").val();
		context._data_.virtual_addr.ipv4_onlink_prefix_length = $(context._select_+">form").find("input[name='ipv4_onlink_prefix_length']").val();
		context._data_.virtual_addr.ipv6 = $(context._select_+">form").find("input[name='ipv6']").val();
		context._data_.virtual_addr.ipv6_onlink_prefix_length = $(context._select_+">form").find("input[name='ipv6_onlink_prefix_length']").val();
		var enable_ipv4 = $(context._select_+">form").find("input[name='enable-ipv4']:checked").size();
		var enable_ipv6 = $(context._select_+">form").find("input[name='enable-ipv6']:checked").size();
		
		context._data_.virtual_addr.status = (enable_ipv4>0?1:0)|(enable_ipv6>0?2:0);
	};
	module.exports = VirtualAddrSetting;
});