package com.lcxbox.netloader.host.model;

import org.codehaus.jackson.annotate.JsonProperty;

import com.lcxbox.common.model.CommonRequest;
/**
 * 移除服务器请求
 * @author lcxl
 *
 */
public class DelServerRequest extends CommonRequest {
	@JsonProperty("miniport_net_luid")
	private long miniportNetLuid; 
	
	@JsonProperty("mac_addr")
	private String macAddr;

	public long getMiniportNetLuid() {
		return miniportNetLuid;
	}

	public void setMiniportNetLuid(long miniportNetLuid) {
		this.miniportNetLuid = miniportNetLuid;
	}

	public String getMacAddr() {
		return macAddr;
	}

	public void setMacAddr(String macAddr) {
		this.macAddr = macAddr;
	}
	
	
}
