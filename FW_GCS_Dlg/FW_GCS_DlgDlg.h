
// FW_GCS_DlgDlg.h: 头文件
//

#pragma once

#include <afxwin.h>      // MFC基础类型
#include <afxmt.h>       // MFC同步对象（CCriticalSection, CSingleLock）
#include <afxdialogex.h> // CDialogEx
#include "UdpData.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include "Page1Dlg.h"
#include "Page2Dlg.h"
#include "MbtilesReader.h"
#include <wrl.h>

#if defined(__has_include)
#if __has_include(<WebView2.h>)
#include <WebView2.h>
#define FW_GCS_WITH_WEBVIEW2 1
#else
#define FW_GCS_WITH_WEBVIEW2 0
#endif
#else
#define FW_GCS_WITH_WEBVIEW2 0
#endif

// UDP配置参数宏
// 注意：UDP_REMOTE_IP 和 UDP_REMOTE_PORT 对应 Simulink（发送端）的本地地址和端口，用于：
//   1. 接收验证：检查收到的数据包是否来自这个地址（Simulink的源地址）
//   2. 发送目标：程序发送数据时发送到这个地址（Simulink的监听地址）
//
// Simulink 配置对应关系：
//   Simulink 本地地址 = UDP_REMOTE_IP (127.0.0.1)
//   Simulink 本地端口 = UDP_REMOTE_PORT (5000) - Simulink需要监听此端口接收程序发送的数据
//   Simulink 远程地址 = 127.0.0.1 (程序所在地址)
//   Simulink 远程端口 = UDP_LOCAL_PORT (5001) - Simulink发送数据的目标端口
//
#define UDP_REMOTE_IP      "192.168.1.11"   // 远程设备IP（飞控固件IP，用于实际连接）
//#define UDP_REMOTE_IP      "127.0.0.1"         // Simulink的本地IP（用于本地测试）
#define UDP_REMOTE_PORT     50000                // Simulink的本地端口（用于本地测试）地面站远程端口
#define UDP_LOCAL_PORT      50001                 // 本程序监听端口（接收Simulink发送的数据）

// 串口配置参数宏
#define SERIAL_PORT_NAME   "COM20"        // 目标串口名称（RS422串口，格式：COM1-COM256）
#define SERIAL_BAUD_RATE   115200         // 波特率（常用值：9600, 19200, 38400, 57600, 115200）

#define   UI_UPDATE_INTERVAL_MS 25		  // 显示控件刷新频率

// 自定义消息：UDP数据接收
#define WM_UDP_DATA_RECEIVED  (WM_USER + 200)
// 自定义消息：串口数据接收（由串口接收线程发送到主线程）
#define WM_SERIAL_DATA_RECEIVED  (WM_USER + 201)

// CFWGCSDlgDlg 对话框
class CFWGCSDlgDlg : public CDialogEx
{
// 构造
public:
	CFWGCSDlgDlg(CWnd* pParent = nullptr);	// 标准构造函数
	~CFWGCSDlgDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FW_GCS_DLG_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// UDP通信相关成员变量
	SOCKET m_udpSocket;                    // UDP Socket句柄
	BOOL m_bUdpConnected;                  // UDP连接状态标志
	sockaddr_in m_udpRemoteAddr;           // 远程地址结构
	CWinThread* m_pUdpRecvThread;         // UDP接收线程指针
	BOOL m_bUdpThreadRunning;             // 线程运行标志
	BOOL m_bUdpRemoteResponded;           // 远程地址响应标志（用于验证连接）
	CCriticalSection m_csUdpResponse;      // 保护响应标志的临界区
	DWORD m_dwLastUdpUiUpdate;            // 上次UI更新时间（用于限频）
	
	// 串口通信相关成员变量
	HANDLE m_hSerialPort;                  // 串口句柄
	BOOL m_bSerialConnected;                // 串口连接状态标志
	CWinThread* m_pSerialRecvThread;       // 串口接收线程指针
	BOOL m_bSerialThreadRunning;           // 串口线程运行标志
	BYTE m_serialBuffer[2048];             // 串口接收缓冲区（用于处理不完整数据包，增大到2048避免溢出）
	int m_nSerialBufferSize;               // 缓冲区中已有数据大小
	
