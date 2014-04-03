package com.lcxbox.netloader.router.service;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.UnknownHostException;
import java.util.List;

import org.codehaus.jackson.JsonEncoding;
import org.codehaus.jackson.map.ObjectMapper;
import org.springframework.stereotype.Service;

import com.lcxbox.netloader.router.model.CommonRequest;
import com.lcxbox.netloader.router.model.LcxlNetCode;
import com.lcxbox.netloader.router.model.Module;
import com.lcxbox.netloader.router.model.ModuleListResponse;
import com.lcxbox.socket.cmd.JsonSocket;

@Service
public class RouterService implements IRouterService {
	/**
	 * 获取模块列表命令
	 */
	public static final short LL_GET_MODULE_LIST = 0x0001;
	/**
	 * 获取服务器列表
	 */
	public static final short LL_GET_SERVER_LIST = 0x0002;
	private ObjectMapper  objectMapper = new ObjectMapper();
	
	private String host = "localhost";
	private Integer port = 32112;
	
	public List<Module> getModuleList() throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		JsonSocket sock = new JsonSocket(host, port);
		CommonRequest request = new CommonRequest();
		request.setCode(LcxlNetCode.JC_MODULE_LIST);
		
		ByteArrayOutputStream os = new ByteArrayOutputStream();
		objectMapper.getJsonFactory().createJsonGenerator(os, JsonEncoding.UTF16_LE).writeObject(request);
		String ret = new String(sock.sendData(os.toByteArray()));
		os.close();
		sock.close();
		
		objectMapper.readValue(ret, ModuleListResponse.class);
		return null;
	}

	public String getHost() {
		return host;
	}

	public void setHost(String host) {
		this.host = host;
	}

	public Integer getPort() {
		return port;
	}

	public void setPort(Integer port) {
		this.port = port;
	}

	
}
