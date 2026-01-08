#include <stdint.h>
// #include <string.h>

// UdpDataProtocol.h: UDP通信协议数据结构定义

#pragma once

// UDP通信协议数据结构
#pragma pack(push, 1)  // 紧密打包，避免字节对齐填充
typedef struct {
    // 帧头 (序号1)
    uint16_t frameHeader;  // 固定值0xAA55 (小端存储: 内存中为55 AA)
    
    // // 字节长度 (序号2)
    uint16_t dataLength;   // 整个数据帧长度(含帧头和校验和)
    
    // 飞机状态数据 (序号3-74)
    // uint8_t  aircraftID;          // 飞机编号 (3)
    int16_t  pitchAngle;          // 俯仰角 (4)
    int16_t  rollAngle;           // 滚转角 (5)
    int16_t  yawAngle;            // 航向角 (6)
    // int16_t  normalOverload;      // 法向过载 (7)
    int16_t  attackAngle;         // 攻角 (8)
    int16_t  sideslipAngle;       // 侧滑角 (9)
    // int8_t   rudderCmd1;          // 1#舵偏指令 (10)
    // int8_t   rudderCmd2;          // 2#舵偏指令 (11)
    // int8_t   rudderCmd3;          // 3#舵偏指令 (12)
    // int8_t   rudderCmd4;          // 4#舵偏指令 (13)
    // uint8_t  throttle;            // 油门控制 (14)
    // uint8_t  controlCommand;      // 控制指令 (15)
    // int8_t   turnRudderCmd;       // 转弯舵机指令 (16)
    // int16_t  pitchRate;           // 俯仰角速率 (17)
    // int16_t  rollRate;            // 滚转角速率 (18)
    // int16_t  yawRate;             // 航向角速率 (19)
    // int16_t  pitchAcceleration;   // 俯仰角加速度 (20)
    // int16_t  rollAcceleration;    // 滚转角加速度 (21)
    // int16_t  yawAcceleration;     // 航向角加速度 (22)
    // int16_t  longitudinalOverload; // 纵向过载 (23)
    // int16_t  lateralOverload;     // 横向过载 (24)
    // int16_t  engineTemp;          // 发动机缸温 (25)
    // int16_t  engineRPM;           // 发动机转速 (26)
    // uint8_t  fuelRemaining;       // 剩余油量 (27)
    // int16_t  gpsAltitude;         // 卫星高度 (28)
    // uint8_t  gpsStatus;           // 卫星定位状态 (29)
    // uint8_t  satelitesNum;        // 卫星收星数 (30)
    // int16_t  gpsCourse;           // 卫星地速航向 (31)
    // int16_t  gpsGroundSpeed;      // 卫星地速 (32)
    // uint8_t  gpsVerticalSpeed;    // 卫星垂直速度 (33)
    // int32_t  longitude;           // 卫星经度 (34)
    // int32_t  latitude;            // 卫星纬度 (35)
    // int16_t  eastVelocity;        // 东向速度 (36)
    // int16_t  northVelocity;       // 北向速度 (37)
    // int16_t  verticalVelocity;    // 天向速度 (38)
    // int8_t   airTemperature;      // 大气温度 (39)
    // int16_t  baroAltitude;        // 气压高度 (40)
    // int16_t  baroAirspeed;        // 气压空速 (41)
    // int16_t  indicatedAirspeed;   // 表速 (42)
    // uint8_t  machNumber;          // 马赫数 (43)
    // uint16_t radioAltitude;       // 无线电高度 (44)
    // uint8_t  navStatus;           // 导航状态 (45)
    // uint8_t  gpsHour;             // GPS时 (46)
    // uint8_t  gpsMinute;           // GPS分 (47)
    // uint8_t  gpsSecond;           // GPS秒 (48)
    // uint8_t  routeNumber;         // 当前航线号 (49)
    // uint8_t  targetWaypoint;      // 目标航点 (50)
    // int16_t  crossTrackError;     // 偏航距 (51)
    // int16_t  courseDeviation;     // 偏航角 (52)
    // int16_t  distanceToGo;        // 待飞距 (53)
    // uint8_t  commandHeading;      // 应飞航向 (54)
    // uint16_t commandSpeed;        // 应飞速度 (55) 注意: 协议类型U8但字节数2，按实际定义
    // uint16_t commandAltitude;     // 应飞高度 (56) 同上
    // uint8_t  commandTime;         // 应飞时间 (57)
    // uint8_t  payloadType;         // 载荷类型 (58)
    // uint8_t  ammoRemaining;       // 剩余弹量 (59)
    // uint8_t  selfTestResult;      // 自检结果 (60)
    // uint8_t  batteryVoltage;      // 电池电压 (61)
    // uint8_t  workflowStatus;      // 工作流程 (62)
    // uint8_t  alarmStatus;         // 报警状态字 (63)
    // uint8_t  switchStatus;        // 开关量状态 (64)
    // int32_t  targetLongitude;     // 目标经度 (65)
    // int32_t  targetLatitude;      // 目标纬度 (66)
    // int16_t  targetAltitude;      // 目标高度 (67)
    // int8_t   targetSpeed;         // 目标速度 (68)
    // int16_t  targetCourse;        // 目标航向 (69)
    // int32_t  reserved1;           // 预留1 (70)
    // int32_t  reserved2;           // 预留2 (71)
    // int32_t  reserved3;           // 预留3 (72)
    // int32_t  reserved4;           // 预留4 (73)
    
    // 校验和 (序号74)
    // uint8_t checksum; 
}UdpRecvDataPacket;
#pragma pack(pop)  // 恢复默认字节对齐
//FlightCtrlToDataLink;

// 后续可在此文件中填充地面站协议
// struct UdpRecvDataPacket
// {
// 	float data1;  // 数据1
// 	float data2;  // 数据2
// 	float data3;  // 数据3
// 	float data4;  // 数据4
// 	float data5;  // 数据5
// };

struct UdpSendDataPacket
{
	float data1;  // 数据1
	float data2;  // 数据2
	float data3;  // 数据3
};

// 握手数据包结构
struct UdpHandshakePacket
{
	char magic[4];      // 握手标识 "GCS\0"
	unsigned int version;     // 协议版本（使用unsigned int替代UINT32）
};
