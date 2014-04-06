package com.lcxbox.netloader.model;

public class LcxlAddrInfo {
	public final static int SA_ENABLE_IPV4 = 0x01;//服务器启用了IPV4协议
	public final static int SA_ENABLE_IPV6 = 0x02;//服务器启用了IPV6协议
	private Integer status;
	private String ipv4;
	private String ipv6;

	public Integer getStatus() {
		return status;
	}

	public void setStatus(Integer status) {
		this.status = status;
	}

	public String getIpv4() {
		return ipv4;
	}

	public void setIpv4(String ipv4) {
		this.ipv4 = ipv4;
	}

	public String getIpv6() {
		return ipv6;
	}

	public void setIpv6(String ipv6) {
		this.ipv6 = ipv6;
	}
	
	
}
