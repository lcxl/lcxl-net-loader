package com.lcxbox.netloader.host.model;

import org.codehaus.jackson.annotate.JsonProperty;

public class Module {
	private Boolean isexist;
	
	@JsonProperty("ipv4_router_active")
	private Boolean ipv4RouterActive;
	
	@JsonProperty("ipv6_router_active")
	private Boolean ipv6RouterActive;
	
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
	
	
	public Boolean getIsexist() {
		return isexist;
	}

	public void setIsexist(Boolean isexist) {
		this.isexist = isexist;
	}

	public Boolean getIpv4RouterActive() {
		return ipv4RouterActive;
	}

	public void setIpv4RouterActive(Boolean ipv4RouterActive) {
		this.ipv4RouterActive = ipv4RouterActive;
	}

	public Boolean getIpv6RouterActive() {
		return ipv6RouterActive;
	}

	public void setIpv6RouterActive(Boolean ipv6RouterActive) {
		this.ipv6RouterActive = ipv6RouterActive;
	}

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
