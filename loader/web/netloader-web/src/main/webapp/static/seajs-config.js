//seajs配置文件
//Author: LCXL(lcx87654321@163.com)
seajs.config({
    //base: "",
    alias: {
      "seajs-text":"sea-plugins/seajs-text.js",
      
      "jquery": "sea-modules/jquery/jquery-1.10.1.js",
      "history": "sea-modules/history/jquery.history.js",
      "cookie": "sea-modules/cookie/jquery.cookie-1.4.1.min.js",
	  "bootstrap": "sea-modules/bootstrap/bootstrap.js",
	  "bootstrap-css": "sea-modules/bootstrap/css/bootstrap.css",
	  "bootstrap-theme-css": "sea-modules/bootstrap/css/bootstrap-theme.css",
	  "handlebars":"sea-modules/handlebars/handlebars-v1.3.0.js",
	  
	  //"catalog":"user-modules/catalog.js",
	  "handlebars-helper":"user-modules/handlebars-helper.js",
	  "navbar":"user-modules/navbar.js",
	  "bs-dialog":"user-modules/bs-dialog.js",
	  "bs-msgbox":"user-modules/bs-msgbox.js",
	  
	  "index":"html/index.js",
	  
	  "module-page":"user-modules/module-page.js",
	  "module-list":"user-modules/module-list.js",
	  "virtual-addr-setting":"user-modules/virtual-addr-setting.js",
	  
	  "server-page":"user-modules/server-page.js",
	  "server-list":"user-modules/server-list.js",
	  
	  "logon-page":"user-modules/logon-page.js",
	  "logon":"user-modules/logon.js"
	  
    },
    preload:[
             "seajs-text",
             ]
  });