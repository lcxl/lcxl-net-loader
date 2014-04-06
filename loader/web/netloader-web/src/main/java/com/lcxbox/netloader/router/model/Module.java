package com.lcxbox.netloader.router.model;

import org.codehaus.jackson.annotate.JsonProperty;

public class Module {
	
	@JsonProperty("miniport_ifindex")
	private Integer miniportIfindex;
	
	@JsonProperty("miniport_net_luid")
	private Long miniportNetLuid; 
	
	@JsonProperty("virtual_addr")
	private LcxlAddrInfo virtualAddr;
	
	@JsonProperty("mac_addr")
	private String macAddr;
	
	@JsonProperty("filter_module_name")
	private String filterModuleName;
	
	@JsonProperty("miniport_friendly_name")
	private String miniportFriendlyName;
	
	@JsonProperty("miniport_name")
	private String miniportName;

	@JsonProperty("server_count")
	private Integer serverCount;
	
	@JsonProperty("router_mac_addr")
	private String routerMacAddr;
	
	public Integer getMiniportIfindex() {
		return miniportIfindex;
	}

	public void setMiniportIfindex(Integer miniportIfindex) {
		this.miniportIfindex = miniportIfindex;
	}

	public Long getMiniportNetLuid() {
		return miniportNetLuid;
	}

	public void setMiniportNetLuid(Long miniportNetLuid) {
		this.miniportNetLuid = miniportNetLuid;
	}

	public LcxlAddrInfo getVirtualAddr() {
		return virtualAddr;
	}

	public void setVirtualAddr(LcxlAddrInfo virtualAddr) {
		this.virtualAddr = virtualAddr;
	}

	public String getMacAddr() {
		return macAddr;
	}

	public void setMacAddr(String macAddr) {
		this.macAddr = macAddr;
	}

	public String getFilterModuleName() {
		return filterModuleName;
	}

	public void setFilterModuleName(String filterModuleName) {
		this.filterModuleName = filterModuleName;
	}

	public String getMiniportFriendlyName() {
		return miniportFriendlyName;
	}

	public void setMiniportFriendlyName(String miniportFriendlyName) {
		this.miniportFriendlyName = miniportFriendlyName;
	}

	public String getMiniportName() {
		return miniportName;
	}

	public void setMiniportName(String miniportName) {
		this.miniportName = miniportName;
	}

	public Integer getServerCount() {
		return serverCount;
	}

	public void setServerCount(Integer serverCount) {
		this.serverCount = serverCount;
	}

	public String getRouterMacAddr() {
		return routerMacAddr;
	}

	public void setRouterMacAddr(String routerMacAddr) {
		this.routerMacAddr = routerMacAddr;
	}

	
}
