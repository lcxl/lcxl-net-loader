package com.lcxbox.netloader.model;

import java.util.List;

import org.codehaus.jackson.annotate.JsonProperty;

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
