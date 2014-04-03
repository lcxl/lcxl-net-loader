/**
 * 
 */

define(function(require, exports, module){
	function Navbar(select, data) {
		(function($){
			//var handlebars = require("handlebars");
			var tpl = require("template/navbar.tpl");
			
			var template = require("handlebars").compile(tpl);
			$(select).html(template(data));
			
		})(jQuery);
	}
	module.exports = Navbar;
});