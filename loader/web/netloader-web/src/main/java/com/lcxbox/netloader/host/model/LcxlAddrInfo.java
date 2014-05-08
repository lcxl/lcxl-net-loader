package com.lcxbox.netloader.host.model;

import org.codehaus.jackson.annotate.JsonProperty;

public class LcxlAddrInfo {
	public final static int SA_ENABLE_IPV4 = 0x01;//服务器启用了IPV4协议
	public final static int SA_ENABLE_IPV6 = 0x02;//服务器启用了IPV6协议
	private Integer status;
	private String ipv4;
	
	@JsonProperty("ipv4_onlink_prefix_length")
	private Integer ipv4OnlinkPrefixLength;
	
	private String ipv6;

	@JsonProperty("ipv6_onlink_prefix_length")
	private Integer ipv6OnlinkPrefixLength;

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

	public Integer getIpv4OnlinkPrefixLength() {
		return ipv4OnlinkPrefixLength;
	}

	public void setIpv4OnlinkPrefixLength(Integer ipv4OnlinkPrefixLength) {
		this.ipv4OnlinkPrefixLength = ipv4OnlinkPrefixLength;
	}

	public String getIpv6() {
		return ipv6;
	}

	public void setIpv6(String ipv6) {
		this.ipv6 = ipv6;
	}

	public Integer getIpv6OnlinkPrefixLength() {
		return ipv6OnlinkPrefixLength;
	}

	public void setIpv6OnlinkPrefixLength(Integer ipv6OnlinkPrefixLength) {
		this.ipv6OnlinkPrefixLength = ipv6OnlinkPrefixLength;
	}
	
	
	
	
}
