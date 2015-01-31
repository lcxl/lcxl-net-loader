package com.lcxbox.netloader.host.service;

import java.io.IOException;
import java.net.UnknownHostException;

import org.springframework.stereotype.Service;

import com.lcxbox.common.model.CommonRequest;
import com.lcxbox.common.model.CommonResponse;
import com.lcxbox.common.model.HostInfo;
import com.lcxbox.common.model.LcxlNetCode;
import com.lcxbox.netloader.host.model.AddServerRequest;
import com.lcxbox.netloader.host.model.DelServerRequest;
import com.lcxbox.netloader.host.model.LcxlAddrInfo;
import com.lcxbox.netloader.host.model.ModuleListResponse;
import com.lcxbox.netloader.host.model.Server;
import com.lcxbox.netloader.host.model.ServerListRequest;
import com.lcxbox.netloader.host.model.ServerListResponse;
import com.lcxbox.netloader.host.model.SetVirtualAddrRequest;
import com.lcxbox.socket.json.SocketRequest;
/**
 * 主机服务实现类
 * @author lcxl
 *
 */
@Service()
public class HostServiceImpl implements IHostService {

	public ModuleListResponse getModuleList(HostInfo hostInfo)
			throws UnknownHostException, IOException {
		// TODO Auto-generated method stub

		CommonRequest request = new CommonRequest();
		request.setCode(LcxlNetCode.JC_MODULE_LIST);
		return SocketRequest.jsonRequest(hostInfo.getHost(),
				hostInfo.getPort(), request, ModuleListResponse.class);
	}

	public CommonResponse setVirtualAddr(HostInfo hostInfo,
			long miniportNetLuid, LcxlAddrInfo virtualAddr)
			throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		SetVirtualAddrRequest request = new SetVirtualAddrRequest();
		request.setCode(LcxlNetCode.JC_SET_VIRTUAL_ADDR);
		request.setMiniportNetLuid(miniportNetLuid);
		request.setVirtualAddr(virtualAddr);
		return SocketRequest.jsonRequest(hostInfo.getHost(),
				hostInfo.getPort(), request, CommonResponse.class);
	}

	public ServerListResponse getServerList(HostInfo hostInfo,
			long miniportNetLuid) throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		ServerListRequest request = new ServerListRequest();
		request.setCode(LcxlNetCode.JC_SERVER_LIST);
		request.setMiniportNetLuid(miniportNetLuid);
		return SocketRequest.jsonRequest(hostInfo.getHost(),
				hostInfo.getPort(), request, ServerListResponse.class);
	}

	public CommonResponse addServer(HostInfo hostInfo, long miniportNetLuid,
			Server server) throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		AddServerRequest request = new AddServerRequest();
		request.setCode(LcxlNetCode.JC_ADD_SERVER);
		request.setMiniportNetLuid(miniportNetLuid);
		request.setServer(server);
		return SocketRequest.jsonRequest(hostInfo.getHost(),
				hostInfo.getPort(), request, CommonResponse.class);
	}

	public CommonResponse delServer(HostInfo hostInfo, long miniportNetLuid,
			String macAddr) throws UnknownHostException, IOException {
		// TODO Auto-generated method stub
		DelServerRequest request = new DelServerRequest();
		request.setCode(LcxlNetCode.JC_DEL_SERVER);
		request.setMiniportNetLuid(miniportNetLuid);
		request.setMacAddr(macAddr);
		return SocketRequest.jsonRequest(hostInfo.getHost(),
				hostInfo.getPort(), request, CommonResponse.class);
	}
}
