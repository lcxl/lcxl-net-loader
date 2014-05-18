/**
 * 用户操作类
 */

define(function(require, exports, module) {
	require("jquery");
	require("cookie");
	
	var $ = jQuery;
	
	
	function User(select, data) {
		
	}
	
	User.prototype.logon = function(userinfo) {
		
	};
	User.prototype.logout = function(userinfo) {
		
	};
	
	User.prototype.islogon = function() {
		var host = $.cookie("host");
		var port = $.cookie("port");
		return (host != null && port  != null);
	};
	
	module.exports=User;
});