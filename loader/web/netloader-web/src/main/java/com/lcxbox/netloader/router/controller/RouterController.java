package com.lcxbox.netloader.router.controller;

import java.io.IOException;
import java.net.UnknownHostException;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.ResponseBody;

import com.lcxbox.netloader.router.model.ModuleListResponse;
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
	@RequestMapping(value = "/module_list.do", method=RequestMethod.GET)
	@ResponseBody
	public ModuleListResponse getModuleList(HttpServletRequest request, HttpServletResponse response) throws UnknownHostException, IOException {
		return routerService.getModuleList();
	}
}
