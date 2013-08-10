package lcxl.netloader.jni;

import java.util.ArrayList;

import lcxl.netloader.record.ClusterListItem;

public class ClusterJNI {
	static{  
        System.loadLibrary("ClusterJni");  
    }  
	public native static ArrayList<ClusterListItem> getClusterList();
}
