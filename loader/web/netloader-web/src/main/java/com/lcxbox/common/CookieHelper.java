package com.lcxbox.common;

import javax.servlet.http.Cookie;

import com.lcxbox.common.model.HostInfo;

public class CookieHelper {
	public static HostInfo getHostInfo(Cookie[] cookies) {
		HostInfo info = new HostInfo();
		for (Cookie cookie: cookies) {
			if (cookie.getName().equals("host")) {
				info.setHost(cookie.getValue());
			} else if (cookie.getName().equals("port"))  {
				info.setPort(Integer.decode(cookie.getValue()));
			} else if (cookie.getName().equals("username"))  {
				info.setUsername(cookie.getValue());
			} else if (cookie.getName().equals("password"))  {
				info.setPassword(cookie.getValue());
			}
		}
		return info;
		
	}
}
