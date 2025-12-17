# Simulink UDP发送配置说明

## 一、通信架构

```
Simulink (发送端)          MFC程序 (接收端)
     │                          │
     │  UDP数据包                │
     ├──────────────────────────>│
     │  目标: 127.0.0.1:14550    │
     │                          │
     │                          │ 接收数据
     │                          │ 本地端口: 14550
     │                          │
```

## 二、MFC程序配置（已设置）

根据您的代码配置：
- **本地接收端口**：`14550`（MFC程序监听此端口接收数据）
- **远程IP**：`127.0.0.1`（本地回环地址）
- **远程端口**：`14551`（MFC程序发送数据的目标端口）

## 三、Simulink UDP Send配置

### 方法1：使用 UDP Send 模块（推荐）

#### 步骤1：添加UDP Send模块
1. 在Simulink库浏览器中找到：**Communications Toolbox** → **UDP Send**
2. 拖放到模型中

#### 步骤2：配置UDP Send模块参数

**双击UDP Send模块，设置以下参数：**

| 参数项 | 设置值 | 说明 |
|--------|--------|------|
| **Remote address** | `127.0.0.1` | 目标IP地址（MFC程序所在地址） |
| **Remote IP port** | `14550` | 目标端口（MFC程序的本地接收端口） |
| **Local IP port source** | `Specify via dialog` | 指定本地端口 |
| **Local IP port** | `14551` | 本地端口（可选，建议与MFC的远程端口一致） |

**重要说明：**
- **Remote IP port** 必须设置为 `14550`（这是MFC程序接收数据的端口）
- **Remote address** 设置为 `127.0.0.1`（本地通信）

#### 步骤3：配置数据格式

**数据包格式必须匹配 `UdpRecvDataPacket` 结构：**

```cpp
struct UdpRecvDataPacket
{
    float data1;  // 4字节
    float data2;  // 4字节
    float data3;  // 4字节
    float data4;  // 4字节
    float data5;  // 4字节
};
// 总共20字节
```

**在Simulink中：**
- 数据类型：`single`（对应C++的float，4字节）
- 数据维度：`[5, 1]` 或 `[1, 5]`（5个float值）
- 字节序：`Little-endian`（Windows默认）

### 方法2：使用 UDP Send 模块（详细配置）

#### 数据准备模块配置：

**使用 Constant 或 Signal Generator 生成5个float值：**
```
[data1] ─┐
[data2] ─┤
[data3] ─┼─> Mux ──> UDP Send
[data4] ─┤
[data5] ─┘
```

**Mux模块设置：**
- Number of inputs: `5`
- Inputs: `[5, 1]` 或 `[1, 5]`

**UDP Send模块设置：**
- Remote address: `127.0.0.1`
- Remote IP port: `14550`
- Data type: `single`（float）
- Data dimensions: `[5, 1]` 或 `[1, 5]`

## 四、数据包格式要求

### 字节顺序（Endianness）
- **必须使用 Little-endian**（小端序）
- Windows系统默认使用小端序
- Simulink默认也是小端序

### 数据对齐
- 结构体按顺序排列，无填充
- 每个float占4字节
- 总大小：20字节

### 示例数据包（十六进制）
```
假设发送：data1=1.0, data2=2.0, data3=3.0, data4=4.0, data5=5.0

字节序列（Little-endian）：
00 00 80 3F  // data1 = 1.0
00 00 00 40  // data2 = 2.0
00 00 40 40  // data3 = 3.0
00 00 80 40  // data4 = 4.0
00 00 A0 40  // data5 = 5.0
```

## 五、测试步骤

### 1. 启动MFC程序
- 运行MFC程序
- 点击"UDP连接"按钮
- 程序开始监听端口14550

### 2. 运行Simulink模型
- 配置好UDP Send模块
- 运行Simulink模型
- 数据会自动发送到MFC程序

### 3. 验证接收
- 在MFC程序的 `IDC_Display` 控件中应该能看到data1的值
- 在调试窗口（TRACE输出）中能看到所有5个数据值

## 六、常见问题

### 问题1：MFC程序收不到数据
**可能原因：**
- Simulink的Remote IP port设置错误（应该是14550）
- 防火墙阻止了UDP通信
- MFC程序未点击"UDP连接"按钮

**解决方法：**
- 检查端口号设置
- 关闭防火墙或添加例外
- 确保MFC程序已连接UDP

### 问题2：数据格式不匹配
**可能原因：**
- 字节序不匹配（Big-endian vs Little-endian）
- 数据类型不匹配（double vs float）
- 数据维度不匹配

**解决方法：**
- 确保使用Little-endian
- 使用single类型（对应float）
- 确保数据维度为[5,1]或[1,5]

### 问题3：端口被占用
**错误信息：** "Address already in use"

**解决方法：**
- 检查是否有其他程序占用端口14550或14551
- 使用 `netstat -ano | findstr 14550` 查看端口占用
- 关闭占用端口的程序

## 七、Simulink模型示例结构

```
┌─────────────┐
│  Constant   │ data1 = 1.0
└──────┬──────┘
       │
┌──────▼──────┐
│  Constant   │ data2 = 2.0
└──────┬──────┘
       │
┌──────▼──────┐
│  Constant   │ data3 = 3.0
└──────┬──────┘
       │
┌──────▼──────┐
│  Constant   │ data4 = 4.0
└──────┬──────┘
       │
┌──────▼──────┐
│  Constant   │ data5 = 5.0
└──────┬──────┘
       │
       ▼
┌─────────────┐
│    Mux      │ 合并5个信号
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  UDP Send   │ 发送到127.0.0.1:14550
└─────────────┘
```

## 八、快速配置清单

✅ **Simulink UDP Send模块设置：**
- [ ] Remote address: `127.0.0.1`
- [ ] Remote IP port: `14550`
- [ ] Local IP port: `14551`（可选）
- [ ] Data type: `single`（float）
- [ ] Data dimensions: `[5, 1]` 或 `[1, 5]`
- [ ] Byte order: `Little-endian`

✅ **MFC程序设置：**
- [ ] 已配置本地端口：14550
- [ ] 已配置远程IP：127.0.0.1
- [ ] 已点击"UDP连接"按钮

✅ **测试验证：**
- [ ] MFC程序显示"UDP连接成功"
- [ ] Simulink模型运行
- [ ] MFC程序IDC_Display控件显示data1值

## 九、网络配置（如果不在同一台机器）

如果Simulink和MFC程序不在同一台机器上：

**Simulink配置：**
- Remote address: MFC程序所在机器的IP地址（如 `192.168.1.100`）
- Remote IP port: `14550`

**MFC程序配置：**
- 需要修改代码中的 `UDP_REMOTE_IP` 为Simulink所在机器的IP地址
- UDP_LOCAL_PORT: `14550`（保持不变）

---

**总结：Simulink UDP Send的关键设置是目标端口14550，目标IP为127.0.0.1（本地）或MFC程序所在机器的IP地址。**

