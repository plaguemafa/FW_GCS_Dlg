#include <stdint.h>
// #include <string.h>

// UdpDataProtocol.h: UDP通信协议数据结构定义

#pragma once

// UDP通信协议数据结构
#pragma pack(push, 1)  // 紧密打包，避免字节对齐填充
typedef struct {
    // 帧头 (序号1)
    // uint16_t frameHeader;  // 固定值0xAA55 (小端存储: 内存中为55 AA)
    
    // // 字节长度 (序号2)
    // uint16_t dataLength;   // 整个数据帧长度(含帧头和校验和)
    
    // 飞机状态数据 (序号3-74)
    // 重要：结构体成员顺序必须与发送端FlightCtrlToDataLink完全一致！严格按照协议序号排列
    // ============================================================================
    // uint8_t  aircraftID;          // 飞机编号 (3)              (系统信息，暂不显示)
    int16_t  pitchAngle;          // 俯仰角 (4)                 视窗组1-0 IDC_Display0
    int16_t  rollAngle;           // 滚转角 (5)                 视窗组1-1 IDC_Display1
    int16_t  yawAngle;            // 航向角 (6)                 视窗组1-2 IDC_Display2
    int16_t  normalOverload;      // 法向过载 (7)            视窗组1-11 IDC_Display11
    int16_t  attackAngle;         // 攻角 (8)                   视窗组1-3 IDC_Display3
    int16_t  sideslipAngle;       // 侧滑角 (9)                 视窗组1-4 IDC_Display4
    int8_t   rudderCmd1;          // 1#舵偏指令 (10)           视窗组4-0 IDC_Display27
    int8_t   rudderCmd2;          // 2#舵偏指令 (11)           视窗组4-1 IDC_Display28
    int8_t   rudderCmd3;          // 3#舵偏指令 (12)           视窗组4-2 IDC_Display29
    int8_t   rudderCmd4;          // 4#舵偏指令 (13)           视窗组4-3 IDC_Display30
    uint8_t  throttle;            // 油门控制 (14)           视窗组3-0 IDC_Display23
    uint8_t  controlCommand;      // 控制指令 (15)             视窗组4-5 IDC_Display32
    int8_t   turnRudderCmd;       // 转弯舵机指令 (16)         视窗组4-4 IDC_Display31
    int16_t  pitchRate;           // 俯仰角速率 (17)         视窗组1-5 IDC_Display5
    int16_t  rollRate;            // 滚转角速率 (18)         视窗组1-6 IDC_Display6
    int16_t  yawRate;             // 航向角速率 (19)         视窗组1-7 IDC_Display7
    int16_t  pitchAcceleration;   // 俯仰角加速度 (20)       视窗组1-8 IDC_Display8
    int16_t  rollAcceleration;    // 滚转角加速度 (21)       视窗组1-9 IDC_Display9
    int16_t  yawAcceleration;     // 航向角加速度 (22)       视窗组1-10 IDC_Display10
    int16_t  longitudinalOverload; // 纵向过载 (23)         视窗组1-12 IDC_Display12
    int16_t  lateralOverload;     // 横向过载 (24)          视窗组1-13 IDC_Display13
    int16_t  engineTemp;          // 发动机缸温 (25)           视窗组3-1 IDC_Display24
    int16_t  engineRPM;           // 发动机转速 (26)           视窗组3-2 IDC_Display25
    uint8_t  fuelRemaining;       // 剩余油量 (27)             视窗组3-3 IDC_Display26
    int16_t  gpsAltitude;         // 卫星高度 (28)           视窗组5-2 IDC_Display35
    uint8_t  gpsStatus;           // 卫星定位状态 (29)       视窗组5-3 IDC_Display36
    uint8_t  satelitesNum;        // 卫星收星数 (30)         视窗组5-4 IDC_Display37
    int16_t  gpsCourse;           // 卫星地速航向 (31)       视窗组5-5 IDC_Display38
    int16_t  gpsGroundSpeed;      // 卫星地速 (32)           视窗组5-6 IDC_Display39
    uint8_t  gpsVerticalSpeed;    // 卫星垂直速度 (33)       视窗组5-7 IDC_Display40
    int32_t  longitude;           // 卫星经度 (34)           视窗组5-0 IDC_Display33
    int32_t  latitude;            // 卫星纬度 (35)           视窗组5-1 IDC_Display34
    int16_t  eastVelocity;        // 东向速度 (36)              视窗组2-5 IDC_Display19
    int16_t  northVelocity;       // 北向速度 (37)              视窗组2-6 IDC_Display20
    int16_t  verticalVelocity;    // 天向速度 (38)              视窗组2-7 IDC_Display21
    int8_t   airTemperature;      // 大气温度 (39)              视窗组2-8 IDC_Display22
    int16_t  baroAltitude;        // 气压高度 (40)              视窗组2-0 IDC_Display14
    int16_t  baroAirspeed;        // 气压空速 (41)              视窗组2-2 IDC_Display16
    int16_t  indicatedAirspeed;   // 表速 (42)                  视窗组2-3 IDC_Display17
    uint8_t  machNumber;          // 马赫数 (43)                视窗组2-4 IDC_Display18
    uint16_t radioAltitude;       // 无线电高度 (44)            视窗组2-1 IDC_Display15
    uint8_t  navStatus;           // 导航状态 (45)          视窗组6-0 IDC_Display44
    uint8_t  gpsHour;             // GPS时 (46)               视窗组5-8 IDC_Display41
    uint8_t  gpsMinute;           // GPS分 (47)               视窗组5-9 IDC_Display42
    uint8_t  gpsSecond;           // GPS秒 (48)               视窗组5-10 IDC_Display43
    uint8_t  routeNumber;         // 当前航线号 (49)         视窗组6-1 IDC_Display45
    uint8_t  targetWaypoint;      // 目标航点 (50)           视窗组6-2 IDC_Display46
    int16_t  crossTrackError;     // 偏航距 (51)             视窗组6-3 IDC_Display47
    int16_t  courseDeviation;     // 偏航角 (52)             视窗组6-4 IDC_Display48
    int16_t  distanceToGo;        // 待飞距 (53)             视窗组6-5 IDC_Display49
    uint8_t  commandHeading;      // 应飞航向 (54)           视窗组6-6 IDC_Display50
    uint16_t commandSpeed;        // 应飞速度 (55)           视窗组6-7 IDC_Display51
    uint16_t commandAltitude;     // 应飞高度 (56)           视窗组6-8 IDC_Display52
    uint8_t  commandTime;         // 应飞时间 (57)           视窗组6-9 IDC_Display53
    uint8_t  payloadType;         // 载荷类型 (58)             视窗组8-0 IDC_Display59
    uint8_t  ammoRemaining;       // 剩余弹量 (59)             视窗组8-1 IDC_Display60
    uint8_t  selfTestResult;      // 自检结果 (60)             视窗组8-2 IDC_Display61
    uint8_t  YIS100A_result;      //IMU自检结果            视窗组9-0 IDC_Display66
    uint8_t  HP5804_result;       //气压计自检结果         视窗组9-1 IDC_Display67
    uint8_t  MS4525D_result;      // 空速计自检结果        视窗组9-2 IDC_Display68
    uint8_t  M401_result;         // 温度计自检结果        视窗组9-3 IDC_Display69
    uint8_t  GPS_result;          // GPS自检结果          视窗组9-4 IDC_Display70
    uint8_t  PAC1931_result1;     // 电压自检结果         视窗组9-5 IDC_Display71
    uint8_t  can_to_pw_result;    // PWM自检结果          视窗组9-6 IDC_Display72
    uint8_t  SBUS_result;         // SBUS自检结果         视窗组9-7 IDC_Display73
    uint8_t  batteryVoltage;      // 电池电压 (61)             视窗组8-3 IDC_Display62
    uint8_t  workflowStatus;      // 工作流程 (62)             视窗组8-4 IDC_Display63
    uint8_t  alarmStatus;         // 报警状态字 (63)           视窗组8-5 IDC_Display64
    uint8_t  switchStatus;        // 开关量状态 (64)           视窗组8-6 IDC_Display65
    int32_t  targetLongitude;     // 目标经度 (65)           视窗组7-0 IDC_Display54
    int32_t  targetLatitude;      // 目标纬度 (66)           视窗组7-1 IDC_Display55
    int16_t  targetAltitude;      // 目标高度 (67)           视窗组7-2 IDC_Display56
    int8_t   targetSpeed;         // 目标速度 (68)           视窗组7-3 IDC_Display57
    int16_t  targetCourse;        // 目标航向 (69)           视窗组7-4 IDC_Display58
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

// 地面站发送数据包结构（需要1字节对齐，避免结构体填充）
#pragma pack(push, 1)  // 紧密打包，避免字节对齐填充
struct UdpSendDataPacket
{
	float data1;  // 数据1
	float data2;  // 数据2
	float data3;  // 数据3
    float data4;  // 数据4
    float data5;  // 数据5

};
#pragma pack(pop)  // 恢复默认字节对齐

// 握手数据包结构
struct UdpHandshakePacket
{
	char magic[4];      // 握手标识 "GCS\0"
	unsigned int version;     // 协议版本（使用unsigned int替代UINT32）
};


// 飞控端数据源代码
// // ================= 飞控系统 -> 数据链 (表1) =================
// #pragma pack(push, 1)  // 1字节对齐
// typedef struct {
//     // 帧头 (序号1)
//     U16 frameHeader;  // 固定值0xAA55 (小端存储: 内存中为55 AA)
    
//     // 字节长度 (序号2)
//     U16 dataLength;   // 整个数据帧长度(含帧头和校验和)
    
//     // 飞机状态数据 (序号3-74)
//     U8  aircraftID;          // 飞机编号 (3)
//     S16 pitchAngle;          // 俯仰角 (4)
//     S16 rollAngle;           // 滚转角 (5)
//     S16 yawAngle;            // 航向角 (6)
//     S16 normalOverload;      // 法向过载 (7)
//     S16 attackAngle;         // 攻角 (8)
//     S16 sideslipAngle;       // 侧滑角 (9)
//     S8  rudderCmd1;          // 1#舵偏指令 (10)
//     S8  rudderCmd2;          // 2#舵偏指令 (11)
//     S8  rudderCmd3;          // 3#舵偏指令 (12)
//     S8  rudderCmd4;          // 4#舵偏指令 (13)
//     U8  throttle;            // 油门控制 (14)
//     U8  controlCommand;      // 控制指令 (15)
//     S8  turnRudderCmd;       // 转弯舵机指令 (16)
//     S16 pitchRate;           // 俯仰角速率 (17)
//     S16 rollRate;            // 滚转角速率 (18)
//     S16 yawRate;             // 航向角速率 (19)
//     S16 pitchAcceleration;   // 俯仰角加速度 (20)
//     S16 rollAcceleration;    // 滚转角加速度 (21)
//     S16 yawAcceleration;     // 航向角加速度 (22)
//     S16 longitudinalOverload; // 纵向过载 (23)
//     S16 lateralOverload;     // 横向过载 (24)
//     S16 engineTemp;          // 发动机缸温 (25)
//     S16 engineRPM;           // 发动机转速 (26)
//     U8  fuelRemaining;       // 剩余油量 (27)
//     S16 gpsAltitude;         // 卫星高度 (28)
//     U8  gpsStatus;           // 卫星定位状态 (29)
//     U8  satelitesNum;        // 卫星收星数 (30)
//     S16 gpsCourse;           // 卫星地速航向 (31)
//     S16 gpsGroundSpeed;      // 卫星地速 (32)
//     U8  gpsVerticalSpeed;    // 卫星垂直速度 (33)
//     S32 longitude;           // 卫星经度 (34)
//     S32 latitude;            // 卫星纬度 (35)
//     S16 eastVelocity;        // 东向速度 (36)
//     S16 northVelocity;       // 北向速度 (37)
//     S16 verticalVelocity;    // 天向速度 (38)
//     S8  airTemperature;      // 大气温度 (39)
//     S16 baroAltitude;        // 气压高度 (40)
//     S16 baroAirspeed;        // 气压空速 (41)
//     S16 indicatedAirspeed;   // 表速 (42)
//     U8  machNumber;          // 马赫数 (43)
//     U16 radioAltitude;       // 无线电高度 (44)
//     U8  navStatus;           // 导航状态 (45)
//     U8  gpsHour;             // GPS时 (46)
//     U8  gpsMinute;           // GPS分 (47)
//     U8  gpsSecond;           // GPS秒 (48)
//     U8  routeNumber;         // 当前航线号 (49)
//     U8  targetWaypoint;      // 目标航点 (50)
//     S16 crossTrackError;     // 偏航距 (51)
//     S16 courseDeviation;     // 偏航角 (52)
//     S16 distanceToGo;        // 待飞距 (53)
//     U8  commandHeading;      // 应飞航向 (54)
//     U16 commandSpeed;        // 应飞速度 (55) 注意: 协议类型U8但字节数2，按实际定义
//     U16 commandAltitude;     // 应飞高度 (56) 同上
//     U8  commandTime;         // 应飞时间 (57)
//     U8  payloadType;         // 载荷类型 (58)
//     U8  ammoRemaining;       // 剩余弹量 (59)
//     U8  selfTestResult;      // 自检结果 (60)
//     U8  batteryVoltage;      // 电池电压 (61)
//     U8  workflowStatus;      // 工作流程 (62)
//     U8  alarmStatus;         // 报警状态字 (63)
//     U8  switchStatus;        // 开关量状态 (64)
//     S32 targetLongitude;     // 目标经度 (65)
//     S32 targetLatitude;      // 目标纬度 (66)
//     S16 targetAltitude;      // 目标高度 (67)
//     S8  targetSpeed;         // 目标速度 (68)
//     S16 targetCourse;        // 目标航向 (69)
//     S32 reserved1;           // 预留1 (70)
//     S32 reserved2;           // 预留2 (71)
//     S32 reserved3;           // 预留3 (72)
//     S32 reserved4;           // 预留4 (73)
    
//     // 校验和 (序号74)
//     U8 checksum; 
// } FlightCtrlToDataLink;
// #pragma pack(pop)  // 恢复默认对齐

// // 计算帧长度宏
// #define FLIGHTCTRL_FRAME_SIZE sizeof(FlightCtrlToDataLink)

// // 初始化飞控数据结构
// void initFlightCtrlFrame(FlightCtrlToDataLink *frame) {
//     memset(frame, 0, sizeof(FlightCtrlToDataLink));
//     frame->frameHeader = 0xAA55;
//     frame->dataLength = FLIGHTCTRL_FRAME_SIZE;
// }

// // 计算校验和函数
// U8 calculateChecksum(const void* data, size_t len) {
//     const U8 *bytes = (const U8*)data;
//     U8 sum = 0;
//     for(size_t i = 0; i < len; i++) {
//         sum += bytes[i];
//     }
//     return sum;
// }

// // ================= 数据链 -> 飞控系统 (表2) =================
// #pragma pack(push, 1)
// typedef struct {
//     // 航路点结构体
//     typedef struct {
//         S32 longitude;  // 经度
//         S32 latitude;   // 纬度
//         S16 altitude;    // 高度
//     } Waypoint;

//     // 帧头 (序号1)
//     U16 frameHeader;  // 固定值0x55AA (小端存储: 内存中为AA 55)
    
//     // 字节长度 (序号2)
//     U16 dataLength;    // 整个数据帧长度
    
//     // 任务指令 (序号3)
//     U8 missionCommand;
    
//     // 控制模式 (序号4)
//     U8 controlMode;
    
//     // 发射点 (序号5-7)
//     S32 launchLongitude;
//     S32 launchLatitude;
//     S16 launchAltitude;
    
//     // 初始姿态 (序号8-10)
//     S16 initPitch;
//     S16 initYaw;
//     S16 initRoll;
    
//     // 初始角速率 (序号11-13)
//     S16 initPitchRate;
//     S16 initYawRate;
//     S16 initRollRate;
    
//     // 初始速度 (序号14-16)
//     S16 initNorthVelocity;
//     S16 initEastVelocity;
//     S16 initVerticalVelocity;
    
//     // 初始加速度 (序号17-19)
//     S16 initNorthAccel;
//     S16 initEastAccel;
//     S16 initVerticalAccel;
    
//     // 航路点数组 (序号20-26)
//     Waypoint waypoints[100];  // 100个航路点
    
//     // 目标点 (序号27-29)
//     S32 targetLongitude;
//     S32 targetLatitude;
//     S16 targetAltitude;
    
//     // 发射点(重复?) (序号30-32)
//     S32 launchLongitude2;
//     S32 launchLatitude2;
//     S16 launchAltitude2;
    
//     // 开伞点 (序号33-35)
//     S32 parachuteLongitude;
//     S32 parachuteLatitude;
//     S16 parachuteAltitude;
    
//     // 控制指令 (序号36-38)
//     S8 elevatorCmd;
//     S8 aileronCmd;
//     U8 airspeedSet;
    
//     // 预留字段 (序号39-42)
//     S32 reserved1;
//     S32 reserved2;
//     S32 reserved3;
//     S32 reserved4;
    
//     // 校验和 (序号43)
//     U8 checksum;
// } DataLinkToFlightCtrl;
// #pragma pack(pop)
