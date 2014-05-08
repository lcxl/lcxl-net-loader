package com.lcxbox.netloader.user.service;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.net.SocketException;
import java.net.UnknownHostException;

import org.springframework.stereotype.Service;

import com.lcxbox.common.model.LcxlNetCode;
import com.lcxbox.netloader.user.model.LogonRequest;
import com.lcxbox.netloader.user.model.LogonRespnse;
import com.lcxbox.socket.json.SocketRequest;

@Service()
public class UserServiceImpl implements IUserService {

	public LogonRespnse logon(String host, Integer port, String username, String password) throws IOException {
		// TODO Auto-generated method stub
		LogonRequest request = new LogonRequest();
		request.setCode(LcxlNetCode.JC_LOGON);
		request.setUsername(username);
		request.setPassword(password);
		try {
		return SocketRequest.jsonRequest(host, port, request, LogonRespnse.class);
		} catch (UnknownHostException e) {
			LogonRespnse response = new LogonRespnse();
			response.setCode(LcxlNetCode.JC_LOGON);
			response.setStatus(LcxlNetCode.JS_UNKNOWN_HOST);
			return response;
		} catch (SocketException e) {
			LogonRespnse response = new LogonRespnse();
			response.setCode(LcxlNetCode.JC_LOGON);
			response.setStatus(LcxlNetCode.JS_SOCKET_ERROR);
			return response;
		}
	}

}
