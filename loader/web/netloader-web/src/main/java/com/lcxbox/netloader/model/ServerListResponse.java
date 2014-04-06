package com.lcxbox.netloader.model;

import java.util.List;

public class ServerListResponse extends CommonResponse {
	private List<Server> data;

	public List<Server> getData() {
		return data;
	}

	public void setData(List<Server> data) {
		this.data = data;
	}
	
	
}
