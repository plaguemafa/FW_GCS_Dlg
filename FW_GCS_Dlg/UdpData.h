#include <stdint.h>
// #include <string.h>

// UdpDataProtocol.h: 通信协议数据结构定义

#pragma once

// UDP通信协议数据结构
#pragma pack(push, 1)             // 紧密打包，避免字节对齐填充
typedef struct {
    uint16_t frameHeader;          // 帧头  固定值0xAA55
    uint8_t  aircraftID;           // 飞机编号 
    int16_t  pitchAngle;           // 俯仰角                 deg x10         Canvas组1 协议(1)
    int16_t  rollAngle;            // 滚转角                 deg x10         Canvas组1 协议(2)
    int16_t  yawAngle;             // 航向角                 deg x10         Canvas组1 协议(3)
    int16_t  normalOverload;       // 法向过载                 / x10         Canvas组1 协议(4-1)
    int16_t  attackAngle;          // 攻角                   deg x10         Canvas组1 协议(5)
    int16_t  sideslipAngle;        // 侧滑角                 deg x10         Canvas组1 协议(6)
    int8_t   rudderCmd1;           // 1#舵偏指令               / x1
    int8_t   rudderCmd2;           // 2#舵偏指令               / x1
    int8_t   rudderCmd3;           // 3#舵偏指令               / x1
    int8_t   rudderCmd4;           // 4#舵偏指令               / x1
    uint8_t  throttle;             // 油门控制           [1,100] x1         Canvas组2 协议(1-1)
    uint8_t controlCommand_D0;     // 开伞指令标志             / x1
    uint8_t controlCommand_D1;     // 开舱指令标志             / x1
    uint8_t controlCommand_D2;     // 起落架指令标志           / x1
    int8_t   turnRudderCmd;        // 转弯舵机指令             / x1
    int16_t  pitchRate;            // 俯仰角速率           deg/s x10
    int16_t  rollRate;             // 滚转角速率           deg/s x10
    int16_t  yawRate;              // 航向角速率           deg/s x10
    int16_t  pitchAcceleration;    // 俯仰角加速度       deg/s^2 x10
    int16_t  rollAcceleration;     // 滚转角加速度       deg/s^2 x10
    int16_t  yawAcceleration;      // 航向角加速度       deg/s^2 x10
    int16_t  longitudinalOverload; // 纵向过载                 / x10        Canvas组1 协议(4-2)
    int16_t  lateralOverload;      // 横向过载                 / x10        Canvas组1 协议(4-3)
    int16_t  engineTemp;           // 发动机缸温               C x1         Canvas组2 协议(2-2)
    int16_t  engineRPM;            // 发动机转速             rpm x1         Canvas组1 协议(7)
    uint8_t  fuelRemaining;        // 剩余油量                 L x1         Canvas组2 协议(2-1)
    int16_t  gpsAltitude;          // 卫星高度                 m x10            绘图层 地图飞机标识高度1
    uint8_t  gpsStatus;            // 卫星定位状态             / x1         Canvas组2 协议(3-2)
    uint8_t  satelitesNum;         // 卫星收星数               / x1         Canvas组2 协议(3-1)
    int16_t  gpsCourse;            // 卫星地速航向           deg x10             绘图层 地图飞机标识方向1
    int16_t  gpsGroundSpeed;       // 卫星地速               m/s x10            绘图层 主界面底部信息栏 1-1
    uint8_t  gpsVerticalSpeed;     // 卫星垂直速度           m/s x10             绘图层 主界面底部信息栏 1-2
    int32_t  longitude;            // 卫星经度                度 x10000000       绘图层 地图飞机标识位置1
    int32_t  latitude;             // 卫星纬度                度 x10000000       绘图层 地图飞机标识位置2
    int16_t  eastVelocity;         // 东向速度               m/s x10
    int16_t  northVelocity;        // 北向速度               m/s x10
    int16_t  verticalVelocity;     // 天向速度               m/s x10
    int8_t   airTemperature;       // 大气温度                 C x1
    int16_t  baroAltitude;         // 气压高度                 m x10         Canvas组1 协议(8)
    int16_t  baroAirspeed;         // 气压空速               m/s x10
    int16_t  indicatedAirspeed;    // 表速                   m/s x10         Canvas组1 协议(9)
    uint8_t  machNumber;           // 马赫数                   / x100         Canvas组1 协议(10)
    uint16_t radioAltitude;        // 无线电高度               / x10          Canvas组1 协议(11)
    uint8_t  navStatus;            // 导航状态                 / x1           Canvas组2 协议(4-1)
    uint8_t  gpsHour;              // GPS时                    h x1         绘图层 主界面底部信息栏 1-3.1
    uint8_t  gpsMinute;            // GPS分                  min x1         绘图层 主界面底部信息栏 1-3.2
    uint8_t  gpsSecond;            // GPS秒                    s x1         绘图层 主界面底部信息栏 1-3.3
    uint8_t  routeNumber;          // 当前航线号                / x1         绘图层 主界面底部信息栏 1-3.4
    uint8_t  targetWaypoint;       // 目标航点                  / x1         Canvas组2 协议(4-2)
    int16_t  crossTrackError;      // 偏航距                    m x10         Canvas组2 协议(5-2)
    int16_t  courseDeviation;      // 偏航角                  deg x10         Canvas组2 协议(6-2)
    int16_t  distanceToGo;         // 待飞距                    m x10         Canvas组2 协议(5-1)
    uint8_t  commandHeading;       // 应飞航向                deg x10         Canvas组2 协议(6-1)
    uint16_t commandSpeed;         // 应飞速度                m/s x10         Canvas组2 协议(7-1)
    uint16_t commandAltitude;      // 应飞高度                  m x10         Canvas组2 协议(7-2)
    uint8_t  commandTime;          // 应飞时间                  s x1         Canvas组2 协议(8-1)
    uint8_t  payloadType;          // 载荷类型                  / x1         Canvas组2 协议(8-2)
    uint8_t  ammoRemaining;        // 剩余弹量                  / x1         Canvas组2 协议(9-1)
    uint8_t  selfTestResult;       // 自检结果                  / x1         Canvas组2 协议(9-2)
    uint8_t  YIS100A_result;       // IMU自检结果               / x1         视窗组9-0 IDC_Display66 扩展协议 pag1
    uint8_t  HP5804_result;        // 气压计自检结果             / x1         视窗组9-1 IDC_Display67 扩展协议 pag1
    uint8_t  MS4525D_result;       // 空速计自检结果             / x1         视窗组9-2 IDC_Display68 扩展协议 pag1
    uint8_t  M401_result;          // 温度计自检结果             / x1         视窗组9-3 IDC_Display69 扩展协议 pag1
    uint8_t  GPS_result;           // GPS自检结果               / x1         视窗组9-4 IDC_Display70 扩展协议 pag1
    uint8_t  PAC1931_result1;      // 电压自检结果              / x1         视窗组9-5 IDC_Display71 扩展协议 pag1
    uint8_t  can_to_pw_result;     // PWM自检结果               / x1         视窗组9-6 IDC_Display72 扩展协议 pag1
    uint8_t  SBUS_result;          // SBUS自检结果              / x1         视窗组9-7 IDC_Display73 扩展协议 pag1
    uint8_t  batteryVoltage;       // 电池电压                  / x1         Canvas组2 协议(1-2)
    uint8_t  workflowStatus_B0;    // 工作流程标志               / x1         视窗组8-4 0时激活IDC_RADIO_Flag4 1时激活IDC_RADIO_Flag4 扩展协议
    uint8_t  workflowStatus_B1;    // 发射状态标志               / x1         视窗组8-4 IDC_RADIO_Flag6 扩展协议
    uint8_t  alarmStatus_B0;       // 电池电压低报警标志          / x1         顶层图层中央横幅报警1
    uint8_t  alarmStatus_B1;       // 高度报警标志               / x1         顶层图层中央横幅报警2
    uint8_t  alarmStatus_B2;       // 油量低报警标志             / x1         顶层图层中央横幅报警3
    uint8_t  alarmStatus_B3;       // 转速异常报警标志           / x1         顶层图层中央横幅报警4
    uint8_t  alarmStatus_B4;       // 空速异常报警标志           / x1         顶层图层中央横幅报警5
    uint8_t  alarmStatus_B5;       // GPS定位精度低报警标志      / x1         顶层图层中央横幅报警6
    uint8_t  switchStatus_B0;      // 发动机并网状态             / x1         Canvas组3 协议(3-1)
    uint8_t  switchStatus_B1;      // 发动机启动状态             / x1         Canvas组3 协议(1-1)
    uint8_t  switchStatus_B2;      // 盘旋状态                  / x1         Canvas组3 协议(2-1)      
    uint8_t  switchStatus_B3;      // 归航状态                  / x1         Canvas组3 协议(2-2)
    uint8_t  switchStatus_B4;      // 关车状态                  / x1         Canvas组3 协议(1-2)
    uint8_t  switchStatus_B5;      // 起落架收放状态             / x1         Canvas组3 协议(4-1)
    uint8_t  switchStatus_B6;      // 开伞状态                  / x1         Canvas组3 协议(3-2)
    uint8_t  switchStatus_B7;      // 夜航灯开关状态             / x1         Canvas组3 协议(4-2)
    int32_t   targetLongitude;     // 目标经度               deg x10000000       绘图层 地图目标标识位置1
    int32_t   targetLatitude;      // 目标纬度               deg x10000000       绘图层 地图目标标识位置2
    int16_t   targetAltitude;      // 目标高度                 m x10             绘图层 地图目标标识位置3
    int8_t    targetSpeed;         // 目标速度               m/s x10             绘图层 地图目标标识速度
    int16_t   targetCourse;        // 目标航向               deg x10             绘图层 地图目标标识方向

    // int32_t  reserved1;           // 预留1             
    // int32_t  reserved2;           // 预留2
    // int32_t  reserved3;           // 预留3
    // int32_t  reserved4;           // 预留4

    //uint8_t checksum;                //校验和，地面站接收数据仅驱动显示，不涉及安全性问题，暂不启用校验

}UdpRecvDataPacket;
#pragma pack(pop)  // 恢复默认字节对齐


