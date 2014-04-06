package com.lcxbox.socket.json;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.UnknownHostException;

import org.codehaus.jackson.JsonEncoding;
import org.codehaus.jackson.map.ObjectMapper;


public class SocketRequest {
	public static <T> T jsonRequest(String host, int port, Object request, Class<T> responseCls) throws UnknownHostException, IOException {
		ObjectMapper  objectMapper = new ObjectMapper();
		
		JsonSocket sock = new JsonSocket(host, port);
		ByteArrayOutputStream os = new ByteArrayOutputStream();
		objectMapper.getJsonFactory().createJsonGenerator(os, JsonEncoding.UTF16_LE).writeObject(request);
		String ret = new String(sock.sendData(os.toByteArray()),"UTF-16LE");
		os.close();
		sock.close();
		return objectMapper.readValue(ret, responseCls);
	}
}
