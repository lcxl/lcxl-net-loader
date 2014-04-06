//seajs配置文件
seajs.config({
    //base: "",
    alias: {
      "seajs-text":"sea-plugins/seajs-text.js",
      
      "jquery": "sea-modules/jquery/jquery-1.10.1.js",
	  "bootstrap": "sea-modules/bootstrap/bootstrap.js",
	  "bootstrap-css": "sea-modules/bootstrap/css/bootstrap.css",
	  "bootstrap-theme-css": "sea-modules/bootstrap/css/bootstrap-theme.css",
	  "handlebars":"sea-modules/handlebars/handlebars-v1.3.0.js",
	  
	  //"catalog":"user-modules/catalog.js",
	  "navbar":"user-modules/navbar.js",
	  "module-list":"user-modules/module-list.js",
	  "bs-dialog":"user-modules/bs-dialog.js",
	  "virtual-addr-setting":"user-modules/virtual-addr-setting.js",
	  
	  "index":"html/index.js",
	  //"mysql-cluster":"html/mysql-cluster.js",
	  //"qingcloud":"html/qingcloud.js",
	  
    },
    preload:[
             "seajs-text",
             ],
  });