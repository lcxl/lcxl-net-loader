/**
 * 网卡页
 */
define(function(require, exports, module) {
	require("jquery");
	var $=jQuery;
	require("history");
	var BsDialog = require("bs-dialog");
	
	var ModuleList = require("module-list");
	var VirtualAddrSetting = require("virtual-addr-setting");
	
	function ModulePage(select, data) {
		$(select).html('<div class="module-list"></div>');
		//获取网卡列表
		$.getJSON("netloader/module_list.do").done(
			function(data) {
				//生成网卡列表
				var moduleList = new ModuleList(".module-list", data);
				//注册响应事件
				moduleList.onServerListBtnClick=function (moduleData) {
					//跳转到ServerList列表
					History.pushState({luid: moduleData.miniport_net_luid}, "服务器列表 - "+moduleData.miniport_friendly_name, "?server");
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
							virtualAddrSetting.ajaxSubmit("netloader/set_virtual_addr.do").done(function(data){
								//关闭对话框
								bsDialog.modal("hide");
								//将设置的结果同步到data中去
								virtualAddrSetting.syncToData();
								//刷新UI界面
								moduleList.refreshUI(moduleData.miniport_net_luid);
							}).fail(function (jqxhr, textStatus, error) {
								//提示出错信息
								bsDialog.errormsg("设置虚拟IP地址出错！");
							});
						}
					};
					bsDialog.modal("show");
					
				};
			}).fail(function( jqxhr, textStatus, error ) {
				var err = textStatus + ", " + error;
				console.log( "Request Failed: " + err );
			});
	};
	module.exports = ModulePage;
});