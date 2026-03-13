#include "UdpTestReceiver.h"
#include "UdpConfig.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

// 这里在测试工程本地重新定义两个“地面站发送”结构体，
// 定义与原工程 UdpData.h 中 112~185 行保持一致。

#pragma pack(push, 1)

// 地面站发送数据包结构    地面站帧头作为隐形msg_id使用
struct UdpSendDataPacket_Cmd {
    uint16_t frameHeader;        // 固定值0xFF00
    // 任务指令
    uint8_t missionCommand_B0;   // 置1，为发射流程指令  置0，为地面测试流程指令
    uint8_t missionCommand_B1;   // 置1，激活           置0，未激活   自检指令
    uint8_t missionCommand_B2;   // 置1，激活           置0，未激活   参数装订指令
    uint8_t missionCommand_B3;   // 置1，激活           置0，未激活   舵面检查指令
    uint8_t missionCommand_B4;   // 置1，激活           置0，未激活   发动机检查指令
    uint8_t missionCommand_B5;   // 置1，激活           置0，未激活   发射指令
    uint8_t controlMode_B0;      // 置2，全自主 置1，半自主 置0，手动遥控 控制模式
    uint8_t checksum;            // 校验和
};

struct Waypoint  // 航路点结构体定义
{
    int32_t longitude;  // 经度  x1E7
    int32_t latitude;   // 纬度  x1E7
    int16_t altitude;   // 高度  x10
};

struct UdpSendDataPacket_Data {
    uint16_t frameHeader;        // 固定值0xF00F
    // 本结构体数据均于page2使用
    int32_t launchLongitude;     // 发射点 经度              deg x10000000
    int32_t launchLatitude;      // 发射点 纬度              deg x10000000
    int16_t launchAltitude;      // 发射点 高度              m x10

    int16_t initPitch;           // 初始姿态角 俯仰角        deg x10
    int16_t initYaw;             // 初始姿态角 航向角        deg x10
    int16_t initRoll;            // 初始姿态角 滚转角        deg x10

    int16_t initPitchRate;       // 初始角速率 俯仰角速率    deg/s x10
    int16_t initYawRate;         // 初始角速率 航向角速率    deg/s x10
    int16_t initRollRate;        // 初始角速率 滚转角速率    deg/s x10

    int16_t initNorthVelocity;   // 初始速度 北向速度        m/s x10
    int16_t initEastVelocity;    // 初始速度 东向速度        m/s x10
    int16_t initVerticalVelocity;// 初始速度 天向速度        m/s x10

    int16_t initNorthAccel;      // 初始加速度 北向加速度    m/s^2 x10
    int16_t initEastAccel;       // 初始加速度 东向加速度    m/s^2 x10
    int16_t initVerticalAccel;   // 初始加速度 天向加速度    m/s^2 x10

    // 航路点数组
    Waypoint waypoints[100];     // 100个航路点

    int32_t targetLongitude;     // 目标点 经度              deg x10000000
    int32_t targetLatitude;      // 目标点 纬度              deg x10000000
    int16_t targetAltitude;      // 目标点 高度              m x10

    int32_t launchLongitude2;    // 发射点(重复?) 经度       deg x10000000
    int32_t launchLatitude2;     // 发射点(重复?) 纬度       deg x10000000
    int16_t launchAltitude2;     // 发射点(重复?) 高度       m x10

    int32_t parachuteLongitude;  // 开伞点 经度              deg x10000000
    int32_t parachuteLatitude;   // 开伞点 纬度              deg x10000000
    int16_t parachuteAltitude;   // 开伞点 高度              m x10

    int8_t  elevatorCmd;         // 控制指令 俯仰舵偏指令    / x1
    int8_t  aileronCmd;          // 控制指令 滚转舵偏指令    / x1
    uint8_t airspeedSet;         // 控制指令 空速设定值      m/s x10

    uint8_t checksum;            // 校验和
};

#pragma pack(pop)

