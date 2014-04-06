/**
 * 
 */

define(function(require, exports, module){
	var $=jQuery;
	
	function Navbar(select, data) {
	
		//var handlebars = require("handlebars");
		var tpl = require("text!template/navbar.htpl");
		
		var template = require("handlebars").compile(tpl);
		$(select).html(template(data));
	}
	module.exports = Navbar;

});