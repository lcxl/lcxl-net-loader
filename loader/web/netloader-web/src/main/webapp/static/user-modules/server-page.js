/**
 * 服务器页
 */
define(function(require, exports, module) {
	require("jquery");
	var $=jQuery;
	require("history");
	var BsDialog = require("bs-dialog");
	var Handlebars = require("handlebars");
	var ServerList = require("server-list");
	var tpl = require("text!template/server-page.htpl");
	var template = Handlebars.compile(tpl);
	/**
	 * @param data 格式如下：{luid: xxxx}
	 */
	function ServerPage(select, data) {
		var context = this;
		context._select_ = select;
		context._data_ = data;
		
		$(select).html(template(data));
		/**
		 * 后退
		 */
		$(select).on("click", "[id=btn-back]", function(event) {
			History.back();
		});
		$(select).on("click", "[id=btn-add-server]", function(event) {
			
		});
		$(select).on("click", "[id=btn-del-server]", function(event) {
	
		});
		/**
		 * 刷新页面
		 */
		$(select).on("click", "[id=btn-refresh]", function(event) {
			window.location.reload();
		});
		//获取网卡列表
		$.getJSON("netloader/server_list.do", {miniport_net_luid: data.luid}).done(
			function(data) {
				/**
				 * data的格式为{code:xxx; server_list: [{status:xxx; ip_status:xxx, mac_addr:xxx, comment:xxx}, ...], status:xxx}
				 */
				new ServerList(".server-list", data);
			}
		).fail(
			function( jqxhr, textStatus, error ) {
				new ServerList(".server-list", data);
				var err = textStatus + ", " + error;
				console.log( "Request Failed: " + err );
			}
		);
	}
	ServerPage.prototype._select_ = null;
	ServerPage.prototype._data_ = null;
	module.exports = ServerPage;
});