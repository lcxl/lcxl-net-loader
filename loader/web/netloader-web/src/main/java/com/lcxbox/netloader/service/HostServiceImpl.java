package com.lcxbox.netloader.service;

import java.io.IOException;
import java.net.UnknownHostException;

import org.springframework.stereotype.Service;

import com.lcxbox.netloader.model.CommonRequest;
import com.lcxbox.netloader.model.CommonResponse;
import com.lcxbox.netloader.model.LcxlAddrInfo;
import com.lcxbox.netloader.model.LcxlNetCode;
import com.lcxbox.netloader.model.ModuleListResponse;
import com.lcxbox.netloader.model.ServerListRequest;
import com.lcxbox.netloader.model.ServerListResponse;
import com.lcxbox.netloader.model.SetVirtualAddrRequest;
import com.lcxbox.socket.json.SocketRequest;

@Service()
public class HostServiceImpl implements IHostService {
	
	private String host = "192.168.237.147";
	private Integer port = 32112;
	
	public ModuleListResponse getModuleList() throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		
		CommonRequest request = new CommonRequest();
		request.setCode(LcxlNetCode.JC_MODULE_LIST);
		return SocketRequest.jsonRequest(host, port, request, ModuleListResponse.class);
	}
	
	public ServerListResponse getServerList(long miniport_net_luid) throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		ServerListRequest request = new ServerListRequest();
		request.setCode(LcxlNetCode.JC_SERVER_LIST);
		request.setMiniportNetLuid(miniport_net_luid);
		return SocketRequest.jsonRequest(host, port, request, ServerListResponse.class);
	}
	
	public CommonResponse setVirtualAddr(long miniportNetLuid,
			LcxlAddrInfo virtualAddr) throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		SetVirtualAddrRequest request = new SetVirtualAddrRequest();
		request.setCode(LcxlNetCode.JC_SET_VIRTUAL_ADDR);
		request.setMiniportNetLuid(miniportNetLuid);
		request.setVirtualAddr(virtualAddr);
		return SocketRequest.jsonRequest(host, port, request, CommonResponse.class);
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
