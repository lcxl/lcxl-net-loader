<%@ page language="java" import="com.lcxbox.netloader.webtest.common.*" pageEncoding="utf-8"%>
<%@ page language="java" import="com.lcxbox.netloader.webtest.monitor.model.*"%>
<%@ page language="java" import="com.lcxbox.netloader.webtest.monitor.service.*"%>

<%
    String path = request.getContextPath();
    String basePath = request.getScheme() + "://"
            + request.getServerName() + ":" + request.getServerPort()
            + path + "/";
%>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
    <head>
        <base href="<%=basePath%>">

        <title>运行状态</title>

        <meta http-equiv="pragma" content="no-cache">
        <meta http-equiv="cache-control" content="no-cache">
        <meta http-equiv="expires" content="0">
        <meta http-equiv="keywords" content="keyword1,keyword2,keyword3">
        <meta http-equiv="description" content="This is my page">
        <meta http-equiv="refresh" content="5">

    </head>

    <body>
        <%
            IMonitorService service = new MonitorServiceImpl();
            MonitorInfoBean monitorInfo = service.getMonitorInfoBean();
            out.println("服务器IP地址" + monitorInfo.getHostAddress() + "<br>");
        	out.println("服务器名称=" + monitorInfo.getHostName() + "<br>");
            out.println("cpu占有率=" + monitorInfo.getCpuRatio() + "<br>");
            out.println("可使用内存=" + monitorInfo.getTotalMemory() + "<br>");
            out.println("剩余内存=" + monitorInfo.getFreeMemory() + "<br>");
            out.println("最大可使用内存=" + monitorInfo.getMaxMemory() + "<br>");
            out.println("操作系统=" + monitorInfo.getOsName() + "<br>");
            out.println("总的物理内存=" + monitorInfo.getTotalMemorySize() + "kb"
                    + "<br>");
            out.println("剩余的物理内存=" + monitorInfo.getFreeMemory() + "kb"
                    + "<br>");
            out.println("已使用的物理内存=" + monitorInfo.getUsedMemory() + "kb"
                    + "<br>");
            out.println("线程总数=" + monitorInfo.getTotalThread() + "kb" + "<br>");
        %>

        cpu:
        <table width="150" style="border: 1px solid #00FF00;">
            <tr>
                <td>
                    <table width="150" bgcolor="#FFFFFF" cellpadding="0"
                        cellspacing="0">
                        <tr>
                            <td width="<%=(int) (monitorInfo.getCpuRatio()*100)%>%" bgcolor="#00FF00"
                                align="center">
                                <%=(int) (monitorInfo.getCpuRatio()*100)%>%
                            </td>
                            <td bgcolor="#FFFFFF">
                                &nbsp;
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>
        </table>

        <table width="1000" style="height: 0.5px;">
            <tr>
                <td></td>
            </tr>
        </table>
        内存:<%
            int memory = (int) (monitorInfo.getUsedMemory() * 100 / monitorInfo
                    .getTotalMemorySize());
        %>
        <table width="150" style="border: 1px solid #00FF00;">
            <tr>
                <td>
                    <table width="150" bgcolor="#FFFFFF" cellpadding="0"
                        cellspacing="0">
                        <tr>
                            <td width="<%=memory%>%" bgcolor="#00FF00" align="center">
                                <%=memory%>%
                            </td>
                            <td bgcolor="#FFFFFF">
                                &nbsp;
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>
        </table>
    </body>
</html>