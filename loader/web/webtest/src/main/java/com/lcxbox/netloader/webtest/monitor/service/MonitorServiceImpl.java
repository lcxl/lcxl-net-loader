package com.lcxbox.netloader.webtest.monitor.service;

import java.lang.management.ManagementFactory;
import java.net.InetAddress;

import com.sun.management.OperatingSystemMXBean;
import com.lcxbox.netloader.webtest.monitor.model.MonitorInfoBean;

@SuppressWarnings("restriction")
public class MonitorServiceImpl implements IMonitorService {
	private OperatingSystemMXBean osmxb;
	public MonitorServiceImpl() {
		osmxb = (OperatingSystemMXBean) ManagementFactory
                .getOperatingSystemMXBean();
		osmxb.getSystemCpuLoad();
	}
    /**
     * 获得当前的监控对象.
     * 
     * @return 返回构造好的监控对象
     * @throws Exception
     * @author GuoHuang
     */
    public MonitorInfoBean getMonitorInfoBean() throws Exception {
        int kb = 1024;
        // 可使用内存
        long totalMemory = Runtime.getRuntime().totalMemory() / kb;
        // 剩余内存
        long freeMemory = Runtime.getRuntime().freeMemory() / kb;
        // 最大可使用内存
        long maxMemory = Runtime.getRuntime().maxMemory() / kb;
       
        // 操作系统
        String osName = System.getProperty("os.name");
        // 总的物理内存
        
        long totalMemorySize = osmxb.getTotalPhysicalMemorySize() / kb;
        // 剩余的物理内存
        long freePhysicalMemorySize = osmxb.getFreePhysicalMemorySize() / kb;
        // 已使用的物理内存
        long usedMemory = (osmxb.getTotalPhysicalMemorySize() - osmxb
                .getFreePhysicalMemorySize())
                / kb;
        //InetAddress addr = InetAddress.getLocalHost();
        InetAddress addrs[] = InetAddress.getAllByName(InetAddress.getLocalHost().getHostName());
        String ips = "";
        for (InetAddress addr: addrs) {
        	ips += addr.getHostAddress()+"; ";
        }
        // 获得线程总数
        ThreadGroup parentThread;
        for (parentThread = Thread.currentThread().getThreadGroup(); parentThread
                .getParent() != null; parentThread = parentThread.getParent())
            ;
        int totalThread = parentThread.activeCount();
        double cpuRatio = osmxb.getSystemCpuLoad();
        //double cpuRatio = osmxb.getSystemLoadAverage();
        
        // 构造返回对象
        MonitorInfoBean infoBean = new MonitorInfoBean();
        infoBean.setFreeMemory(freeMemory);
        infoBean.setFreePhysicalMemorySize(freePhysicalMemorySize);
        infoBean.setMaxMemory(maxMemory);
        infoBean.setOsName(osName);
        infoBean.setTotalMemory(totalMemory);
        infoBean.setTotalMemorySize(totalMemorySize);
        infoBean.setTotalThread(totalThread);
        infoBean.setUsedMemory(usedMemory);
        infoBean.setCpuRatio(cpuRatio);
        infoBean.setHostAddress(ips);
        infoBean.setHostName(addrs[0].getHostName());
        return infoBean;
    }

    
    /**
     * 测试方法.
     * 
     * @param args
     * @throws Exception
     * @author GuoHuang
     */
    public static void main(String[] args) throws Exception {
        IMonitorService service = new MonitorServiceImpl();
        MonitorInfoBean monitorInfo = service.getMonitorInfoBean();
        System.out.println("服务器IP地址=" + monitorInfo.getHostAddress());
        System.out.println("服务器名称=" + monitorInfo.getHostName());
        System.out.println("cpu占有率=" + monitorInfo.getCpuRatio());
        System.out.println("可使用内存=" + monitorInfo.getTotalMemory());
        System.out.println("剩余内存=" + monitorInfo.getFreeMemory());
        System.out.println("最大可使用内存=" + monitorInfo.getMaxMemory());
        System.out.println("操作系统=" + monitorInfo.getOsName());
        System.out.println("总的物理内存=" + monitorInfo.getTotalMemorySize() + "kb");
        System.out.println("剩余的物理内存=" + monitorInfo.getFreeMemory() + "kb");
        System.out.println("已使用的物理内存=" + monitorInfo.getUsedMemory() + "kb");
        System.out.println("线程总数=" + monitorInfo.getTotalThread() + "kb");
        
    }

}
