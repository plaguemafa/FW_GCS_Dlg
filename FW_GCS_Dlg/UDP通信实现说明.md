# UDP通信实现方案与架构设计

## 一、整体架构思路

### 1. 双通道通信架构
```
┌─────────────────────────────────────┐
│      CFWGCSDlgDlg (对话框类)        │
│                                     │
│  ┌──────────────┐  ┌──────────────┐ │
│  │  串口通信模块 │  │  UDP通信模块 │ │
│  │              │  │              │ │
│  │ - 打开/关闭  │  │ - 连接/断开  │ │
│  │ - 发送/接收  │  │ - 发送/接收  │ │
│  │ - 工作线程   │  │ - 工作线程   │ │
│  └──────────────┘  └──────────────┘ │
│                                     │
│  ┌──────────────────────────────┐   │
│  │      UI控件更新              │   │
│  │  - 按钮状态                  │   │
│  │  - 数据显示                   │   │
│  │  - 状态指示                   │   │
│  └──────────────────────────────┘   │
└─────────────────────────────────────┘
```

### 2. 代码组织原则
- **所有通信相关代码都在 `FW_GCS_DlgDlg` 类中**
- **串口和UDP独立管理，互不干扰**
- **使用工作线程处理数据接收，避免阻塞UI**
- **统一的状态管理机制**

---

## 二、UDP通信实现方案

### 方案选择：Windows Socket API (Winsock)

**推荐使用 Windows Socket API**，原因：
- MFC原生支持，无需第三方库
- 功能完整，性能好
- 与串口通信代码风格统一（都使用Windows API）

---

## 三、代码位置规划

### 1. 头文件声明位置：`FW_GCS_DlgDlg.h`

**需要添加的内容：**

```cpp
class CFWGCSDlgDlg : public CDialogEx
{
    // ... 现有代码 ...
    
    // ========== UDP通信相关成员变量 ==========
    SOCKET m_udpSocket;              // UDP Socket句柄
    BOOL m_bUdpConnected;            // UDP连接状态标志
    CString m_strUdpRemoteIP;        // 远程IP地址（如 "192.168.1.100"）
    UINT m_nUdpRemotePort;           // 远程端口（如 8888）
    UINT m_nUdpLocalPort;            // 本地端口（如 8889）
    sockaddr_in m_udpRemoteAddr;     // 远程地址结构
    
    // UDP工作线程相关
    CWinThread* m_pUdpRecvThread;   // UDP接收线程指针
    BOOL m_bUdpThreadRunning;        // 线程运行标志
    
    // ========== UDP通信相关函数 ==========
    // UDP初始化与连接管理
    BOOL InitUdpSocket();            // 初始化UDP Socket
    BOOL ConnectUdp(CString strIP, UINT nRemotePort, UINT nLocalPort);  // 连接UDP
    void DisconnectUdp();            // 断开UDP连接
    
    // UDP数据收发
    BOOL SendUdpData(BYTE* pData, int nSize);  // 发送UDP数据
    void OnUdpDataReceived(BYTE* pData, int nSize);  // UDP数据接收处理回调
    
    // UDP工作线程
    static UINT UdpRecvThread(LPVOID pParam);  // UDP接收线程函数（静态）
    
    // UDP按钮事件处理
    afx_msg void OnBnClickedUdplink();          // UDP连接按钮（已有）
    afx_msg void OnBnClickedSeriallink();      // 串口连接按钮（待添加）
    
    // 自定义消息处理（用于线程到主线程通信）
    afx_msg LRESULT OnUdpDataReceivedMsg(WPARAM wParam, LPARAM lParam);
    
    DECLARE_MESSAGE_MAP()
};
```

### 2. 实现文件位置：`FW_GCS_DlgDlg.cpp`

**需要添加的内容：**

