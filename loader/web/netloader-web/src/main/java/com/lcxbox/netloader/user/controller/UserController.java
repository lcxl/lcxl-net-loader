package com.lcxbox.netloader.user.controller;

import java.io.IOException;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.ResponseBody;

import com.lcxbox.netloader.user.model.LogonRespnse;
import com.lcxbox.netloader.user.service.IUserService;
/**
 * 用户控制类
 * @author lcxl
 *
 */
@Controller
@RequestMapping(value = "/user")
public class UserController {
	@Autowired
    private IUserService userService;
	/**
	 * 登录
	 * @param request
	 * @param response
	 * @param host
	 * @param port
	 * @param username
	 * @param password
	 * @param rememberMe
	 * @return
	 * @throws IOException
	 */
	@RequestMapping(value = "/logon.do", method=RequestMethod.POST)
	@ResponseBody
	public LogonRespnse logon(HttpServletRequest request, 
			HttpServletResponse response,
			@RequestParam("host") String host,
			@RequestParam("port") Integer port,
			@RequestParam(value="username", required=false, defaultValue = "") String username,
			@RequestParam(value="password", required=false, defaultValue = "") String password,
			@RequestParam(value="remember-me", required=false, defaultValue = "0") boolean rememberMe) throws IOException {
		LogonRespnse logonresponse = userService.logon(host, port, username, password);
		/*//ajax下设置cookie失效
		if (logonresponse.getStatus() == LcxlNetCode.JS_SUCCESS) {
			int cookieAge;
			
			if (rememberMe) {
				cookieAge = 60*60*24*7;
			} else {
				//游览器退出后清除cookie
				cookieAge = -1;
			}
			
			Cookie cookie;
			
			cookie= new Cookie("host",host);
		    cookie.setMaxAge(cookieAge);
		    response.addCookie(cookie);  
		    
		    cookie = new Cookie("port",port.toString());
		    cookie.setMaxAge(cookieAge);
		    response.addCookie(cookie);  
		    
		    cookie = new Cookie("username",username);
		    cookie.setMaxAge(cookieAge);
		    response.addCookie(cookie);  
		    
		    cookie = new Cookie("password",password);
		    cookie.setMaxAge(cookieAge);
		    response.addCookie(cookie);
		} 
		*/
		return logonresponse;
	}
}
