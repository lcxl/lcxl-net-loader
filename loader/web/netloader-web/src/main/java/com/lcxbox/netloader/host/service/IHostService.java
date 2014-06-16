package com.lcxbox.netloader.host.service;

import java.io.IOException;
import java.net.UnknownHostException;

import com.lcxbox.common.model.CommonResponse;
import com.lcxbox.common.model.HostInfo;
import com.lcxbox.netloader.host.model.LcxlAddrInfo;
import com.lcxbox.netloader.host.model.ModuleListResponse;
import com.lcxbox.netloader.host.model.Server;
import com.lcxbox.netloader.host.model.ServerListResponse;

public interface IHostService {
	public ModuleListResponse getModuleList(HostInfo hostInfo) throws UnknownHostException, IOException;
	public ServerListResponse getServerList(HostInfo hostInfo, long miniportNetLuid) throws UnknownHostException, IOException;
	public CommonResponse setVirtualAddr(HostInfo hostInfo, long miniportNetLuid, LcxlAddrInfo virtualAddr) throws UnknownHostException, IOException;
	public CommonResponse addServer(HostInfo hostInfo, long miniportNetLuid, Server server) throws UnknownHostException, IOException;
	public CommonResponse delServer(HostInfo hostInfo, long miniportNetLuid, String macAddr) throws UnknownHostException, IOException;
}
