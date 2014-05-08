package com.lcxbox.netloader.host.service;

import java.io.IOException;
import java.net.UnknownHostException;

import org.springframework.stereotype.Service;

import com.lcxbox.common.model.CommonRequest;
import com.lcxbox.common.model.CommonResponse;
import com.lcxbox.common.model.HostInfo;
import com.lcxbox.common.model.LcxlNetCode;
import com.lcxbox.netloader.host.model.LcxlAddrInfo;
import com.lcxbox.netloader.host.model.ModuleListResponse;
import com.lcxbox.netloader.host.model.ServerListRequest;
import com.lcxbox.netloader.host.model.ServerListResponse;
import com.lcxbox.netloader.host.model.SetVirtualAddrRequest;
import com.lcxbox.socket.json.SocketRequest;

@Service()
public class HostServiceImpl implements IHostService {
	
	public ModuleListResponse getModuleList(HostInfo hostInfo) throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		
		CommonRequest request = new CommonRequest();
		request.setCode(LcxlNetCode.JC_MODULE_LIST);
		return SocketRequest.jsonRequest(hostInfo.getHost(), hostInfo.getPort(), request, ModuleListResponse.class);
	}
	
	public ServerListResponse getServerList(HostInfo hostInfo, long miniport_net_luid) throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		ServerListRequest request = new ServerListRequest();
		request.setCode(LcxlNetCode.JC_SERVER_LIST);
		request.setMiniportNetLuid(miniport_net_luid);
		return SocketRequest.jsonRequest(hostInfo.getHost(), hostInfo.getPort(), request, ServerListResponse.class);
	}
	
	public CommonResponse setVirtualAddr(HostInfo hostInfo, long miniportNetLuid,
			LcxlAddrInfo virtualAddr) throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		SetVirtualAddrRequest request = new SetVirtualAddrRequest();
		request.setCode(LcxlNetCode.JC_SET_VIRTUAL_ADDR);
		request.setMiniportNetLuid(miniportNetLuid);
		request.setVirtualAddr(virtualAddr);
		return SocketRequest.jsonRequest(hostInfo.getHost(), hostInfo.getPort(), request, CommonResponse.class);
	}

	
}
