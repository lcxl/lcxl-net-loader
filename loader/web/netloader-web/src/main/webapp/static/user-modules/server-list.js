/**
 * 
 */
define(function(require, exports, module){
	require("jquery");
	var $=jQuery;
	var Handlebars = require("handlebars");
	//加载handlebars辅助器
	require("handlebars-helper");
	var tpl = require("text!template/server-list.htpl");
	var template = Handlebars.compile(tpl);
	
	function ServerList(select, data) {
		var context = this;
		context._data_ = data;
		context._select_ = select;
		$(select).html(template(data));
	}
	ServerList.prototype._select_ = null;
	ServerList.prototype._data_ = null;
	module.exports = ServerList;
});