// 地面站发送数据包结构（需要1字节对齐，避免结构体填充）
#pragma pack(push, 1)  // 紧密打包，避免字节对齐填充
// 地面站发送数据包结构    地面站帧头作为隐形msg_id使用
struct UdpSendDataPacket_Cmd{
    uint16_t frameHeader;                            // 固定值0xFF00
    // 任务指令
    uint8_t missionCommand_B0;                  // 置1，为发射流程指令     置0，为地面测试流程指令         
    uint8_t missionCommand_B1;                  // 置1，激活              置0，未激活                    自检指令
    uint8_t missionCommand_B2;                  // 置1，激活              置0，未激活                    参数装订指令
    uint8_t missionCommand_B3;                  // 置1，激活              置0，未激活                    舵面检查指令
    uint8_t missionCommand_B4;                  // 置1，激活              置0，未激活                    发动机检查指令
    uint8_t missionCommand_B5;                  // 置1，激活              置0，未激活                    发射指令
    uint8_t controlMode_B0;                     // 置2，全自主    置1，半自主     置0，手动遥控            控制模式                    
    uint8_t checksum;                           // 校验和 
};

struct Waypoint  // 航路点结构体定义
{
    int32_t longitude;  // 经度  x1E7
    int32_t latitude;   // 纬度  x1E7
    int16_t altitude;   // 高度  x1（注意使用高程值，无小数部分）
};

