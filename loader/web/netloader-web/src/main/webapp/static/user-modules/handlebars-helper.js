/**
 * 
 */
define(function(require, exports, module){
	var Handlebars = require("handlebars");
	
	/*
	 * #define SA_ENABLE_IPV4	0x01//服务器启用了IPV4协议
	 * #define SA_ENABLE_IPV6	0x02//服务器启用了IPV6协议
	 */
	var SA_ENABLE_IPV4 = 0x01;
	var SA_ENABLE_IPV6 = 0x02;
	/**
	 * 注册模版帮助类
	 */
	/**
	 * 是否启用了虚拟IPv6地址
	 */
	Handlebars.registerHelper('ifEnableIpv6', function(ipStatus, options) {
		  if(ipStatus & SA_ENABLE_IPV6) {
			  return options.fn(this);
		  } else {
			  return options.inverse(this);
		  }
		});
	/**
	 * 是否启用了虚拟IPv4地址
	 */
	Handlebars.registerHelper('ifEnableIpv4', function(ipStatus, options) {
		  if(ipStatus & SA_ENABLE_IPV4) {
			  return options.fn(this);
		  } else {
			  return options.inverse(this);
		  }
		});
	
	/**
	 * 获取路由算法的名称
	 */
	Handlebars.registerHelper('raName', function(ra, options) {
		switch(ra) {
			case 0x00://RA_POLL
				return "轮询算法";
			case 0x01://RA_IP_HASH
				return "IP Hash算法";
			case 0x10://RA_LEAST_CONNECTION
				return "最小连接数算法";
			case 0x11://RA_FAST_RESPONSE
				return "最快响应时间算法";
			default:
				return "未知算法("+ra+")";
		};
	});
	/**
	 * 获得角色名称
	 */
	Handlebars.registerHelper('roleName', function(role, options) {
		switch(role) {
			case 0://LCXL_ROLE_UNKNOWN
				return "未配置";
			case 1://LCXL_ROLE_ROUTER
				return "负载均衡器";
			case 2://LCXL_ROLE_SERVER
				return "服务器";
			default:
				return "未知角色("+role+")";	
		};
	});
	/**
	 * 是否是负载均衡器
	 */
	Handlebars.registerHelper('ifRouter', function(role, options) {
		if(role == 1) {
			  return options.fn(this);
		} else {
			  return options.inverse(this);
		}
	});
	/**
	 * 是否是后端服务器
	 */
	Handlebars.registerHelper('ifServer', function(role, options) {
		if(role == 2) {
			  return options.fn(this);
		} else {
			  return options.inverse(this);
		}
	});
	
	var SS_ONLINE = 0x01;
	var SS_CHECKING = 0x02;
	var SS_DELETED = 0x80;
	/**
	 * 后端服务器是否在线
	 */
	Handlebars.registerHelper('ifServerOnline', function(status, options) {
		if((status & SS_ONLINE) != 0) {
			  return options.fn(this);
		} else {
			  return options.inverse(this);
		}
	});
	/**
	 * 后端服务器当前正在检查可用性
	 */
	Handlebars.registerHelper('ifServerChecking', function(status, options) {
		if((status & SS_CHECKING) != 0) {
			  return options.fn(this);
		} else {
			  return options.inverse(this);
		}
	});
	/**
	 * 后端服务器是否已经被删除
	 */
	Handlebars.registerHelper('ifServerDeleted', function(status, options) {
		if((status & SS_DELETED) != 0) {
			  return options.fn(this);
		} else {
			  return options.inverse(this);
		}
	});
});