	// UI控件变量（保留用于兼容，但数据将显示在子对话框中）
	CEdit m_editData1;                     // data1数据显示控件（IDC_Display0）
	CEdit m_editData2;                     // data2数据显示控件（IDC_Display1）
	CEdit m_editData3;                     // data3数据显示控件（IDC_Display2）
	CEdit m_editData4;                     // data4数据显示控件（IDC_Display3）
	CEdit m_editData5;                     // data5数据显示控件（IDC_Display4）
	
	// 视窗组1相关控件
	CEdit m_editDisplay5;                  // 俯仰角速率（IDC_Display5）
	CEdit m_editDisplay6;                  // 滚转角速率（IDC_Display6）
	CEdit m_editDisplay7;                  // 航向角速率（IDC_Display7）
	CEdit m_editDisplay8;                  // 俯仰角加速度（IDC_Display8）
	CEdit m_editDisplay9;                  // 滚转角加速度（IDC_Display9）
	CEdit m_editDisplay10;                 // 航向角加速度（IDC_Display10）
	CEdit m_editDisplay11;                 // 法向过载（IDC_Display11）
	CEdit m_editDisplay12;                 // 纵向过载（IDC_Display12）
	CEdit m_editDisplay13;                 // 横向过载（IDC_Display13）
	
	// 视窗组2相关控件
	CEdit m_editDisplay14;                 // 气压高度（IDC_Display14）
	CEdit m_editDisplay15;                 // 无线电高度（IDC_Display15）
	CEdit m_editDisplay16;                 // 气压空速（IDC_Display16）
	CEdit m_editDisplay17;                 // 表速（IDC_Display17）
	CEdit m_editDisplay18;                 // 马赫数（IDC_Display18）
	CEdit m_editDisplay19;                 // 东向速度（IDC_Display19）
	CEdit m_editDisplay20;                 // 北向速度（IDC_Display20）
	CEdit m_editDisplay21;                 // 天向速度（IDC_Display21）
	CEdit m_editDisplay22;                 // 大气温度（IDC_Display22）
	
	// 视窗组3相关控件
	CEdit m_editDisplay23;                 // 油门控制（IDC_Display23）
	CEdit m_editDisplay24;                 // 发动机缸温（IDC_Display24）
	CEdit m_editDisplay25;                 // 发动机转速（IDC_Display25）
	CEdit m_editDisplay26;                 // 剩余油量（IDC_Display26）
	
	// 视窗组4相关控件
	CEdit m_editDisplay27;                 // 1#舵偏指令（IDC_Display27）
	CEdit m_editDisplay28;                 // 2#舵偏指令（IDC_Display28）
	CEdit m_editDisplay29;                 // 3#舵偏指令（IDC_Display29）
	CEdit m_editDisplay30;                 // 4#舵偏指令（IDC_Display30）
	// m_editDisplay31 已移除，因为 IDC_Display31 在资源文件中不存在
	CEdit m_editDisplay32;                 // 转弯舵机指令（IDC_Display32），注意：原 controlCommand 已改为 Radio Button
	
	// 视窗组5相关控件（GPS）
	CEdit m_editDisplay33;                 // 卫星经度（IDC_Display33）
	CEdit m_editDisplay34;                 // 卫星纬度（IDC_Display34）
	CEdit m_editDisplay35;                 // 卫星高度（IDC_Display35）
	CEdit m_editDisplay36;                 // 卫星定位状态（IDC_Display36）
	CEdit m_editDisplay37;                 // 卫星收星数（IDC_Display37）
	CEdit m_editDisplay38;                 // 卫星地速航向（IDC_Display38）
	CEdit m_editDisplay39;                 // 卫星地速（IDC_Display39）
	CEdit m_editDisplay40;                 // 卫星垂直速度（IDC_Display40）
	CEdit m_editDisplay41;                 // GPS时（IDC_Display41）
	CEdit m_editDisplay42;                 // GPS分（IDC_Display42）
	CEdit m_editDisplay43;                 // GPS秒（IDC_Display43）
	
