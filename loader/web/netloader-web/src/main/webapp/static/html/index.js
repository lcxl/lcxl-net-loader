/**
 * 
 */

define(function(require, exports, module) {
	require("jquery");
	require("bootstrap");
	
	var Narbar = require("navbar");
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
});