#include <stdint.h>
// #include <string.h>

// UdpDataProtocol.h: UDP通信协议数据结构定义

#pragma once

// UDP通信协议数据结构
#pragma pack(push, 1)  // 紧密打包，避免字节对齐填充
typedef struct {
    // 帧头 (序号1)
    // uint16_t frameHeader;  // 固定值0xAA55 (小端存储: 内存中为55 AA)
    // uint16_t dataLength;   // 整个数据帧长度(含帧头和校验和)
    
    // 飞机状态数据 (序号3-74)
    //uint8_t  aircraftID;           // 飞机编号 (3)           系统信息
    int16_t  pitchAngle;           // 俯仰角 (4)             HUD组1 协议(1)
    int16_t  rollAngle;            // 滚转角 (5)             HUD组1 协议(2)
    int16_t  yawAngle;             // 航向角 (6)             HUD组1 协议(3)
    int16_t  normalOverload;       // 法向过载 (7)           HUD组1 协议(4-1)
    int16_t  attackAngle;          // 攻角 (8)               HUD组1 协议(5)
    int16_t  sideslipAngle;        // 侧滑角 (9)             HUD组1 协议(6)
    int8_t   rudderCmd1;           // 1#舵偏指令 (10)
    int8_t   rudderCmd2;           // 2#舵偏指令 (11)
    int8_t   rudderCmd3;           // 3#舵偏指令 (12)
    int8_t   rudderCmd4;           // 4#舵偏指令 (13)
    uint8_t  throttle;             // 油门控制 (14)          HUD组2 协议(1-1)
    //uint8_t  controlCommand;     // 控制指令 (15) 已拆解
    uint8_t controlCommand_D0;        // 开伞指令标志        视窗组4-5 IDC_RADIO_Flag1 扩展协议
    uint8_t controlCommand_D1;        // 开舱指令标志        视窗组4-5 IDC_RADIO_Flag2 扩展协议
    uint8_t controlCommand_D2;        // 起落架指令标志      视窗组4-5 IDC_RADIO_Flag3 扩展协议
    int8_t   turnRudderCmd;        // 转弯舵机指令 (16)
    int16_t  pitchRate;            // 俯仰角速率 (17)
    int16_t  rollRate;             // 滚转角速率 (18)
    int16_t  yawRate;              // 航向角速率 (19)
    int16_t  pitchAcceleration;    // 俯仰角加速度 (20)
    int16_t  rollAcceleration;     // 滚转角加速度 (21)
    int16_t  yawAcceleration;      // 航向角加速度 (22)
    int16_t  longitudinalOverload; // 纵向过载 (23)            HUD组1 协议(4-2)
    int16_t  lateralOverload;      // 横向过载 (24)            HUD组1 协议(4-3)
    int16_t  engineTemp;           // 发动机缸温 (25)           HUD组2 协议(2-2)
    int16_t  engineRPM;            // 发动机转速 (26)          HUD组1 协议(7)
    uint8_t  fuelRemaining;        // 剩余油量 (27)            HUD组2 协议(2-1)
    int16_t  gpsAltitude;          // 卫星高度 (28)
    uint8_t  gpsStatus;            // 卫星定位状态 (29)        HUD组2 协议(3-2)
    uint8_t  satelitesNum;         // 卫星收星数 (30)          HUD组2 协议(3-1)
    int16_t  gpsCourse;            // 卫星地速航向 (31)    HUD层 地图飞机标识方向驱动1
    int16_t  gpsGroundSpeed;       // 卫星地速 (32)        HUD层 主界面底部信息栏 1-1
    uint8_t  gpsVerticalSpeed;     // 卫星垂直速度 (33)    HUD层 主界面底部信息栏 1-2
    int32_t  longitude;            // 卫星经度 (34)        HUD层 地图飞机标识位置驱动1
    int32_t  latitude;             // 卫星纬度 (35)        HUD层 地图飞机标识位置驱动2
    int16_t  eastVelocity;         // 东向速度 (36)
    int16_t  northVelocity;        // 北向速度 (37)
    int16_t  verticalVelocity;     // 天向速度 (38)       
    int8_t   airTemperature;       // 大气温度 (39)
    int16_t  baroAltitude;         // 气压高度 (40)         HUD组1 协议(8)
    int16_t  baroAirspeed;         // 气压空速 (41)
    int16_t  indicatedAirspeed;    // 表速 (42)             HUD组1 协议(9)
    uint8_t  machNumber;           // 马赫数 (43)           HUD组1 协议(10)
    uint16_t radioAltitude;        // 无线电高度 (44)
    uint8_t  navStatus;            // 导航状态 (45)        HUD组2 协议(4-1)
    uint8_t  gpsHour;              // GPS时 (46)           HUD层 主界面底部信息栏 1-3.1
    uint8_t  gpsMinute;            // GPS分 (47)           HUD层 主界面底部信息栏 1-3.2
    uint8_t  gpsSecond;            // GPS秒 (48)           HUD层 主界面底部信息栏 1-3.3
    uint8_t  routeNumber;          // 当前航线号 (49)
    uint8_t  targetWaypoint;       // 目标航点 (50)           HUD组2 协议(4-2)
    int16_t  crossTrackError;      // 偏航距 (51)             HUD组2 协议(5-2)
    int16_t  courseDeviation;      // 偏航角 (52)             HUD组2 协议(6-2)
    int16_t  distanceToGo;         // 待飞距 (53)             HUD组2 协议(5-1)
    uint8_t  commandHeading;       // 应飞航向 (54)           HUD组2 协议(6-1)
    uint16_t commandSpeed;         // 应飞速度 (55)           HUD组2 协议(7-1)
    uint16_t commandAltitude;      // 应飞高度 (56)           HUD组2 协议(7-2)
    uint8_t  commandTime;          // 应飞时间 (57)           HUD组2 协议(8-1)
    uint8_t  payloadType;          // 载荷类型 (58)           HUD组2 协议(8-2)
    uint8_t  ammoRemaining;        // 剩余弹量 (59)           HUD组2 协议(9-1)
    uint8_t  selfTestResult;       // 自检结果 (60)           HUD组2 协议(9-2)
    uint8_t  YIS100A_result;         // IMU自检结果      视窗组9-0 IDC_Display66 扩展协议 pag1
    uint8_t  HP5804_result;          // 气压计自检结果   视窗组9-1 IDC_Display67 扩展协议 pag1
    uint8_t  MS4525D_result;         // 空速计自检结果   视窗组9-2 IDC_Display68 扩展协议 pag1
    uint8_t  M401_result;            // 温度计自检结果   视窗组9-3 IDC_Display69 扩展协议 pag1
    uint8_t  GPS_result;             // GPS自检结果      视窗组9-4 IDC_Display70 扩展协议 pag1
    uint8_t  PAC1931_result1;        // 电压自检结果     视窗组9-5 IDC_Display71 扩展协议 pag1
    uint8_t  can_to_pw_result;       // PWM自检结果     视窗组9-6 IDC_Display72 扩展协议 pag1
    uint8_t  SBUS_result;            // SBUS自检结果    视窗组9-7 IDC_Display73 扩展协议 pag1
    uint8_t  batteryVoltage;       // 电池电压 (61)
    //uint8_t  workflowStatus;     // 工作流程 (62)    已拆解
    uint8_t     workflowStatus_B0;   // 工作流程标志           视窗组8-4 0时激活IDC_RADIO_Flag4 1时激活IDC_RADIO_Flag4 扩展协议
    uint8_t     workflowStatus_B1;   // 发射状态标志           视窗组8-4 IDC_RADIO_Flag6 扩展协议
    //uint8_t  alarmStatus;        // 报警状态字 (63)  已拆解
    uint8_t     alarmStatus_B0;      // 电池电压低报警标志        顶层图层中央横幅报警1
    uint8_t     alarmStatus_B1;      // 高度报警标志              顶层图层中央横幅报警2
    uint8_t     alarmStatus_B2;      // 油量低报警标志            顶层图层中央横幅报警3
    uint8_t     alarmStatus_B3;      // 转速异常报警标志          顶层图层中央横幅报警4
    uint8_t     alarmStatus_B4;      // 空速异常报警标志          顶层图层中央横幅报警5
    uint8_t     alarmStatus_B5;      // GPS定位精度低报警标志     顶层图层中央横幅报警6
    // uint8_t  switchStatus;        // 开关量状态 (64) 已拆解
    uint8_t     switchStatus_B0;     // 发动机并网状态            HUD组3 协议(3-2)
    uint8_t     switchStatus_B1;     // 发动机启动状态            HUD组3 协议(1-1)
    uint8_t     switchStatus_B2;     // 盘旋状态                 HUD组3 协议(2-1)             
    uint8_t     switchStatus_B3;     // 归航状态                 HUD组3 协议(2-2)
    uint8_t     switchStatus_B4;     // 关车状态                 HUD组3 协议(1-2)
    uint8_t     switchStatus_B5;     // 起落架收放状态            HUD组3 协议(3-1)
    uint8_t     switchStatus_B6;     // 开伞状态                 HUD组3 协议(3-2)
    uint8_t     switchStatus_B7;     // 夜航灯开关状态            HUD组3 协议(3-1)
    int32_t  targetLongitude;     // 目标经度 (65)
    int32_t  targetLatitude;      // 目标纬度 (66)
    int16_t  targetAltitude;      // 目标高度 (67)
    int8_t   targetSpeed;         // 目标速度 (68)
    int16_t  targetCourse;        // 目标航向 (69)

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

// 航路点结构体定义
struct Waypoint
{
    int32_t longitude;  // 经度
    int32_t latitude;   // 纬度
    int16_t altitude;   // 高度
};

// 地面站发送数据包结构
struct UdpSendDataPacket
{
            
    // 任务指令（使用 Radio Button 控件：IDC_RADIO_Flag22~28）
    uint8_t missionCommand_B0;                  // IDC_RADIO_Flag22激活时, 此参数置0，为地面测试流程指令，
                                                // IDC_RADIO_Flag23激活时，此参数置1，为发射流程指令，
    uint8_t missionCommand_B1;                  // IDC_RADIO_Flag24激活时，此参数置1，未激活置0  自检指令
    uint8_t missionCommand_B2;                  // IDC_RADIO_Flag25激活时，此参数置1，未激活置0  参数装订指令
    uint8_t missionCommand_B3;                  // IDC_RADIO_Flag26激活时，此参数置1，未激活置0  舵面检查指令
    uint8_t missionCommand_B4;                  // IDC_RADIO_Flag27激活时，此参数置1，未激活置0  发动机检查指令
    uint8_t missionCommand_B5;                  // IDC_RADIO_Flag28激活时，此参数置1，未激活置0  发射指令

    // 控制模式（使用 Radio Button 控件：IDC_RADIO_Flag29~31）
    uint8_t controlMode_B0;                         // IDC_RADIO_Flag29激活时，此参数置0，为手动遥控
                                                    // IDC_RADIO_Flag30激活时，此参数置1，为半自主
                                                    // IDC_RADIO_Flag31激活时，此参数置2，为全自主

    // 发射点 
    int32_t launchLongitude;                     // IDC_Display_EditData2
    int32_t launchLatitude;                      // IDC_Display_EditData3
    int16_t launchAltitude;                      // IDC_Display_EditData4
            
    // 初始姿态
    int16_t initPitch;                           // IDC_Display_EditData5
    int16_t initYaw;                             // IDC_Display_EditData6
    int16_t initRoll;                            // IDC_Display_EditData7
            
    // 初始角速率
    int16_t initPitchRate;                       // IDC_Display_EditData8
    int16_t initYawRate;                         // IDC_Display_EditData9
    int16_t initRollRate;                        // IDC_Display_EditData10
            
    // 初始速度 
    int16_t initNorthVelocity;                   // IDC_Display_EditData11
    int16_t initEastVelocity;                    // IDC_Display_EditData12
    int16_t initVerticalVelocity;                // IDC_Display_EditData13
            
    // 初始加速度
    int16_t initNorthAccel;                      // IDC_Display_EditData14
    int16_t initEastAccel;                       // IDC_Display_EditData15
    int16_t initVerticalAccel;                   // IDC_Display_EditData16
            
    // 航路点数组
    Waypoint waypoints[100];  // 100个航路点
            
    // 目标点 
    int32_t targetLongitude;                     // IDC_Display_EditData17
    int32_t targetLatitude;                      // IDC_Display_EditData18
    int16_t targetAltitude;                      // IDC_Display_EditData19
            
    // 发射点(重复?) (序号30-32)
    int32_t launchLongitude2;                    // IDC_Display_EditData20
    int32_t launchLatitude2;                     // IDC_Display_EditData21
    int16_t launchAltitude2;                     // IDC_Display_EditData22
            
    // 开伞点 (序号33-35)
    int32_t parachuteLongitude;                  // IDC_Display_EditData23
    int32_t parachuteLatitude;                   // IDC_Display_EditData24
    int16_t parachuteAltitude;                   // IDC_Display_EditData25
            
    // 控制指令 (序号36-38)
    int8_t elevatorCmd;                          // IDC_Display_EditData26
    int8_t aileronCmd;                           // IDC_Display_EditData27
    uint8_t airspeedSet;                         // IDC_Display_EditData28

	// float data1;  // 数据1
	// float data2;  // 数据2
	// float data3;  // 数据3
    // float data4;  // 数据4
    // float data5;  // 数据5
};
#pragma pack(pop)  // 恢复默认字节对齐

// 握手数据包结构
struct UdpHandshakePacket
{
	char magic[4];      // 握手标识 "GCS\0"
	unsigned int version;     // 协议版本（使用unsigned int替代UINT32）
};

// // 计算帧长度宏
// #define FLIGHTCTRL_FRAME_SIZE sizeof(FlightCtrlToDataLink)

// // 初始化飞控数据结构
// void initFlightCtrlFrame(FlightCtrlToDataLink *frame) {
//     memset(frame, 0, sizeof(FlightCtrlToDataLink));
//     frame->frameHeader = 0xAA55;
//     frame->dataLength = FLIGHTCTRL_FRAME_SIZE;
// }

// // 计算校验和函数
// uint8_t calculateChecksum(const void* data, size_t len) {
//     const uint8_t *bytes = (const uint8_t*)data;
//     uint8_t sum = 0;
//     for(size_t i = 0; i < len; i++) {
//         sum += bytes[i];
//     }
//     return sum;
// }
