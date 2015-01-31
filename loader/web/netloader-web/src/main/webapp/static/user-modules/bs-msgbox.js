/**
 * 消息框
 * lcxl
 */

define(function(require, exports, module) {
	var BsDialog = require("bs-dialog");
	require("jquery");
	var $ = jQuery;
	
	function BsMsgBox(select, data) {
		//调用父类的构造器
		BsDialog.call(this, select, data);
		this.modal('show');
	}
	//继承父类
	BsMsgBox.prototype = new BsDialog();
	BsMsgBox.prototype.onButtonClick = function (html, data) {
		//关闭对话框
		this.modal('hide');
		
	};
	module.exports=BsMsgBox;
});