#### 2.1 消息映射（BEGIN_MESSAGE_MAP）
```cpp
BEGIN_MESSAGE_MAP(CFWGCSDlgDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_UDPlink, &CFWGCSDlgDlg::OnBnClickedUdplink)
    ON_BN_CLICKED(IDC_SERIALlink, &CFWGCSDlgDlg::OnBnClickedSeriallink)  // 串口按钮
    ON_MESSAGE(WM_USER + 200, OnUdpDataReceivedMsg)  // 自定义消息：UDP数据接收
    ON_WM_DESTROY()  // 对话框销毁时清理资源
END_MESSAGE_MAP()
```

#### 2.2 构造函数初始化
```cpp
CFWGCSDlgDlg::CFWGCSDlgDlg(CWnd* pParent)
    : CDialogEx(IDD_FW_GCS_DLG_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    
    // UDP初始化
    m_udpSocket = INVALID_SOCKET;
    m_bUdpConnected = FALSE;
    m_pUdpRecvThread = NULL;
    m_bUdpThreadRunning = FALSE;
    m_nUdpRemotePort = 0;
    m_nUdpLocalPort = 0;
}
```

#### 2.3 OnInitDialog() - 初始化Winsock库
```cpp
BOOL CFWGCSDlgDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    
    // ... 现有代码 ...
    
    // 初始化Winsock库（只需要初始化一次）
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    // 初始化UDP Socket（但不连接）
    InitUdpSocket();
    
    return TRUE;
}
```

#### 2.4 OnDestroy() - 清理资源
```cpp
void CFWGCSDlgDlg::OnDestroy()
{
    // 断开UDP连接
    DisconnectUdp();
    
    // 清理Winsock库
    WSACleanup();
    
    CDialogEx::OnDestroy();
}
```

#### 2.5 UDP按钮事件处理
```cpp
void CFWGCSDlgDlg::OnBnClickedUdplink()
{
    if (!m_bUdpConnected)
    {
        // 当前未连接，执行连接操作
        // 1. 从UI控件获取IP和端口（需要添加IP和端口输入控件）
        // 2. 调用 ConnectUdp() 连接
        // 3. 启动接收线程
        // 4. 更新按钮文本为"断开UDP"
        // 5. 更新状态显示
    }
    else
    {
        // 当前已连接，执行断开操作
        // 1. 调用 DisconnectUdp() 断开
        // 2. 停止接收线程
        // 3. 更新按钮文本为"连接UDP"
        // 4. 更新状态显示
    }
}
```

---

## 四、UDP持续通信实现思路

### 方案：工作线程 + 消息通知机制

```
┌─────────────────────────────────────────────┐
│  主线程（UI线程）                           │
│                                             │
│  OnBnClickedUdplink()                      │
│    ↓                                        │
│  ConnectUdp()                               │
│    ↓                                        │
│  创建UDP Socket                             │
│    ↓                                        │
│  启动接收线程 ←──────────────────────────┐ │
│    ↓                                      │ │
│  返回，UI继续响应                          │ │
└───────────────────────────────────────────┼─┘
                                             │
┌───────────────────────────────────────────┼─┐
│  UDP接收线程（后台工作线程）                │ │
│                                             │ │
│  UdpRecvThread()                            │ │
│    ↓                                        │ │
│  while(线程运行标志)                        │ │
│    ↓                                        │ │
│  recvfrom() 阻塞等待数据                    │ │
│    ↓                                        │ │
│  收到数据                                    │ │
│    ↓                                        │ │
│  PostMessage(WM_USER+200, ...) 通知主线程  │ │
│    ↓                                        │ │
│  继续循环等待                                │ │
└─────────────────────────────────────────────┘
                                             │
┌─────────────────────────────────────────────┐
│  主线程收到消息                              │
│                                             │
│  OnUdpDataReceivedMsg()                     │
│    ↓                                        │
│  解析数据                                    │
│    ↓                                        │
│  更新UI显示                                  │
│  处理业务逻辑                                │
└─────────────────────────────────────────────┘
```

### 关键点：

