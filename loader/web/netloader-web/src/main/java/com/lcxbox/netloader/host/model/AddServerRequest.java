package com.lcxbox.netloader.host.model;

import org.codehaus.jackson.annotate.JsonProperty;

import com.lcxbox.common.model.CommonRequest;

public class AddServerRequest extends CommonRequest {
	@JsonProperty("miniport_net_luid")
	private long miniportNetLuid; 
	
	private Server server;

	public long getMiniportNetLuid() {
		return miniportNetLuid;
	}

	public void setMiniportNetLuid(long miniportNetLuid) {
		this.miniportNetLuid = miniportNetLuid;
	}

	public Server getServer() {
		return server;
	}

	public void setServer(Server server) {
		this.server = server;
	}
	
	
}