	// 视窗组6相关控件（导航）
	CEdit m_editDisplay44;                 // 导航状态（IDC_Display44）
	CEdit m_editDisplay45;                 // 当前航线号（IDC_Display45）
	CEdit m_editDisplay46;                 // 目标航点（IDC_Display46）
	CEdit m_editDisplay47;                 // 偏航距（IDC_Display47）
	CEdit m_editDisplay48;                 // 偏航角（IDC_Display48）
	CEdit m_editDisplay49;                 // 待飞距（IDC_Display49）
	CEdit m_editDisplay50;                 // 应飞航向（IDC_Display50）
	CEdit m_editDisplay51;                 // 应飞速度（IDC_Display51）
	CEdit m_editDisplay52;                 // 应飞高度（IDC_Display52）
	CEdit m_editDisplay53;                 // 应飞时间（IDC_Display53）
	
	// 视窗组7相关控件（目标）
	CEdit m_editDisplay54;                 // 目标经度（IDC_Display54）
	CEdit m_editDisplay55;                 // 目标纬度（IDC_Display55）
	CEdit m_editDisplay56;                 // 目标高度（IDC_Display56）
	CEdit m_editDisplay57;                 // 目标速度（IDC_Display57）
	CEdit m_editDisplay58;                 // 目标航向（IDC_Display58）
	
	// 视窗组8相关控件（载荷）
	CEdit m_editDisplay59;                 // 载荷类型（IDC_Display59）
	CEdit m_editDisplay60;                 // 剩余弹量（IDC_Display60）
	CEdit m_editDisplay61;                 // 自检结果（IDC_Display61）
	CEdit m_editDisplay62;                 // 电池电压（IDC_Display62）
	CEdit m_editDisplay63;                 // 工作流程（IDC_Display63）- 已弃用，保留用于兼容
	CEdit m_editDisplay64;                 // 报警状态字（IDC_Display64）- 已弃用，保留用于兼容
	CEdit m_editDisplay65;                 // 开关量状态（IDC_Display65）- 已弃用，保留用于兼容
	
	// 扩展协议 Radio Button 控件（指示灯显示，只读）
	CButton m_radioFlag1;                   // 开伞指令标志（IDC_RADIO_Flag1）
	CButton m_radioFlag2;                   // 开舱指令标志（IDC_RADIO_Flag2）
	CButton m_radioFlag3;                   // 起落架指令标志（IDC_RADIO_Flag3）
	CButton m_radioFlag4;                   // 工作流程标志-0（IDC_RADIO_Flag4）
	CButton m_radioFlag5;                   // 工作流程标志-1（IDC_RADIO_Flag5）
	CButton m_radioFlag6;                   // 发射状态标志（IDC_RADIO_Flag6）
	CButton m_radioFlag7;                   // 电池电压低报警标志（IDC_RADIO_Flag7）
	CButton m_radioFlag8;                   // 高度报警标志（IDC_RADIO_Flag8）
	CButton m_radioFlag9;                   // 油量低报警标志（IDC_RADIO_Flag9）
	CButton m_radioFlag10;                  // 转速异常报警标志（IDC_RADIO_Flag10）
	CButton m_radioFlag11;                  // 空速异常报警标志（IDC_RADIO_Flag11）
	CButton m_radioFlag12;                  // GPS定位精度低报警标志（IDC_RADIO_Flag12）
	CButton m_radioFlag13;                  // 发动机并网状态（IDC_RADIO_Flag13）
	CButton m_radioFlag14;                  // 发动机启动状态（IDC_RADIO_Flag14）
	CButton m_radioFlag15;                  // 盘旋状态（IDC_RADIO_Flag15）
	CButton m_radioFlag16;                  // 归航状态（IDC_RADIO_Flag16）
	CButton m_radioFlag17;                  // 关车状态（IDC_RADIO_Flag17）
	CButton m_radioFlag18;                  // 起落架收放状态-0（IDC_RADIO_Flag18）
	CButton m_radioFlag19;                  // 起落架收放状态-1（IDC_RADIO_Flag19）
	CButton m_radioFlag20;                  // 开伞状态（IDC_RADIO_Flag20）
	CButton m_radioFlag21;                  // 夜航灯开关状态（IDC_RADIO_Flag21）
	
