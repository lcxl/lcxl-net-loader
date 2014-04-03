package com.lcxbox.netloader.router.model;

import org.codehaus.jackson.annotate.JsonProperty;

public class Server {
	private Integer status;
	
	@JsonProperty("ip_status")
	private Integer ipStatus;
	
	@JsonProperty("mac_addr")
	private String macAddr;

	public Integer getStatus() {
		return status;
	}

	public void setStatus(Integer status) {
		this.status = status;
	}

	public Integer getIpStatus() {
		return ipStatus;
	}

	public void setIpStatus(Integer ipStatus) {
		this.ipStatus = ipStatus;
	}

	public String getMacAddr() {
		return macAddr;
	}

	public void setMacAddr(String macAddr) {
		this.macAddr = macAddr;
	}
	
	
}
