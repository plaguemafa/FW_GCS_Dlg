#pragma once
#ifndef FW_GCS_UAV_DATALINK_H
#define FW_GCS_UAV_DATALINK_H

#include <stdint.h>
// #include <string.h>

#pragma pack(push, 1)             // 紧密打包，避免字节对齐填充
typedef struct {
    uint16_t  frameHeader;         // 帧头  固定值0xAA55
    uint8_t   Protocol_version;    // 当前协议版本，V3 = 3  

    uint8_t   aircraftID;          // 飞机编号  
    uint16_t  Heartbeat_count;     // 心跳计数，飞控端定时器计数  s x1         地面站解包到65535时主动断开数据链（18.2h）

    int16_t  pitchAngle;           // 俯仰角                 deg x10         Canvas组1 协议(1)
    int16_t  rollAngle;            // 滚转角                 deg x10         Canvas组1 协议(2)
    int16_t  yawAngle;             // 航向角                 deg x10         Canvas组1 协议(3)

    int16_t  pitchRate;            // 俯仰角速率           deg/s x10
    int16_t  rollRate;             // 滚转角速率           deg/s x10
    int16_t  yawRate;              // 航向角速率           deg/s x10
    int16_t  pitchAcceleration;    // 俯仰角加速度       deg/s^2 x10
    int16_t  rollAcceleration;     // 滚转角加速度       deg/s^2 x10
    int16_t  yawAcceleration;      // 航向角加速度       deg/s^2 x10

    int16_t  attackAngle;          // 攻角                   deg x10         Canvas组1 协议(5)
    int16_t  sideslipAngle;        // 侧滑角                 deg x10         Canvas组1 协议(6)

    int16_t  normalOverload;       // 法向过载                 / x10         Canvas组1 协议(4-1)
    int16_t  longitudinalOverload; // 纵向过载                 / x10        Canvas组1 协议(4-2)
    int16_t  lateralOverload;      // 横向过载                 / x10        Canvas组1 协议(4-3)

    int8_t   rudderCmd1;           // 1#舵偏指令               / x1
    int8_t   rudderCmd2;           // 2#舵偏指令               / x1
    int8_t   rudderCmd3;           // 3#舵偏指令               / x1
    int8_t   rudderCmd4;           // 4#舵偏指令               / x1
    uint8_t  throttle;             // 油门控制           [1,100] x1         Canvas组2 协议(1-1)
    uint8_t  controlCommand_D0;    // 开伞指令标志             / 0xAA
    uint8_t  controlCommand_D1;    // 开舱指令标志             / 0xAA
    uint8_t  controlCommand_D2;    // 起落架指令标志           / 0xAA
    int8_t   turnRudderCmd;        // 转弯舵机指令             / x1

    int16_t  engineTemp;           // 发动机缸温               C x1         Canvas组2 协议(2-2)
    int16_t  engineRPM;            // 发动机转速             rpm x1         Canvas组1 协议(7)
    uint8_t  fuelRemaining;        // 剩余油量                 L x1         Canvas组2 协议(2-1)

    int16_t  gpsAltitude;          // 卫星高度                 m x10            绘图层 地图飞机标识高度1
    uint8_t  gpsStatus;            // 卫星定位状态             / 待定         Canvas组2 协议(3-2)
    uint8_t  satelitesNum;         // 卫星收星数               / x1         Canvas组2 协议(3-1)
    int16_t  gpsCourse;            // 卫星地速航向           deg x10            绘图层 地图飞机标识方向1
    int16_t  gpsGroundSpeed;       // 卫星地速               m/s x10            绘图层 主界面底部信息栏 1-1
    uint8_t  gpsVerticalSpeed;     // 卫星垂直速度           m/s x10            绘图层 主界面底部信息栏 1-2
    int32_t  longitude;            // 卫星经度                度 x10000000      绘图层 地图飞机标识位置1
    int32_t  latitude;             // 卫星纬度                度 x10000000      绘图层 地图飞机标识位置2

    int16_t  eastVelocity;         // 东向速度               m/s x10
    int16_t  northVelocity;        // 北向速度               m/s x10
    int16_t  verticalVelocity;     // 天向速度               m/s x10

    int8_t   airTemperature;       // 大气温度                 C x1
    int16_t  baroAltitude;         // 气压高度                 m x10         Canvas组1 协议(8)
    int16_t  baroAirspeed;         // 气压空速               m/s x10
    int16_t  indicatedAirspeed;    // 表速                   m/s x10         Canvas组1 协议(9)
    uint8_t  machNumber;           // 马赫数                   / x100         Canvas组1 协议(10)
    uint16_t radioAltitude;        // 无线电高度               / x10          Canvas组1 协议(11)

    uint8_t  navStatus;            // 导航状态                 / 待定         Canvas组2 协议(4-1)
    uint8_t  gpsHour;              // GPS时                    h x1         绘图层 主界面底部信息栏 1-3.1
    uint8_t  gpsMinute;            // GPS分                  min x1         绘图层 主界面底部信息栏 1-3.2
    uint8_t  gpsSecond;            // GPS秒                    s x1         绘图层 主界面底部信息栏 1-3.3
    uint8_t  routeNumber;          // 当前航线号                / x1         绘图层 主界面底部信息栏 1-3.4
    uint8_t  targetWaypoint;       // 目标航点                  / x1         Canvas组2 协议(4-2)
    int16_t  crossTrackError;      // 偏航距                    m x10        Canvas组2 协议(5-2)
    int16_t  courseDeviation;      // 偏航角                  deg x10        Canvas组2 协议(6-2)
    int16_t  distanceToGo;         // 待飞距                    m x10        Canvas组2 协议(5-1)
    uint8_t  commandHeading;       // 应飞航向                deg x10        Canvas组2 协议(6-1)
    uint16_t commandSpeed;         // 应飞速度                m/s x10        Canvas组2 协议(7-1)
    uint16_t commandAltitude;      // 应飞高度                  m x10        Canvas组2 协议(7-2)
    uint8_t  commandTime;          // 应飞时间                  s x1         Canvas组2 协议(8-1)
    uint8_t  payloadType;          // 载荷类型                  / x1         Canvas组2 协议(8-2)
    uint8_t  ammoRemaining;        // 剩余弹量                  / x1         Canvas组2 协议(9-1)
    uint8_t  selfTestResult;       // 自检结果                  / 0xAA       Canvas组2 协议(9-2)

    uint8_t  YIS100A_result;       // IMU自检结果               / 0xAA         视窗组9-0 IDC_Display66 扩展协议 pag1
    uint8_t  HP5804_result;        // 气压计自检结果             / 0xAA         视窗组9-1 IDC_Display67 扩展协议 pag1
    uint8_t  MS4525D_result;       // 空速计自检结果             / 0xAA         视窗组9-2 IDC_Display68 扩展协议 pag1
    uint8_t  M401_result;          // 温度计自检结果             / 0xAA         视窗组9-3 IDC_Display69 扩展协议 pag1
    uint8_t  GPS_result;           // GPS自检结果               / 0xAA          视窗组9-4 IDC_Display70 扩展协议 pag1
    uint8_t  PAC1931_result1;      // 电压自检结果              / 0xAA          视窗组9-5 IDC_Display71 扩展协议 pag1
    uint8_t  can_to_pw_result;     // PWM自检结果               / 0xAA          视窗组9-6 IDC_Display72 扩展协议 pag1
    uint8_t  SBUS_result;          // SBUS自检结果              / 0xAA          视窗组9-7 IDC_Display73 扩展协议 pag1

    uint8_t  batteryVoltage;       // 电池电压                  / x1         Canvas组2 协议(1-2)
    uint8_t  workflowStatus_D0;    // 工作流程标志               // 0xAA为发射控制流程     0x00为地面测试流程         
    uint8_t  workflowStatus_D1;    // 发射状态标志               // 0xAA为发射后          0x00为发射前         

    uint8_t  alarmStatus_D0;       // 电池电压低报警标志          / 0xAA         顶层图层中央横幅报警1
    uint8_t  alarmStatus_D1;       // 高度报警标志               / 0xAA         顶层图层中央横幅报警2
    uint8_t  alarmStatus_D2;       // 油量低报警标志             / 0xAA          顶层图层中央横幅报警3
    uint8_t  alarmStatus_D3;       // 转速异常报警标志           / 0xAA          顶层图层中央横幅报警4
    uint8_t  alarmStatus_D4;       // 空速异常报警标志           / 0xAA          顶层图层中央横幅报警5
    uint8_t  alarmStatus_D5;       // GPS定位精度低报警标志      / 0xAA          顶层图层中央横幅报警6

    uint8_t  switchStatus_D0;      // 发动机并网状态             / 0xAA          Canvas组3 协议(3-1)
    uint8_t  switchStatus_D1;      // 发动机启动状态             / 0xAA          Canvas组3 协议(1-1)
    uint8_t  switchStatus_D2;      // 盘旋状态                  / 0xAA          Canvas组3 协议(2-1)      
    uint8_t  switchStatus_D3;      // 归航状态                  / 0xAA          Canvas组3 协议(2-2)
    uint8_t  switchStatus_D4;      // 关车状态                  / 0xAA          Canvas组3 协议(1-2)
    uint8_t  switchStatus_D5;      // 起落架收放状态             / 0xAA          Canvas组3 协议(4-1)
    uint8_t  switchStatus_D6;      // 开伞状态                  / 0xAA          Canvas组3 协议(3-2)
    uint8_t  switchStatus_D7;      // 夜航灯开关状态             / 0xAA          Canvas组3 协议(4-2)

    int32_t  targetLongitude;     // 目标经度               deg x10000000       绘图层 地图目标标识位置1
    int32_t  targetLatitude;      // 目标纬度               deg x10000000       绘图层 地图目标标识位置2
    int16_t  targetAltitude;      // 目标高度                 m x10             绘图层 地图目标标识位置3
    int8_t   targetSpeed;         // 目标速度               m/s x10             绘图层 地图目标标识速度
    int16_t  targetCourse;        // 目标航向               deg x10             绘图层 地图目标标识方向

    uint16_t  FCS_Report_Flag;         // 飞控状态回报字段
        //正常工作状态： 0x0001 上电自检状态 0x0002 自检  0x0003 准备阶段（等待任务设置参数，完成自检） 
        //             0xAA01 进入发控流程，等待任务参数      0xAA02 等待参数装订指令     0xAA03 等待发射指令(准备好)    0xAA04 离架飞行状态    0xAA05 开伞状态
        //             0xBB01 进入地测流程，等待测试指令      0xCC02 舵面测试状态         0xCC03 发动机测试状态   
        //异常工作状态： 0xEE01 自检失败终止状态，等待下电

    uint8_t   DataLink_CmdResult;      // 数据链指令接收结果         // 0xAA 确认接收解析 3s内        0x00为无回报
    uint8_t   DataLink_DataResult;     // 数据链数据接收结果         // 0xAA 确认接收解析 3s内        0x00为无回报

    // int32_t  reserved1;           // 预留1             
    // int32_t  reserved2;           // 预留2
    // int32_t  reserved3;           // 预留3
    // int32_t  reserved4;           // 预留4

    uint8_t checksum;                //校验和 地面站不涉及安全性问题，暂不做接收弃用包处理

}DataLinkRecvDataPacket_s;
#pragma pack(pop)  // 恢复默认字节对齐

