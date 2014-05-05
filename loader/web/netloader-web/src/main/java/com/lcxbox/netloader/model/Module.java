package com.lcxbox.netloader.model;

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
	
	@JsonProperty("lcxl_role")
	private Integer lcxlRole;
	
	@JsonProperty("server_count")
	private Integer serverCount;
	
	@JsonProperty("route_timeout")
	private Integer routeTimeout;
	
	@JsonProperty("server_check")
	private ServerCheck serverCheck;
	
	@JsonProperty("routing_algorithm")
	private Integer routingAlgorithm;
	
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

	public Integer getLcxlRole() {
		return lcxlRole;
	}

	public void setLcxlRole(Integer lcxlRole) {
		this.lcxlRole = lcxlRole;
	}

	public Integer getServerCount() {
		return serverCount;
	}

	public void setServerCount(Integer serverCount) {
		this.serverCount = serverCount;
	}

	public Integer getRouteTimeout() {
		return routeTimeout;
	}

	public void setRouteTimeout(Integer routeTimeout) {
		this.routeTimeout = routeTimeout;
	}

	public ServerCheck getServerCheck() {
		return serverCheck;
	}

	public void setServerCheck(ServerCheck serverCheck) {
		this.serverCheck = serverCheck;
	}

	public Integer getRoutingAlgorithm() {
		return routingAlgorithm;
	}

	public void setRoutingAlgorithm(Integer routingAlgorithm) {
		this.routingAlgorithm = routingAlgorithm;
	}

	
}