// 打印两个包的内容（只挑关键字段）
static void PrintCmdPacket(const UdpSendDataPacket_Cmd& pkt)
{
    std::cout << "===== UdpSendDataPacket_Cmd (frameHeader=0x"
        << std::hex << std::uppercase << pkt.frameHeader << std::dec << ") =====\n";
    std::cout << "missionCommand_B0 (发射/地面测试): " << static_cast<int>(pkt.missionCommand_B0) << "\n";
    std::cout << "missionCommand_B1 (自检指令):       " << static_cast<int>(pkt.missionCommand_B1) << "\n";
    std::cout << "missionCommand_B2 (参数装订):       " << static_cast<int>(pkt.missionCommand_B2) << "\n";
    std::cout << "missionCommand_B3 (舵面检查):       " << static_cast<int>(pkt.missionCommand_B3) << "\n";
    std::cout << "missionCommand_B4 (发动机检查):     " << static_cast<int>(pkt.missionCommand_B4) << "\n";
    std::cout << "missionCommand_B5 (发射指令):       " << static_cast<int>(pkt.missionCommand_B5) << "\n";
    std::cout << "controlMode_B0   (控制模式):        " << static_cast<int>(pkt.controlMode_B0) << "\n";
    std::cout << "checksum:                          0x"
        << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(pkt.checksum) << std::dec << "\n";
    std::cout << "============================================================\n\n";
}

static void PrintDataPacket(const UdpSendDataPacket_Data& pkt)
{
    std::cout << "===== UdpSendDataPacket_Data (frameHeader=0x"
        << std::hex << std::uppercase << pkt.frameHeader << std::dec << ") =====\n";

    std::cout << "launchLongitude (发射点经度, deg*1e7): " << pkt.launchLongitude << "\n";
    std::cout << "launchLatitude  (发射点纬度, deg*1e7): " << pkt.launchLatitude << "\n";
    std::cout << "launchAltitude  (发射点高度, m*10):    " << pkt.launchAltitude << "\n";

    std::cout << "initPitch (俯仰, deg*10):             " << pkt.initPitch << "\n";
    std::cout << "initYaw   (航向,  deg*10):            " << pkt.initYaw << "\n";
    std::cout << "initRoll  (滚转,  deg*10):            " << pkt.initRoll << "\n";

    std::cout << "initPitchRate (俯仰角速率, deg/s*10): " << pkt.initPitchRate << "\n";
    std::cout << "initYawRate   (航向角速率, deg/s*10): " << pkt.initYawRate << "\n";
    std::cout << "initRollRate  (滚转角速率, deg/s*10): " << pkt.initRollRate << "\n";

    std::cout << "initNorthVelocity   (北向速度, m/s*10): " << pkt.initNorthVelocity << "\n";
    std::cout << "initEastVelocity    (东向速度, m/s*10): " << pkt.initEastVelocity << "\n";
    std::cout << "initVerticalVelocity(天向速度, m/s*10): " << pkt.initVerticalVelocity << "\n";

    std::cout << "initNorthAccel   (北向加速度, m/s^2*10): " << pkt.initNorthAccel << "\n";
    std::cout << "initEastAccel    (东向加速度, m/s^2*10): " << pkt.initEastAccel << "\n";
    std::cout << "initVerticalAccel(天向加速度, m/s^2*10): " << pkt.initVerticalAccel << "\n";

    std::cout << "targetLongitude (目标经度, deg*1e7): " << pkt.targetLongitude << "\n";
    std::cout << "targetLatitude  (目标纬度, deg*1e7): " << pkt.targetLatitude << "\n";
    std::cout << "targetAltitude  (目标高度, m*10):    " << pkt.targetAltitude << "\n";

    std::cout << "launchLongitude2 (发射点2经度, deg*1e7): " << pkt.launchLongitude2 << "\n";
    std::cout << "launchLatitude2  (发射点2纬度, deg*1e7): " << pkt.launchLatitude2 << "\n";
    std::cout << "launchAltitude2  (发射点2高度, m*10):    " << pkt.launchAltitude2 << "\n";

    std::cout << "parachuteLongitude (开伞点经度, deg*1e7): " << pkt.parachuteLongitude << "\n";
    std::cout << "parachuteLatitude  (开伞点纬度, deg*1e7): " << pkt.parachuteLatitude << "\n";
    std::cout << "parachuteAltitude  (开伞点高度, m*10):    " << pkt.parachuteAltitude << "\n";

    std::cout << "elevatorCmd (俯仰舵指令): " << static_cast<int>(pkt.elevatorCmd) << "\n";
    std::cout << "aileronCmd  (滚转舵指令): " << static_cast<int>(pkt.aileronCmd) << "\n";
    std::cout << "airspeedSet (空速设定, m/s*10): "
        << static_cast<int>(pkt.airspeedSet) << "\n";

    std::cout << "checksum: 0x"
        << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(pkt.checksum) << std::dec << "\n";

    int nonZeroWpCount = 0;
    for (int i = 0; i < 100; ++i) {
        if (pkt.waypoints[i].longitude != 0 ||
            pkt.waypoints[i].latitude != 0 ||
            pkt.waypoints[i].altitude != 0) {
            ++nonZeroWpCount;
        }
    }
    std::cout << "waypoints: 非零航路点数量 = " << nonZeroWpCount << " / 100\n";
    std::cout << "============================================================\n\n";
}