1. **接收线程是阻塞的**
   - 使用 `recvfrom()` 阻塞等待数据
   - 有数据到达时自动唤醒
   - 效率高，不占用CPU

2. **线程安全通信**
   - 使用 `PostMessage()` 从工作线程通知主线程
   - 避免直接操作UI控件（线程不安全）
   - 自定义消息 WM_USER + 200

3. **线程生命周期管理**
   - 连接时创建线程
   - 断开时设置标志，线程自动退出
   - 等待线程结束后再清理Socket

---

## 五、UDP握手与持续通信流程

### 连接流程：
```
1. 用户点击"连接UDP"按钮
   ↓
2. OnBnClickedUdplink() 被调用
   ↓
3. 从UI获取目标IP和端口（如：192.168.1.100:8888）
   ↓
4. 调用 ConnectUdp(strIP, nRemotePort, nLocalPort)
   ↓
5. 创建UDP Socket
   ↓
6. 绑定本地端口（可选，如果需要固定本地端口）
   ↓
7. 设置远程地址结构（用于后续发送）
   ↓
8. 发送握手数据包（可选，根据协议要求）
   ↓
9. 启动接收线程 UdpRecvThread()
   ↓
10. 更新UI：按钮文本改为"断开UDP"，显示"已连接"
```

### 持续通信流程：
```
【发送数据】
1. 业务逻辑需要发送数据
   ↓
2. 调用 SendUdpData(pData, nSize)
   ↓
3. 使用 sendto() 发送到远程地址
   ↓
4. 返回发送结果

【接收数据】
1. UDP接收线程 recvfrom() 阻塞等待
   ↓
2. 收到数据包
   ↓
3. PostMessage(WM_USER+200, size, buffer) 通知主线程
   ↓
4. 主线程 OnUdpDataReceivedMsg() 处理
   ↓
5. 解析数据、更新UI、处理业务逻辑
   ↓
6. 线程继续循环，等待下一个数据包
```

### 断开流程：
```
1. 用户点击"断开UDP"按钮
   ↓
2. OnBnClickedUdplink() 被调用
   ↓
3. 设置 m_bUdpThreadRunning = FALSE（通知线程退出）
   ↓
4. 关闭Socket（closesocket），recvfrom() 会返回错误
   ↓
5. 等待接收线程退出（WaitForSingleObject）
   ↓
6. 清理资源
   ↓
7. 更新UI：按钮文本改为"连接UDP"，显示"未连接"
```

---

## 六、与串口通信的对比

| 特性 | 串口通信 | UDP通信 |
|------|---------|---------|
| **初始化位置** | OnInitDialog() | OnInitDialog() |
| **连接函数** | OpenSerialPort() | ConnectUdp() |
| **断开函数** | CloseSerialPort() | DisconnectUdp() |
| **发送函数** | SendData() | SendUdpData() |
| **接收方式** | 工作线程 ReadFile() | 工作线程 recvfrom() |
| **线程函数** | SerialReadThread() | UdpRecvThread() |
| **状态标志** | m_bPortOpen | m_bUdpConnected |
| **句柄类型** | HANDLE | SOCKET |
| **按钮事件** | OnBnClickedSeriallink() | OnBnClickedUdplink() |

**统一管理建议：**
- 两个通信模块独立，互不干扰
- 可以同时连接串口和UDP
- 按钮状态独立管理
- 数据接收后统一处理（可以区分来源）

---

## 七、UI控件需求

### 需要添加的控件：

1. **UDP连接相关**
   - IP地址输入框（Edit Control）：`IDC_EDIT_UDP_IP`
   - 远程端口输入框（Edit Control）：`IDC_EDIT_UDP_REMOTE_PORT`
   - 本地端口输入框（Edit Control）：`IDC_EDIT_UDP_LOCAL_PORT`（可选）
   - UDP状态显示（Static Text）：`IDC_STATIC_UDP_STATUS`