struct UdpSendDataPacket_Data{
    uint16_t frameHeader;                         // 固定值0xF00F
    //本结构体数据均于page2使用
    int32_t launchLongitude;                   // 发射点 经度              deg x10000000    IDC_Display_EditData2
    int32_t launchLatitude;                    // 发射点 纬度              deg x10000000    IDC_Display_EditData3
    int16_t launchAltitude;                    // 发射点 高度                m x10         IDC_Display_EditData4
       
    int16_t initPitch;                         // 初始姿态角 俯仰角          deg x10        IDC_Display_EditData5
    int16_t initYaw;                           // 初始姿态角 航向角          deg x10        IDC_Display_EditData6
    int16_t initRoll;                          // 初始姿态角 滚转角          deg x10        IDC_Display_EditData7
     
    int16_t initPitchRate;                      // 初始角速率 俯仰角速率      deg/s x10      IDC_Display_EditData8
    int16_t initYawRate;                        // 初始角速率 航向角速率      deg/s x10      IDC_Display_EditData9
    int16_t initRollRate;                       // 初始角速率 滚转角速率      deg/s x10      IDC_Display_EditData10
    

    int16_t initNorthVelocity;                  // 初始速度 北向速度          m/s x10      IDC_Display_EditData11
    int16_t initEastVelocity;                   // 初始速度 东向速度          m/s x10      IDC_Display_EditData12
    int16_t initVerticalVelocity;               // 初始速度 天向速度          m/s x10      IDC_Display_EditData13
    

