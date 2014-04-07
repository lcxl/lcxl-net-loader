/**
 * Bootstrap对话框，进一步封装
 */

define(function(require, exports, module) {
	require("jquery");
	require("bootstrap");
	var $=jQuery;
	function BsDialog(select, data) {
		var context=this;
		//var handlebars = require("handlebars");
		var tpl = require("text!template/bs-dialog.htpl");
		
		var template = require("handlebars").compile(tpl);
		$(select).html(template(data));
		context._select_ = select;
		context.bodytag = select+" .modal-body";
		$(select+">div").on("click", ".modal-footer button", function(event) {
			console.log(this);
			context.onButtonClick(this, data);
		});
	}
	/**
	 * 保存选择器字符串
	 * @param options
	 */
	BsDialog.prototype._select_ = null;
	BsDialog.prototype.bodytag = null;
	
	BsDialog.prototype.modal = function(options) {
		$(this._select_).find(".modal").modal(options);
	};
	BsDialog.prototype.errormsg = function(msg) {
		if (msg==null ||msg=="") {
			$(this.bodytag+" .error_msg").empty();
		} else {
			$(this.bodytag+" .error_msg").html('<div class="alert alert-warning alert-dismissable">\
					  <button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>'
					+msg+
					'</div>');
		}
	}
	BsDialog.prototype.onButtonClick = function (html, data) {
		
	};
	module.exports=BsDialog;
});