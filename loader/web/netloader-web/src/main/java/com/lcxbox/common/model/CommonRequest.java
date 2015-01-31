package com.lcxbox.common.model;

import org.codehaus.jackson.map.annotate.JsonSerialize;
/**
 * 通用请求类
 * @author lcxl
 *
 */
@JsonSerialize(include=JsonSerialize.Inclusion.NON_EMPTY)
public class CommonRequest {
	private int code;

	public int getCode() {
		return code;
	}

	public void setCode(int code) {
		this.code = code;
	}
	
	
}
