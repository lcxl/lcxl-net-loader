
define(function(require, exports, module) {
	require("jquery");
	require("cookie");
	var $=jQuery;
	require("history");
	require("bootstrap");
	var Narbar = require("navbar");
	
	//生成导航栏
	new Narbar(".my-narbar", {
		title : "LCXL Net Loader",
		navbarlist : [ {
			active : 1,
			url : "index.html",
			name : "首页"
		}, {
			url : "setting.html",
			name : "设置"
		}, {
			url : "about.html",
			name : "关于"
		}, ]
	});
	//his is the same as the onpopstate event except it does not fire for traditional anchors
	$(window).on('statechange',function(){
		// Do something, inspect History.getState() to decide what
		var state = History.getState();
		var modulestate = History.normalizeState({url:"?nav=index"});
		var serverstate = History.normalizeState({url:"?nav=server"});
		var logonstate = History.normalizeState({url:"?nav=logon"});
		//
		if ($.cookie("host") && $.cookie("port")) {
			if (state.url == modulestate.url) {
				var ModulePage = require("module-page");
				new ModulePage(".lcxlpage");
			} else	if (state.url == serverstate.url) {
				if (state.data.luid == null) {
					console.log('获取luid失败！');
					//回退
					History.back();
					return;
				}
				var ServerPage = require("server-page");
				new ServerPage(".lcxlpage", state.data);
			} else {
				//默认，跳转到网卡列表
				History.replaceState(null, "网卡列表", "?nav=index");
			}
		} else {
			//需要登录
			if (state.url == logonstate.url) {
				var LogonPage = require("logon-page");
				new LogonPage(".lcxlpage", state.data);
			} else {
				//
				History.pushState(null, "登录到服务器", "?nav=logon");
			}
		}
		
		console.log("statechange");
	});
	//this is the same as the onhashchange event except it does not fire for states
	$(window).on('anchorchange',function(){
		// Do something, inspect History.getState() to decide what
		var state = History.getState();
		console.log("anchorchange");
	});
	$(window).trigger('statechange');
	
});
