#include "UdpTestReceiver.h"
#include "UdpConfig.h"  // 为了用 UDP_REMOTE_PORT 宏

int main()
{
    // 建议用 0.0.0.0 监听所有网卡，端口用地面站当前的 UDP_REMOTE_PORT

    //RunUdpTestReceiver("0.0.0.0", UDP_REMOTE_PORT);
    RunUdpTestReceiver(UDP_LOCAL_IP, UDP_REMOTE_PORT);

    return 0;
}