	// 子对话框（分页）
	CPage1Dlg* m_pPage1Dlg;                // 第一页子对话框指针
	CPage2Dlg* m_pPage2Dlg;                // 第二页子对话框指针
	int m_nCurrentPage;                    // 当前显示的页面（0=第一页，1=第二页）
	
	// 子对话框管理函数
	BOOL CreateChildDialogs();             // 创建子对话框
	void ShowPage(int nPage);              // 显示指定页面，隐藏其他页面
	void DestroyChildDialogs();            // 销毁子对话框

	uint8_t calculateChecksum(const void* data, size_t len);  // 计算校验和函数
	
	// UDP通信相关函数
	BOOL InitUdpSocket();                  // 初始化UDP Socket
	BOOL ConnectUdp();                     // 连接UDP并握手
	void DisconnectUdp();                  // 断开UDP连接
	BOOL SendUdpData(const void* pData, int nSize);  // 发送UDP数据
	BOOL SendHandshake();                  // 发送握手数据包
	void ProcessReceivedData(const UdpRecvDataPacket* pPacket);  // 处理接收到的数据包
	static UINT UdpRecvThread(LPVOID pParam);  // UDP接收线程函数（静态）
	void UpdateControlText(UINT nID, const CString& strText);  // 辅助函数：更新控件文本（优先在子对话框中查找）
	
	// 串口通信相关函数
	BOOL OpenSerialPort();                 // 打开串口
	void CloseSerialPort();                // 关闭串口
	BOOL SendSerialData(const void* pData, int nSize);  // 发送串口数据
	void ProcessSerialReceivedData(const UdpRecvDataPacket* pPacket);  // 处理串口接收到的数据包
	static UINT SerialRecvThread(LPVOID pParam);  // 串口接收线程函数（静态）

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedUdplink();
	afx_msg void OnDestroy();
	afx_msg LRESULT OnUdpDataReceivedMsg(WPARAM wParam, LPARAM lParam);  // 自定义消息：UDP数据接收
	afx_msg LRESULT OnSerialDataReceivedMsg(WPARAM wParam, LPARAM lParam);  // 自定义消息：串口数据接收
	afx_msg void OnBnClickedPage1();      // 切换到第一页
	afx_msg void OnBnClickedPage2();      // 切换到第二页
	afx_msg void OnSize(UINT nType, int cx, int cy);  // 窗口大小改变时调整子对话框位置
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);  // 拦截只读 Radio Button 的点击
	virtual BOOL PreTranslateMessage(MSG* pMsg);  // 拦截鼠标消息，阻止只读 Radio Button 的点击
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedSeriallink();
	afx_msg void OnBnClickedButton1();
public:
	// 提供给子对话框安全调用的UDP发送封装
	BOOL SendUdpDataPublic(const void* pData, int nSize) { return SendUdpData(pData, nSize); }

private:
	// 离线地图（MBTiles + WebView2）
	HWND m_hMapHostWnd = nullptr;
#if FW_GCS_WITH_WEBVIEW2
	Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_webViewEnvironment;
	Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_webViewController;
	Microsoft::WRL::ComPtr<ICoreWebView2> m_webView;
#endif
	CMbtilesReader m_mbtilesReader;
	MbtilesMetadata m_mbtilesMetadata;
	CString m_mbtilesPath;
	bool m_comInitialized = false;

	void InitMapWebView();
	void ResizeMapWebView(int cx, int cy);
	CString BuildMapHtml() const;
	CString GetDefaultMbtilesPath() const;
};
