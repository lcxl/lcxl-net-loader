/**
 * 服务器页
 */
define(function(require, exports, module) {
	require("jquery");
	var $=jQuery;
	require("history");
	var BsDialog = require("bs-dialog");
	
	var ServerList = require("server-list");
	
	function ServerPage(select, data) {
		$(select).html('<div class="server-list"></div>');
		
	}
	module.exports = ServerPage;
});