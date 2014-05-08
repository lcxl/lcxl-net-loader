/**
 * 登录模块
 */
define(function(require, exports, module) {
	var $=jQuery;
	var Handlebars = require("handlebars");
	
	var tpl = require("text!template/logon.htpl");
	var template = Handlebars.compile(tpl);
	
	function Logon(select, data) {
		var context = this;
		context._data_ = data;
		context._select_ = select;
		$(select).html(template(data));
		
		$(select).on("click", "#logon-btn", function(event) {
			
			context.OnLogonBtnClick("#logon-form");
		});
	};
	
	Logon.prototype._select_ = null;
	Logon.prototype._data_ = null;
	Logon.prototype.OnLogonBtnClick =function(formsel) {
		
	};
	module.exports = Logon;
});