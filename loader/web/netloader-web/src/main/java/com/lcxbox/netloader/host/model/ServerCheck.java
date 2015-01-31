package com.lcxbox.netloader.host.model;

import org.codehaus.jackson.annotate.JsonProperty;
/**
 * 服务检查相关信息
 * @author lcxl
 *
 */
public class ServerCheck {
	@JsonProperty("interval")
	private Integer interval;
	
	@JsonProperty("timeout")
	private Integer timeout;
	
	@JsonProperty("retry_number")
	private Integer retryNumber;

	public Integer getInterval() {
		return interval;
	}

	public void setInterval(Integer interval) {
		this.interval = interval;
	}

	public Integer getTimeout() {
		return timeout;
	}

	public void setTimeout(Integer timeout) {
		this.timeout = timeout;
	}

	public Integer getRetryNumber() {
		return retryNumber;
	}

	public void setRetryNumber(Integer retryNumber) {
		this.retryNumber = retryNumber;
	}
	
	
}
