package com.lcxbox.socket.cmd;

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
		os.writeInt(data.length);
		os.write(data);
		int len = is.readInt();
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
