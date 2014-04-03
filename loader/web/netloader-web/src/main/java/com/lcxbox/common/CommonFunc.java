package com.lcxbox.common;

public class CommonFunc {
	
	public static long bytesToLong(byte[] data) {
		return bytesToLong(data,0);
	}
	
	public static long bytesToLong(byte[] data, int offset) {
		assert(data.length-offset>=8);
		return ((long)data[offset])|
		(data[offset+1]<<1)|
		(data[offset+2]<<2)|
		(data[offset+3]<<3)|
		(data[offset+4]<<4)|
		(data[offset+5]<<5)|
		(data[offset+6]<<6)|
		(data[offset+7]<<7);
	}
	
	public static int bytesToInt(byte[] data) {
		return bytesToInt(data, 0);
	}
	
	public static int bytesToInt(byte[] data, int offset) {
		assert(data.length-offset>=4);
		return (data[offset])|
				(data[offset+1]<<1)|
				(data[offset+2]<<2)|
				(data[offset+3]<<3);
	}
}