// 地面站发送数据包结构（需要1字节对齐，避免结构体填充）
#pragma pack(push, 1)  // 紧密打包，避免字节对齐填充
struct CmdSendPacket_s{                   // 地面站发送数据包结构    地面站帧头作为隐形msg_id使用
    uint16_t frameHeader;                       // 固定值0xAAAA
    // 任务指令  
    uint8_t missionCommand_D0;         // 0xAA 为发射流程指令     0x00 为地面测试流程指令
    uint8_t missionCommand_D1;         // 0xAA 为激活            0x00 为未激活                         自检指令
    uint8_t missionCommand_D2;         // 0xAA 为激活            0x00 为未激活                         任务参数装订指令
    uint8_t missionCommand_D3;         // 0xAA 为激活            0x00 为未激活                         舵面检查指令
    uint8_t missionCommand_D4;         // 0xAA 为激活            0x00 为未激活                         发动机检查指令
    uint8_t missionCommand_D5;         // 0xAA 为激活            0x00 为未激活                         发射指令
    uint8_t missionCommand_D6;         // 0xAA 为全自主       0xFF 为半自主        0x00 为手动遥控       控制模式  
    uint8_t missionCommand_D7;         // 0xAA 为激活            0x00 为未激活                         IMU精度检查指令
    uint8_t missionCommand_D8;         // 0xAA 为激活            0x00 为未激活                         卫星收星检查指令
    uint8_t missionCommand_D9;         // 0xAA 为激活            0x00 为未激活                         卫星定位精度检查指令
    int8_t elevatorCmd;                // 控制指令 俯仰舵偏指令        [+—30]     
    int8_t aileronCmd;                 // 控制指令 滚转舵偏指令        [+—30]
    uint8_t checksum;                  // 校验和 
};
#pragma pack(pop)  // 恢复默认字节对齐

