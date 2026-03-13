// UdpConfig.h: UDP通信配置宏（共享头）
//

#pragma once

// UDP配置参数宏
// 注意：UDP_REMOTE_IP 和 UDP_REMOTE_PORT 对应 Simulink（发送端）的本地地址和端口，用于：
//   1. 接收验证：检查收到的数据包是否来自这个地址（Simulink的源地址）
//   2. 发送目标：程序发送数据时发送到这个地址（Simulink的监听地址）
//
// Simulink 配置对应关系：
//   Simulink 本地地址 = UDP_REMOTE_IP (127.0.0.1)
//   Simulink 本地端口 = UDP_REMOTE_PORT (5000) - Simulink需要监听此端口接收程序发送的数据
//   Simulink 远程地址 = 127.0.0.1 (程序所在地址)
//   Simulink 远程端口 = UDP_LOCAL_PORT (5001) - Simulink发送数据的目标端口
//
#define UDP_LOCAL_IP       "127.0.0.1"   // 本机IP（局域网网口IP，或"0.0.0.0"表示监听所有接口）
//#define UDP_REMOTE_IP      "192.168.1.11"   // 远程设备IP（飞控固件IP，用于实际连接）
#define UDP_REMOTE_IP      "127.0.0.1"     // Simulink的本地IP（用于本地测试）
#define UDP_REMOTE_PORT     50000           // Simulink的本地端口（用于本地测试）地面站远程端口
#define UDP_LOCAL_PORT      50001           // 本程序监听端口（接收Simulink发送的数据）
