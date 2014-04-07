package com.lcxbox.netloader.model;

import java.util.List;

import org.codehaus.jackson.annotate.JsonProperty;

public class ModuleListResponse extends CommonResponse {
	@JsonProperty("module_list")
	private List<Module> moduleList;

	public List<Module> getModuleList() {
		return moduleList;
	}

	public void setModuleList(List<Module> moduleList) {
		this.moduleList = moduleList;
	}

	
	
	
}
