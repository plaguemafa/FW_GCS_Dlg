// UdpDataProtocol.h: UDP通信协议数据结构定义
//

#pragma once

// UDP通信协议数据结构
// 后续可在此文件中填充地面站协议
struct UdpRecvDataPacket
{
	float data1;  // 数据1
	float data2;  // 数据2
	float data3;  // 数据3
	float data4;  // 数据4
	float data5;  // 数据5
};

// 握手数据包结构
struct UdpHandshakePacket
{
	char magic[4];      // 握手标识 "GCS\0"
	unsigned int version;     // 协议版本（使用unsigned int替代UINT32）
};

