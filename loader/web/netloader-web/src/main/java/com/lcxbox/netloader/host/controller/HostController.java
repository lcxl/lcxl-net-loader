package com.lcxbox.netloader.host.controller;

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

import com.lcxbox.common.CookieHelper;
import com.lcxbox.common.model.CommonResponse;
import com.lcxbox.netloader.host.model.LcxlAddrInfo;
import com.lcxbox.netloader.host.model.ModuleListResponse;
import com.lcxbox.netloader.host.model.Server;
import com.lcxbox.netloader.host.model.ServerListResponse;
import com.lcxbox.netloader.host.service.IHostService;

@Controller
@RequestMapping(value = "/netloader")
public class HostController {
	
	@Autowired
    private IHostService hostService; 
	
	/**
	 * 获取模块列表
	 * @return
	 * @throws IOException 
	 * @throws UnknownHostException 
	 */
	@RequestMapping(value = "/module_list.do", method=RequestMethod.GET)
	@ResponseBody
	public ModuleListResponse getModuleList(HttpServletRequest request, HttpServletResponse response) throws UnknownHostException, IOException {
		return hostService.getModuleList(CookieHelper.getHostInfo(request.getCookies()));
	}
	/**
	 * 设置虚拟IP地址
	 * @param request
	 * @param response
	 * @param miniportNetLuid
	 * @param ipv4
	 * @param ipv4OnlinkPrefixLength
	 * @param ipv6
	 * @param ipv6OnlinkPrefixLength
	 * @param enableIpv4
	 * @param enableIpv6
	 * @return
	 * @throws UnknownHostException
	 * @throws IOException
	 */
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
		
		return hostService.setVirtualAddr(CookieHelper.getHostInfo(request.getCookies()), miniportNetLuid, virtualAddr);
	}
	/**
	 * 获取后端服务器列表
	 * @param request
	 * @param response
	 * @param miniportNetLuid
	 * @return
	 * @throws UnknownHostException
	 * @throws IOException
	 */
	@RequestMapping(value = "/server_list.do", method=RequestMethod.GET)
	@ResponseBody
	public ServerListResponse getServerList(
			HttpServletRequest request, 
			HttpServletResponse response, 
			@RequestParam("miniport_net_luid") long miniportNetLuid) throws UnknownHostException, IOException {
		return hostService.getServerList(CookieHelper.getHostInfo(request.getCookies()), miniportNetLuid);
		
	}
	
	@RequestMapping(value = "/add_server.do", method=RequestMethod.POST)
	@ResponseBody
	public CommonResponse addServer(
			HttpServletRequest request, 
			HttpServletResponse response,
			@RequestParam("miniport_net_luid") long miniportNetLuid,
			@RequestParam("status") int status,
			@RequestParam("ip_status") int ipStatus,
			@RequestParam("mac_addr") String macAddr,
			@RequestParam("comment") String comment
			) throws UnknownHostException, IOException {
		Server server = new Server();
		server.setStatus(status);
		server.setIpStatus(ipStatus);
		server.setMacAddr(macAddr);
		server.setComment(comment);
		return hostService.addServer(CookieHelper.getHostInfo(request.getCookies()), miniportNetLuid, server);
	}
	
	@RequestMapping(value = "/del_server.do", method=RequestMethod.POST)
	@ResponseBody
	public CommonResponse delServer(
			HttpServletRequest request, 
			HttpServletResponse response,
			@RequestParam("miniport_net_luid") long miniportNetLuid,
			@RequestParam("mac_addr") String macAddr
			) throws UnknownHostException, IOException {
		return hostService.delServer(CookieHelper.getHostInfo(request.getCookies()), miniportNetLuid, macAddr);
	}
}
