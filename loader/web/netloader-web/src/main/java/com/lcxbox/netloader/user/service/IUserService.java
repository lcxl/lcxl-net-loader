package com.lcxbox.netloader.user.service;

import java.io.IOException;

import com.lcxbox.netloader.user.model.LogonRespnse;

public interface IUserService {
	public LogonRespnse logon(String host, Integer port, String username, String password) throws IOException; 
}
