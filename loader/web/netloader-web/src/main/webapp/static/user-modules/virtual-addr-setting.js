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
	}
	VirtualAddrSetting.prototype._select_ = null;
	VirtualAddrSetting.prototype.ajaxSubmit = function(url) {
		var context = this;
		//序列化参数
		var formdata = $(context._select_+">form").serialize();
		return $.post(url, formdata, null, "json");
	};
	module.exports = VirtualAddrSetting;
});