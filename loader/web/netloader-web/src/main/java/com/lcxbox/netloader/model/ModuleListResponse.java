package com.lcxbox.netloader.model;

import java.util.List;

public class ModuleListResponse extends CommonResponse {
	private List<Module> data;

	public List<Module> getData() {
		return data;
	}

	public void setData(List<Module> data) {
		this.data = data;
	}
	
	
}
