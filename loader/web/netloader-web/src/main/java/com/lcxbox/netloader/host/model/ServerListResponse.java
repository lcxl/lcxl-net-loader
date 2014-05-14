package com.lcxbox.netloader.host.model;

import java.util.List;

import org.codehaus.jackson.annotate.JsonProperty;

import com.lcxbox.common.model.CommonResponse;

public class ServerListResponse extends CommonResponse {
	
	@JsonProperty("server_list")
	private List<Server> serverList;

	public List<Server> getServerList() {
		return serverList;
	}

	public void setServerList(List<Server> serverList) {
		this.serverList = serverList;
	}
}
