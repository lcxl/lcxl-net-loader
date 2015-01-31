package com.lcxbox.netloader.host.model;

import org.codehaus.jackson.annotate.JsonProperty;
/**
 * 服务器相关信息
 * @author lcxl
 *
 */
public class Server {
	private Integer status;
	/**
	 * IP状态，开启了IPv4还是IPv6还是全开了
	 */
	@JsonProperty("ip_status")
	private Integer ipStatus;
	/**
	 * MAC地址
	 */
	@JsonProperty("mac_addr")
	private String macAddr;
	/**
	 * 备注
	 */
	private String comment;

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

	public String getComment() {
		return comment;
	}

	public void setComment(String comment) {
		this.comment = comment;
	}
	
	
}