void RunUdpTestReceiver(const char* bindIp, unsigned short port)
{
    WSADATA wsaData{};
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0) {
        std::cerr << "WSAStartup 失败，错误码: " << ret << std::endl;
        return;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket 创建失败，错误码: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(port);

    if (bindIp && std::string(bindIp).length() > 0) {
        if (inet_pton(AF_INET, bindIp, &localAddr.sin_addr) != 1) {
            std::cerr << "inet_pton 解析绑定地址失败: " << bindIp << std::endl;
            closesocket(sock);
            WSACleanup();
            return;
        }
    }
    else {
        localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(sock, reinterpret_cast<sockaddr*>(&localAddr), sizeof(localAddr)) == SOCKET_ERROR) {
        std::cerr << "bind 失败，错误码: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return;
    }

    char ipStr[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &localAddr.sin_addr, ipStr, sizeof(ipStr));
    std::cout << "UDP_Test 接收器已启动，绑定到 "
        << ipStr << ":" << port << std::endl;
    std::cout << "建议在 main.cpp 中调用：RunUdpTestReceiver(\"0.0.0.0\", UDP_REMOTE_PORT="
        << UDP_REMOTE_PORT << ")\n";
    std::cout << "等待地面站发送指令 / 装订参数数据包...\n\n";

    while (true) {
        char buffer[2048];
        sockaddr_in fromAddr{};
        int fromLen = sizeof(fromAddr);

        int nRecv = recvfrom(sock,
            buffer,
            static_cast<int>(sizeof(buffer)),
            0,
            reinterpret_cast<sockaddr*>(&fromAddr),
            &fromLen);
        if (nRecv == SOCKET_ERROR) {
            int err = WSAGetLastError();
            std::cerr << "recvfrom 失败，错误码: " << err << std::endl;
            break;
        }
        if (nRecv <= 0) {
            continue;
        }

        char fromIp[INET_ADDRSTRLEN] = { 0 };
        inet_ntop(AF_INET, &fromAddr.sin_addr, fromIp, sizeof(fromIp));
        std::cout << "收到 UDP 数据包，大小 = " << nRecv
            << " 字节，来自 " << fromIp << ":"
            << ntohs(fromAddr.sin_port) << "\n";

        if (nRecv < static_cast<int>(sizeof(uint16_t))) {
            std::cout << "数据太短，无法解析帧头，忽略。\n\n";
            continue;
        }

        const uint16_t* pFrameHeader = reinterpret_cast<const uint16_t*>(buffer);
        uint16_t frameHeader = *pFrameHeader;

        if (frameHeader == 0xFF00 &&
            nRecv >= static_cast<int>(sizeof(UdpSendDataPacket_Cmd))) {
            const UdpSendDataPacket_Cmd* pCmd =
                reinterpret_cast<const UdpSendDataPacket_Cmd*>(buffer);
            PrintCmdPacket(*pCmd);
        }
        else if (frameHeader == 0xF00F &&
            nRecv >= static_cast<int>(sizeof(UdpSendDataPacket_Data))) {
            const UdpSendDataPacket_Data* pData =
                reinterpret_cast<const UdpSendDataPacket_Data*>(buffer);
            PrintDataPacket(*pData);
        }
        else {
            std::cout << "未知或长度不匹配的数据包：frameHeader=0x"
                << std::hex << std::uppercase << frameHeader
                << std::dec << "，长度=" << nRecv << "，忽略。\n\n";
        }
    }

    closesocket(sock);
    WSACleanup();
}