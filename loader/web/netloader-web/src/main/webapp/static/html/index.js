
define(function(require, exports, module) {
	require("jquery");
	var $=jQuery;
	require("bootstrap");
	var Narbar = require("navbar");
	var ModuleList = require("module-list");
	var BsDialog = require("bs-dialog");
	var VirtualAddrSetting = require("virtual-addr-setting");
	//生成导航栏
	new Narbar(".my-narbar", {
		title : "LCXL Net Loader",
		navbarlist : [ {
			active : 1,
			url : "index.html",
			name : "首页"
		}, {
			url : "mysql-cluster.html",
			name : "MySQL集群"
		}, {
			url : "warning-system.html",
			name : "告警系统"
		}, ]
	});
	
	
	//获取网卡列表
	$.getJSON("router/module_list.do").done(
		function(data) {
			//生成网卡列表
			var moduleList = new ModuleList(".module-list", data);
			//注册响应事件
			moduleList.onServerListBtnClick=function (moduleData) {
				//生成模态对话框
				var bsDialog = new BsDialog(".bs-dialog", {
					id:"myModal",
					title:"服务器列表 - "+moduleData.miniport_friendly_name,
					body:"",
				});
				bsDialog.modal("show");
			};
			moduleList.onVirtualAddrBtnClick=function (moduleData) {
				//生成模态对话框
				var bsDialog = new BsDialog(".bs-dialog", {
					id:"myModal",
					title:"虚拟IP设置 - "+moduleData.miniport_friendly_name,
					body:"",
					buttons:[ {
						dismiss:true,
						text:"关闭",
					},{
						id:"virtual-addr-setting-ok",
						text:"确定",
						btncls:"btn-primary",
					},],
				});
				var virtualAddrSetting = new VirtualAddrSetting(bsDialog.bodytag, {
					miniport_net_luid:	moduleData.miniport_net_luid,
					virtual_addr:moduleData.virtual_addr,
				});
				//监听按钮点击事件
				bsDialog.onButtonClick = function(html, data) {
					if ($(html).attr("id")=="virtual-addr-setting-ok") {
						virtualAddrSetting.ajaxSubmit("");
					}
				};
				bsDialog.modal("show");
				
			};
		}).fail(function( jqxhr, textStatus, error ) {
			var err = textStatus + ", " + error;
			console.log( "Request Failed: " + err );
		});
	
	
});
