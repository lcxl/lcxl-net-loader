package com.lcxbox.socket.json;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

public class JsonSocket {
	private Socket socket;
	private DataInputStream is;
	private DataOutputStream os;
	
	public JsonSocket(String host, int port) throws UnknownHostException, IOException {
		socket = new Socket(host, port);
		is = new DataInputStream(socket.getInputStream());
		os = new DataOutputStream(socket.getOutputStream());
	}
	/**
	 * 发送数据
	 * @param cmdData
	 * @return
	 * @throws IOException
	 */
	public byte[] sendData(byte[] data) throws IOException {
		byte[] datalen = new byte[4];
		datalen[0] = (byte) (data.length & 0xFF);
		datalen[1] = (byte) ((data.length>>8) & 0xFF);
		datalen[2] = (byte) ((data.length>>16) & 0xFF);
		datalen[3] = (byte) ((data.length>>24) & 0xFF);
		
		os.write(datalen);
		os.write(data);
		is.readFully(datalen);
		int len = (datalen[0] & 0xff)|((datalen[1]& 0xff)<<8)|((datalen[2]& 0xff)<<16)|((datalen[3]& 0xff)<<24);
		byte[] ret = new byte[len];
		is.readFully(ret);
		return ret;
	}
	
	public void close() throws IOException {
		os.close();
		is.close();
		socket.close();
	}
}