2. **串口连接相关**
   - 串口选择（ComboBox）：`IDC_COMBO_SERIAL_PORT`
   - 波特率选择（ComboBox）：`IDC_COMBO_BAUD_RATE`
   - 串口状态显示（Static Text）：`IDC_STATIC_SERIAL_STATUS`

3. **数据显示**
   - 接收数据显示（Edit Control 或 ListBox）：`IDC_EDIT_RECEIVE_DATA`
   - 发送数据输入（Edit Control）：`IDC_EDIT_SEND_DATA`

---

## 八、实现步骤建议

### 第一阶段：UDP基础功能
1. ✅ 添加UDP相关成员变量到头文件
2. ✅ 实现 InitUdpSocket()
3. ✅ 实现 ConnectUdp() / DisconnectUdp()
4. ✅ 实现 SendUdpData()
5. ✅ 实现 UdpRecvThread()
6. ✅ 实现 OnUdpDataReceivedMsg()
7. ✅ 完善 OnBnClickedUdplink()

### 第二阶段：UI完善
1. 添加IP、端口输入控件
2. 添加状态显示控件
3. 实现按钮状态切换（连接/断开）
4. 实现数据发送UI

### 第三阶段：业务逻辑
1. 实现握手协议（如果需要）
2. 实现数据解析
3. 实现错误处理
4. 实现重连机制（可选）

### 第四阶段：串口通信（并行开发）
1. 参考UDP实现串口通信
2. 统一数据接收处理接口
3. 实现双通道数据管理

---

## 九、注意事项

### 1. Winsock库初始化
- 在 `OnInitDialog()` 中调用 `WSAStartup()`
- 在 `OnDestroy()` 中调用 `WSACleanup()`
- 只需要初始化一次

### 2. 线程安全
- **绝对不要**在工作线程中直接操作UI控件
- 使用 `PostMessage()` 从线程通知主线程
- 数据缓冲区需要动态分配或使用静态缓冲区

### 3. Socket资源管理
- Socket关闭后，`recvfrom()` 会立即返回错误
- 确保线程退出后再关闭Socket
- 使用 `WaitForSingleObject()` 等待线程结束

### 4. 错误处理
- 网络错误是常见的，需要处理
- 连接断开检测（recvfrom返回0或错误）
- 超时处理（如果需要）

### 5. 数据缓冲区
- UDP数据包有大小限制（通常65507字节）
- 接收缓冲区要足够大
- 发送大数据时需要分包

---

## 十、代码文件总结

### 需要修改的文件：

1. **FW_GCS_DlgDlg.h**
   - 添加UDP相关成员变量
   - 添加UDP相关函数声明
   - 添加消息映射声明

2. **FW_GCS_DlgDlg.cpp**
   - 添加Winsock头文件：`#include <winsock2.h>`
   - 实现所有UDP相关函数
   - 添加消息映射
   - 修改构造函数、OnInitDialog()、OnDestroy()

3. **Resource.h / FWGCSDlg.rc**
   - 添加UI控件ID定义
   - 设计对话框布局

4. **FW_GCS_Dlg.vcxproj**（如果需要）
   - 确保链接 ws2_32.lib（Winsock库）

---

## 总结

**核心思路：**
- ✅ 所有代码在 `FW_GCS_DlgDlg` 类中实现
- ✅ 使用工作线程处理UDP接收，避免阻塞UI
- ✅ 使用消息机制实现线程间通信
- ✅ UDP和串口独立管理，可以同时工作
- ✅ 按钮控制连接/断开状态切换

**关键函数位置：**
- UDP初始化：`OnInitDialog()`
- UDP连接：`OnBnClickedUdplink()` → `ConnectUdp()`
- UDP发送：`SendUdpData()`（可在任何地方调用）
- UDP接收：`UdpRecvThread()` → `OnUdpDataReceivedMsg()`
- UDP断开：`OnBnClickedUdplink()` → `DisconnectUdp()`
- 资源清理：`OnDestroy()`

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

