package com.lcxbox.netloader.host.model;

import org.codehaus.jackson.annotate.JsonProperty;
import org.codehaus.jackson.map.annotate.JsonSerialize;

import com.lcxbox.common.model.CommonRequest;

@JsonSerialize(include=JsonSerialize.Inclusion.NON_EMPTY)
public class ServerListRequest extends CommonRequest {
	@JsonProperty("miniport_net_luid")
	private long miniportNetLuid;

	public long getMiniportNetLuid() {
		return miniportNetLuid;
	}

	public void setMiniportNetLuid(long miniportNetLuid) {
		this.miniportNetLuid = miniportNetLuid;
	}

	
	
	
}
