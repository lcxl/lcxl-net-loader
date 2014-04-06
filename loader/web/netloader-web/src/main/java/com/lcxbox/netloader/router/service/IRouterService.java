package com.lcxbox.netloader.router.service;

import java.io.IOException;
import java.net.UnknownHostException;

import com.lcxbox.netloader.router.model.ModuleListResponse;
import com.lcxbox.netloader.router.model.ServerListResponse;

public interface IRouterService {
	public ModuleListResponse getModuleList() throws UnknownHostException, IOException;
	public ServerListResponse getServerList(long miniport_net_luid) throws UnknownHostException, IOException;
}
