package com.lcxbox.netloader.model;

import org.codehaus.jackson.annotate.JsonProperty;
import org.codehaus.jackson.map.annotate.JsonSerialize;

@JsonSerialize(include=JsonSerialize.Inclusion.NON_EMPTY)
public class SetVirtualAddrRequest extends CommonRequest {
	@JsonProperty("miniport_net_luid")
	private long miniportNetLuid; 
	
	@JsonProperty("virtual_addr")
	private LcxlAddrInfo virtualAddr;
	
	public long getMiniportNetLuid() {
		return miniportNetLuid;
	}
	public void setMiniportNetLuid(long miniportNetLuid) {
		this.miniportNetLuid = miniportNetLuid;
	}
	public LcxlAddrInfo getVirtualAddr() {
		return virtualAddr;
	}
	public void setVirtualAddr(LcxlAddrInfo virtualAddr) {
		this.virtualAddr = virtualAddr;
	}
	
	
}
