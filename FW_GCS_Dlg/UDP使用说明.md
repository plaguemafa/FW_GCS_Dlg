# UDP通信功能使用说明

## 一、功能概述

已实现完整的UDP通信功能，包括：
- ✅ UDP连接/断开
- ✅ 握手协议
- ✅ 持续数据包接收（后台线程）
- ✅ 数据包处理

## 二、文件结构

### 1. 协议数据结构文件
**`UdpDataProtocol.h`** - 定义通信协议
```cpp
struct UdpDataPacket
{
    float data1;  // 数据1
    float data2;  // 数据2
    float data3;  // 数据3
    float data4;  // 数据4
    float data5;  // 数据5
};
```

**后续可以在此文件中修改协议结构，添加地面站协议字段。**

### 2. 主要实现文件
- **`FW_GCS_DlgDlg.h`** - UDP相关成员变量和函数声明
- **`FW_GCS_DlgDlg.cpp`** - UDP通信功能实现

## 三、配置参数

UDP连接参数写死在代码中（`FW_GCS_DlgDlg.cpp` 第17-20行）：

```cpp
#define UDP_REMOTE_IP      "192.168.1.100"  // 远程IP地址
#define UDP_REMOTE_PORT    8888             // 远程端口
#define UDP_LOCAL_PORT     8889             // 本地端口
```

**修改方法：** 直接修改这些宏定义即可。

## 四、使用流程

### 1. 连接UDP
1. 点击 **"UDP连接"** 按钮（IDC_UDPlink）
2. 系统自动执行：
   - 创建UDP Socket
   - 绑定本地端口
   - 发送握手数据包
   - 启动接收线程
3. 显示连接成功消息框

### 2. 持续接收数据
- 接收线程在后台持续运行
- 收到数据包后自动调用 `ProcessReceivedData()` 处理
- 数据通过消息机制传递到主线程，确保线程安全

### 3. 断开UDP
1. 再次点击 **"UDP连接"** 按钮
2. 系统自动执行：
   - 停止接收线程
   - 关闭Socket
   - 清理资源
3. 显示断开消息框

## 五、关键函数说明

### 1. `ConnectUdp()` - 连接UDP
- 初始化Socket
- 设置远程地址
- 发送握手数据包
- 启动接收线程

### 2. `DisconnectUdp()` - 断开UDP
- 停止接收线程
- 关闭Socket
- 清理资源

### 3. `SendHandshake()` - 发送握手
- 发送握手数据包（"GCS" + 版本号）

### 4. `UdpRecvThread()` - 接收线程
- 后台持续运行
- 阻塞等待数据（`recvfrom()`）
- 收到数据后通过消息通知主线程

### 5. `ProcessReceivedData()` - 处理数据
- 在主线程中执行（线程安全）
- 当前使用TRACE输出到调试窗口
- **可以修改此函数，更新UI控件显示数据**

## 六、修改协议结构

### 步骤1：修改 `UdpDataProtocol.h`
```cpp
struct UdpDataPacket
{
    // 添加你的地面站协议字段
    float altitude;      // 高度
    float latitude;      // 纬度
    float longitude;     // 经度
    UINT16 status;       // 状态
    // ... 其他字段
};
```

### 步骤2：修改 `ProcessReceivedData()` 函数
在 `FW_GCS_DlgDlg.cpp` 中修改数据处理逻辑：
```cpp
void CFWGCSDlgDlg::ProcessReceivedData(const UdpDataPacket* pPacket)
{
    // 处理你的协议数据
    // 更新UI控件
    CString strDisplay;
    strDisplay.Format(_T("高度: %.2f\n纬度: %.2f\n经度: %.2f"), 
        pPacket->altitude, pPacket->latitude, pPacket->longitude);
    
    // 更新到显示控件
    GetDlgItem(IDC_EDIT_DISPLAY)->SetWindowText(strDisplay);
}
```

## 七、发送数据示例

如果需要发送数据，可以调用 `SendUdpData()`：

```cpp
// 示例：发送数据包
UdpDataPacket packet;
packet.data1 = 1.0f;
packet.data2 = 2.0f;
packet.data3 = 3.0f;
packet.data4 = 4.0f;
packet.data5 = 5.0f;

SendUdpData(&packet, sizeof(UdpDataPacket));
```

## 八、注意事项

1. **线程安全**
   - 接收线程中不能直接操作UI控件
   - 使用 `PostMessage()` 传递数据到主线程
   - 数据处理在 `ProcessReceivedData()` 中进行

2. **资源管理**
   - Socket在 `OnDestroy()` 中自动清理
   - 线程在断开时自动退出
   - Winsock库在程序退出时清理

3. **错误处理**
   - Socket错误会自动检测
   - 连接失败会显示错误消息
   - 接收线程错误会自动退出

4. **数据包大小**
   - 当前接收缓冲区1024字节
   - 数据包必须严格等于 `sizeof(UdpDataPacket)` 才会被处理
   - 如果协议大小变化，需要修改接收逻辑

## 九、调试方法

1. **查看接收数据**
   - 在Visual Studio的输出窗口查看TRACE输出
   - 或修改 `ProcessReceivedData()` 添加消息框显示

2. **测试连接**
   - 使用网络调试工具（如UDP测试工具）发送数据
   - 确保IP和端口配置正确

3. **检查线程状态**
   - 连接后线程自动启动
   - 断开时线程自动退出

## 十、后续扩展

1. **添加串口通信**
   - 参考UDP实现方式
   - 可以同时运行串口和UDP通信

2. **添加UI显示控件**
   - 在对话框中添加Edit Control或ListBox
   - 在 `ProcessReceivedData()` 中更新显示

3. **添加数据记录**
   - 在 `ProcessReceivedData()` 中添加文件写入
   - 记录接收到的数据包

4. **添加错误重连**
   - 检测连接断开
   - 自动重连机制



