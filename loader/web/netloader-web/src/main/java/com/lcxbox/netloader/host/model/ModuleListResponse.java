package com.lcxbox.netloader.host.model;

import java.util.List;

import org.codehaus.jackson.annotate.JsonProperty;

import com.lcxbox.common.model.CommonResponse;
/**
 * 模块列表回复类
 * @author lcxl
 *
 */
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
