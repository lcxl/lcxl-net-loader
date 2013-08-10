package lcxl.netloader.action;

import java.util.ArrayList;
import java.util.List;

import lcxl.netloader.record.ClusterListItem;

import com.opensymphony.xwork2.ActionSupport;

public class ClusterAction extends ActionSupport {
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	private List<ClusterListItem> clusterList = new ArrayList<ClusterListItem>();
	/**
	 * 获得集群列表
	 * @return clusterList返回集群列表
	 */
	public String clusterList() {
		
		return SUCCESS;
	}
	public List<ClusterListItem> getClusterList() {
		return clusterList;
	}
}