    int16_t initNorthAccel;                     // 初始加速度 北向加速度      m/s^2 x10      IDC_Display_EditData14
    int16_t initEastAccel;                      // 初始加速度 东向加速度      m/s^2 x10      IDC_Display_EditData15
    int16_t initVerticalAccel;                  // 初始加速度 天向加速度      m/s^2 x10      IDC_Display_EditData16
    
    // 航路点数组
    Waypoint waypoints[100];  // 100个航路点
    
    int32_t targetLongitude;                  // 目标点 经度                 deg x10000000    IDC_Display_EditData17
    int32_t targetLatitude;                   // 目标点 纬度                 deg x10000000    IDC_Display_EditData18
    int16_t targetAltitude;                   // 目标点 高度                   m x10          IDC_Display_EditData19
    

    int32_t launchLongitude2;                  // 发射点(重复?) 经度          deg x10000000    IDC_Display_EditData20
    int32_t launchLatitude2;                   // 发射点(重复?) 纬度          deg x10000000    IDC_Display_EditData21
    int16_t launchAltitude2;                   // 发射点(重复?) 高度            m x10          IDC_Display_EditData22

    int32_t parachuteLongitude;                 // 开伞点 经度               deg x10000000    IDC_Display_EditData23
    // 开伞点      // IDC_Display_EditData23
    int32_t parachuteLatitude;                  // 开伞点 纬度                deg x10000000    IDC_Display_EditData24
    int16_t parachuteAltitude;                  // 开伞点 高度                  m x10          IDC_Display_EditData25
    

    int8_t elevatorCmd;                          // 控制指令 俯仰舵偏指令        / x1          IDC_Display_EditData26
    int8_t aileronCmd;                           // 控制指令 滚转舵偏指令        / x1          IDC_Display_EditData27
    uint8_t airspeedSet;                         // 控制指令 空速设定值        m/s x10         IDC_Display_EditData28

    uint8_t checksum;                          // 校验和 
};
#pragma pack(pop)  // 恢复默认字节对齐

// // 计算帧长度宏
// #define FLIGHTCTRL_FRAME_SIZE sizeof(FlightCtrlToDataLink)

// // 初始化飞控数据结构
// void initFlightCtrlFrame(FlightCtrlToDataLink *frame) {
//     memset(frame, 0, sizeof(FlightCtrlToDataLink));
//     frame->frameHeader = 0xAA55;
//     frame->dataLength = FLIGHTCTRL_FRAME_SIZE;
// }

// 计算校验和函数 飞控原版函数
//uint8_t calculateChecksum(const void* data, size_t len) {
//    const uint8_t *bytes = (const uint8_t*)data;
//    uint8_t sum = 0;
//    for(size_t i = 0; i < len; i++) {
//        sum += bytes[i];
//    }
//    return sum;
//}

// 握手数据包结构
struct UdpHandshakePacket
{
    char magic[4];      // 握手标识 "GCS\0"
    unsigned int version;     // 协议版本（使用unsigned int替代UINT32）
};
