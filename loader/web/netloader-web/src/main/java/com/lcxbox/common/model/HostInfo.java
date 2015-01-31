package com.lcxbox.common.model;
/**
 * LCXL Netloader服务所在主机的相关信息
 * @author lcxl
 *
 */
public class HostInfo {
	private String host;
	private Integer port;
	private String username;
	private String password;
	public String getHost() {
		return host;
	}
	public void setHost(String host) {
		this.host = host;
	}
	public Integer getPort() {
		return port;
	}
	public void setPort(Integer port) {
		this.port = port;
	}
	public String getUsername() {
		return username;
	}
	public void setUsername(String username) {
		this.username = username;
	}
	public String getPassword() {
		return password;
	}
	public void setPassword(String password) {
		this.password = password;
	}
	
	
}
