/**
 * 消息框
 * lcxl
 */

define(function(require, exports, module) {
	var BsDialog = require("bs-dialog");
	require("jquery");
	var $ = jQuery;
	
	function BsMsgBox(select, data) {
		var context=this;
		
		context.bs_dialog = new BsDialog(select, dlgdata);
		context.bs_dialog.onButtonClick = function(html, data) {
			if (typeof(context.onButtonClick) == "function") {
				context.onButtonClick(html,data);
				//关闭对话框
				context.bs_dialog.modal('hide');
			}
		};
		context.bs_dialog.modal('show');
	}
	BsMsgBox.prototype.bs_dialog = null;
	BsMsgBox.prototype.onButtonClick = function (html, data) {
		
	};
	module.exports=BsMsgBox;
});