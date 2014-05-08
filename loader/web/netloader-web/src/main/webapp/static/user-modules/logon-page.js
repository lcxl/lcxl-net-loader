/**
 * 登录页面
 */
define(function(require, exports, module) {
	require("jquery");
	var $=jQuery;
	require("history");
	var Logon = require("logon");
	
	function LogonPage(select, data) {
		$(select).html('<div class="logon"></div>');
		var logon = new Logon(".logon");
		//按钮点击事件
		logon.OnLogonBtnClick = function(formsel) {
			$("#alert").hide();
			//序列化参数
			var formdata = $(formsel).serialize();
			$.post('user/logon.do', formdata, null, "json").done(function(data) {
				if (data.status != 0) {
					var errorTest = "未知状态码（"+data.status+"）";
					switch (data.status) {
					case 0x10:
						errorTest = "主机名称未知。";
						break;
					case 0x11:
						errorTest = "无法连接到主机。";
						break;
					}
					
					$("#alert-text").html("登录失败："+errorTest);
					$("#alert").show();
				} else {
					//后退
					History.back();
				}
			}).fail(function (jqxhr, textStatus, error) {
				$("#alert-text").html("登录失败，错误信息："+jqxhr.status+":"+ error);
				$("#alert").show();
			});
		};
		
	}
	module.exports = LogonPage;
});