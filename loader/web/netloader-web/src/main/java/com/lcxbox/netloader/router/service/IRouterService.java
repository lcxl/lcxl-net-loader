package com.lcxbox.netloader.router.service;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.List;

import com.lcxbox.netloader.router.model.Module;

public interface IRouterService {
	public List<Module> getModuleList() throws UnknownHostException, IOException;
}
