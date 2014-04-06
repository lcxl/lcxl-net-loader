package com.lcxbox.netloader.service;

import java.io.IOException;
import java.net.UnknownHostException;

import com.lcxbox.netloader.model.CommonResponse;
import com.lcxbox.netloader.model.LcxlAddrInfo;
import com.lcxbox.netloader.model.ModuleListResponse;
import com.lcxbox.netloader.model.ServerListResponse;

public interface IRouterService {
	public ModuleListResponse getModuleList() throws UnknownHostException, IOException;
	public ServerListResponse getServerList(long miniport_net_luid) throws UnknownHostException, IOException;
	public CommonResponse setVirtualAddr(long miniportNetLuid, LcxlAddrInfo virtualAddr) throws UnknownHostException, IOException;
}