struct Waypoint  // 航路点结构体定义
{
    int32_t longitude;  // 经度  x1E7
    int32_t latitude;   // 纬度  x1E7
    int16_t altitude;   // 高度  x10
};

#pragma pack(push, 1)  // 紧密打包，避免字节对齐填充
struct DataSendPacket_s{
    uint16_t frameHeader;                       // 固定值0xF00F
    //本结构体数据均于page2使用
    int32_t launchLongitude;                    // 发射点 经度              deg x10000000    IDC_Display_EditData2
    int32_t launchLatitude;                     // 发射点 纬度              deg x10000000    IDC_Display_EditData3
    int16_t launchAltitude;                     // 发射点 高度                m x10         IDC_Display_EditData4
       
    int16_t initPitch;                          // 初始姿态角 俯仰角          deg x10        IDC_Display_EditData5
    int16_t initYaw;                            // 初始姿态角 航向角          deg x10        IDC_Display_EditData6
    int16_t initRoll;                           // 初始姿态角 滚转角          deg x10        IDC_Display_EditData7
     
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
    
    int32_t targetLongitude;                     // 目标点 经度                 deg x10000000    IDC_Display_EditData17
    int32_t targetLatitude;                      // 目标点 纬度                 deg x10000000    IDC_Display_EditData18
    int16_t targetAltitude;                      // 目标点 高度                   m x10          IDC_Display_EditData19

    int32_t parachuteLongitude;                  // 开伞点 经度               deg x10000000    IDC_Display_EditData23
    int32_t parachuteLatitude;                   // 开伞点 纬度                deg x10000000    IDC_Display_EditData24
    int16_t parachuteAltitude;                   // 开伞点 高度                  m x10          IDC_Display_EditData25
    
    uint8_t airspeedSet;                         // 控制指令 空速设定值        m/s x10         IDC_Display_EditData28

    uint8_t checksum;                            // 校验和 
};
#pragma pack(pop)  // 恢复默认字节对齐

// 握手数据包结构
struct UdpHandshakePacket
{
    char magic[4];      // 握手标识 "GCS\0"
    unsigned int version;     // 协议版本（使用unsigned int替代UINT32）
};

#endif // FW_GCS_UAV_DATALINK_H
