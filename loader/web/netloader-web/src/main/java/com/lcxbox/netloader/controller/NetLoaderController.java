package com.lcxbox.netloader.controller;

import java.io.IOException;
import java.net.UnknownHostException;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.ResponseBody;

import com.lcxbox.netloader.model.CommonResponse;
import com.lcxbox.netloader.model.LcxlAddrInfo;
import com.lcxbox.netloader.model.ModuleListResponse;
import com.lcxbox.netloader.service.IHostService;

@Controller
@RequestMapping(value = "/netloader")
public class NetLoaderController {
	
	@Autowired
    private IHostService routerService; 
	
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
	
	@RequestMapping(value = "/set_virtual_addr.do", method=RequestMethod.POST)
	@ResponseBody
	public CommonResponse setVirtualAddr(
			HttpServletRequest request, 
			HttpServletResponse response, 
			@RequestParam("miniport_net_luid") long miniportNetLuid, 
			@RequestParam("ipv4") String ipv4, 
			@RequestParam("ipv4_onlink_prefix_length") Integer ipv4OnlinkPrefixLength, 
			@RequestParam("ipv6") String ipv6,
			@RequestParam("ipv6_onlink_prefix_length") Integer ipv6OnlinkPrefixLength, 
			@RequestParam(value="enable-ipv4", required=false, defaultValue = "0") boolean enableIpv4,
			@RequestParam(value="enable-ipv6", required=false, defaultValue = "0") boolean enableIpv6) throws UnknownHostException, IOException {
		LcxlAddrInfo virtualAddr = new LcxlAddrInfo();
		virtualAddr.setStatus((enableIpv4?LcxlAddrInfo.SA_ENABLE_IPV4:0) |(enableIpv6?LcxlAddrInfo.SA_ENABLE_IPV6:0));
		virtualAddr.setIpv4(ipv4);
		virtualAddr.setIpv4OnlinkPrefixLength(ipv4OnlinkPrefixLength);
		virtualAddr.setIpv6(ipv6);
		virtualAddr.setIpv6OnlinkPrefixLength(ipv6OnlinkPrefixLength);
		
		return routerService.setVirtualAddr(miniportNetLuid, virtualAddr);
	}
}
