package com.lcxbox.netloader.router.controller;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.ResponseBody;

import com.lcxbox.netloader.router.model.Module;
import com.lcxbox.netloader.router.service.IRouterService;

@Controller
@RequestMapping(value = "/router")
public class RouterController {
	@Autowired
    private IRouterService routerService; 
	/**
	 * 获取模块列表
	 * @return
	 * @throws IOException 
	 * @throws UnknownHostException 
	 */
	@RequestMapping(value = "/module_list", method=RequestMethod.GET)
	@ResponseBody
	public List<Module> getModuleList() throws UnknownHostException, IOException {
		return routerService.getModuleList();
	}
}
