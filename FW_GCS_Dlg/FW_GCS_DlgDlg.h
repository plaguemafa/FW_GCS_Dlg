
// FW_GCS_DlgDlg.h: 头文件
//

#pragma once

#include <afxwin.h>      // MFC基础类型
#include <afxmt.h>       // MFC同步对象（CCriticalSection, CSingleLock）
#include <afxdialogex.h> // CDialogEx
#include "UdpData.h"
#include "UdpConfig.h"
#include "SerialConfig.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include "Page1Dlg.h"
#include "Page2Dlg.h"
#include "MbtilesReader.h"
#include "UdpSettingsDlg.h"
#include "SerialSettingsDlg.h"
#include <wrl.h>
#include <vector>

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

#define   UI_UPDATE_INTERVAL_MS 25		  // 显示控件刷新频率

// 自定义消息：UDP数据接收
#define WM_UDP_DATA_RECEIVED  (WM_USER + 200)
// 自定义消息：串口数据接收（由串口接收线程发送到主线程）
#define WM_SERIAL_DATA_RECEIVED  (WM_USER + 201)
// 自定义消息：地图选点结果（JS 左键选点后发回经纬度，由主线程更新 Page2）
#define WM_MAP_PICK_TARGET_RESULT    (WM_USER + 202)
#define WM_MAP_PICK_PARACHUTE_RESULT (WM_USER + 203)
#define WM_MAP_PICK_LAUNCH_RESULT    (WM_USER + 204)
#define WM_MAP_PICK_WAYPOINT_RESULT  (WM_USER + 205)

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
	CMenu m_mainMenu;					   // 菜单栏
	CFont m_menuFont;					   // 菜单字体
	int m_menuItemHeight = 0;			   // 菜单项高度
	CBrush m_menuBrush;				   // 菜单背景刷
	
	// 控制指令状态跟踪（默认：地面测试流程，手动遥控模式）
	uint8_t m_missionCommand_B0;            // 0=地面测试流程, 1=发射流程
	uint8_t m_missionCommand_B1;            // 自检指令
	uint8_t m_missionCommand_B2;            // 参数装订指令
	uint8_t m_missionCommand_B3;            // 舵面检查指令
	uint8_t m_missionCommand_B4;            // 发动机检查指令
	uint8_t m_missionCommand_B5;            // 发射指令
	uint8_t m_controlMode_B0;               // 0=手动遥控, 1=半自主, 2=全自主

	// UDP通信相关成员变量
	SOCKET m_udpSocket;                    // UDP Socket句柄
	BOOL m_bUdpConnected;                  // UDP连接状态标志
	sockaddr_in m_udpRemoteAddr;           // 远程地址结构
	CWinThread* m_pUdpRecvThread;         // UDP接收线程指针
	BOOL m_bUdpThreadRunning;             // 线程运行标志
	BOOL m_bUdpRemoteResponded;           // 远程地址响应标志（用于验证连接）
	CCriticalSection m_csUdpResponse;      // 保护响应标志的临界区
	DWORD m_dwLastUdpUiUpdate;            // 上次UI更新时间（用于限频）
	// 通信丢包报警：统计建立后的平均收包率，超过 10 倍平均间隔未收包则弹窗
	DWORD m_dwUdpRecvCount;              // 连接后累计收到的 UDP 包数
	ULONGLONG m_ullUdpConnectTime;       // 连接成功时的时间（GetTickCount64）
	ULONGLONG m_ullLastUdpRecvTime;      // 最后一次收到包的时间
	BOOL m_bPacketLossAlarmShown;        // 是否已向 JS 发送了“通信丢包”弹窗（恢复后置 FALSE）
	
	// UDP配置参数（运行时配置，优先于宏定义）
	CString m_strUdpLocalIP;               // 本机IP（通常为"0.0.0.0"表示监听所有接口）
	int m_nUdpLocalPort;                  // 本机端口
	CString m_strUdpRemoteIP;             // 远程IP
	int m_nUdpRemotePort;                 // 远程端口
	
	// 串口配置参数（运行时配置，优先于宏定义）
	CString m_strSerialPortName;          // 串口名称（例如 "COM21"）
	int     m_nSerialBaudRate;            // 串口波特率（例如 115200）
	
	// 串口通信相关成员变量
	HANDLE m_hSerialPort;                  // 串口句柄
	BOOL m_bSerialConnected;                // 串口连接状态标志
	CWinThread* m_pSerialRecvThread;       // 串口接收线程指针
	BOOL m_bSerialThreadRunning;           // 串口线程运行标志
	BYTE m_serialBuffer[2048];             // 串口接收缓冲区（用于处理不完整数据包，增大到2048避免溢出）
	int m_nSerialBufferSize;               // 缓冲区中已有数据大小
	
#if 0
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
#endif
	
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
	BOOL SendControlCommand();             // 发送控制指令（UdpSendDataPacket_Cmd）
	void ProcessReceivedData(const UdpRecvDataPacket* pPacket);  // 处理接收到的数据包
	static UINT UdpRecvThread(LPVOID pParam);  // UDP接收线程函数（静态）
	void UpdateControlText(UINT nID, const CString& strText);  // 辅助函数：更新控件文本（优先在子对话框中查找）
	void ClearAllDisplayData();  // 断开UDP后清除所有显示控件及JS端数据为0
	
	// UDP配置管理函数
	void LoadUdpConfig();                 // 从注册表加载UDP配置（如果没有则使用宏默认值）
	void SaveUdpConfig();                 // 保存UDP配置到注册表
	
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
	afx_msg void OnTimer(UINT_PTR nIDEvent);  // 定时检查 UDP 收包超时（通信丢包报警）
	afx_msg LRESULT OnUdpDataReceivedMsg(WPARAM wParam, LPARAM lParam);  // 自定义消息：UDP数据接收
	afx_msg LRESULT OnSerialDataReceivedMsg(WPARAM wParam, LPARAM lParam);  // 自定义消息：串口数据接收
	afx_msg void OnSize(UINT nType, int cx, int cy);  // 窗口大小改变时调整子对话框位置
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMenuUdpSettings();      // UDP通信设置菜单项
	afx_msg void OnMenuSerialSettings();   // 串口通信设置菜单项
	afx_msg void OnMenuUdpLink();          // UDP连接（原 IDC_UDPlink 功能）
	afx_msg void OnMenuSerialLink();       // 串口连接（原 IDC_SerialLink 功能）
	// 控制模式菜单项
	afx_msg void OnMenuCtrlModeManual();   // 手动遥控模式
	afx_msg void OnMenuCtrlModeSemi();     // 半自主模式
	afx_msg void OnMenuCtrlModeFull();     // 全自主模式
	// 任务指令菜单项
	afx_msg void OnMenuMissionTest();      // 地面测试流程
	afx_msg void OnMenuMissionLaunchProc(); // 发射流程
	afx_msg void OnMenuMissionBindParam(); // 参数装订
	afx_msg void OnMenuMissionLaunchCmd(); // 发射指令
	// 自检指令菜单项
	afx_msg void OnMenuCheckSelf();        // 自检指令
	afx_msg void OnMenuCheckSurface();     // 舵面检查
	afx_msg void OnMenuCheckEngine();      // 发动机检查
	afx_msg void OnMenuCheckDetail();      // 详细自检结果：呼出 Page1 对话框
	// 位置装订菜单项
	afx_msg void OnMenuMapMouseCoord();    // 启用/关闭鼠标经纬度显示
	afx_msg void OnMenuMapWaypointPickConnect(); // 航点连线显示开关
	afx_msg void OnMenuMapWaypointPick();  // 航路点：地图选点（多次点击依次装订1~100航路点）
	afx_msg void OnMenuMapTargetPick();    // 目标点：地图选点
	afx_msg void OnMenuMapParachutePick(); // 开伞点：地图选点
	afx_msg void OnMenuMapLaunchPick();    // 发射点：地图选点
	afx_msg void OnMenuShowPage2();        // 装订数据：呼出 Page2 对话框
	// 菜单更新函数（用于显示选中状态）
	afx_msg void OnUpdateMenuCtrlModeManual(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuCtrlModeSemi(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuCtrlModeFull(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuMissionTest(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuMissionLaunchProc(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuMissionBindParam(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuMissionLaunchCmd(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuCheckSelf(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuCheckSurface(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuCheckEngine(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuMapMouseCoord(CCmdUI* pCmdUI); // 更新鼠标经纬度菜单勾选状态
	afx_msg void OnUpdateMenuMapWaypointPickConnect(CCmdUI* pCmdUI); // 更新航点连线显示菜单状态
	afx_msg void OnUpdateMenuMapWaypointPick(CCmdUI* pCmdUI); // 更新航路点选点菜单状态
	afx_msg void OnUpdateMenuMapTargetPick(CCmdUI* pCmdUI); // 更新目标点选点菜单状态
	afx_msg void OnUpdateMenuMapParachutePick(CCmdUI* pCmdUI); // 更新开伞点选点菜单状态
	afx_msg void OnUpdateMenuMapLaunchPick(CCmdUI* pCmdUI);    // 更新发射点选点菜单状态
	afx_msg void OnNcPaint();
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedMenuBtn1();
	afx_msg void OnBnClickedMenuBtn2();
	afx_msg void OnBnClickedMenuBtn3();
	afx_msg void OnBnClickedMenuBtn4();
	afx_msg void OnBnClickedMenuBtn5();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);  // 拦截只读 Radio Button 的点击
	virtual BOOL PreTranslateMessage(MSG* pMsg);  // 拦截鼠标消息，阻止只读 Radio Button 的点击
	afx_msg LRESULT OnMapPickTargetResult(WPARAM wParam, LPARAM lParam);       // 目标点选点结果（写入 EditData17/18）
	afx_msg LRESULT OnMapPickParachuteResult(WPARAM wParam, LPARAM lParam);    // 开伞点选点结果（写入 EditData23/24）
	afx_msg LRESULT OnMapPickLaunchResult(WPARAM wParam, LPARAM lParam);       // 发射点选点结果（写入 EditData2/3）
	afx_msg LRESULT OnMapPickWaypointResult(WPARAM wParam, LPARAM lParam);     // 航路点选点结果（填入 Page2 航路点列表）
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedSeriallink();
	//afx_msg void OnBnClickedButton1();
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
	CMbtilesReader m_mbtilesReader;  // 全国底图（粗分辨率）
	MbtilesMetadata m_mbtilesMetadata;
	CString m_mbtilesPath;
	int m_baseMinZoom = 0;   // 底图最小层级，用于限制滚轮缩小不低于粗略全局地图最小分辨率
	int m_baseMaxZoom = 10;  // 底图最大层级（合并局部图前的 maxZoom），用于无局部精细区域时限制放大
	
	// 局部精细地图（支持多个）
	struct LocalMapInfo
	{
		MbtilesMetadata metadata;
		CString path;
	};
	std::vector<LocalMapInfo> m_localMaps;
	
	bool m_comInitialized = false;

	// 鼠标经纬度提示开关（由“位置装订”菜单控制）
	bool m_mouseCoordEnabled = false;
	// 航点连线显示开关（由“航点连线显示”菜单控制），启动时默认打开
	bool m_waypointConnectVisible = true;
	// 航路点地图选点模式：为 true 时地图左键点击将按顺序装订 1~100 号航路点
	bool m_mapPickWaypointMode = false;
	// 目标点地图选点模式：为 true 时地图左键点击将把经纬度写入 Page2 目标点编辑框
	bool m_mapPickTargetMode = false;
	// 开伞点地图选点模式：为 true 时地图左键点击将把经纬度写入 Page2 开伞点编辑框
	bool m_mapPickParachuteMode = false;
	// 发射点地图选点模式：为 true 时地图左键点击将把经纬度写入 Page2 发射点编辑框
	bool m_mapPickLaunchMode = false;

	void InitMapWebView();
	void ResizeMapWebView(int cx, int cy);
	CString BuildMapHtml() const;
	CString GetDefaultMbtilesPath() const;
	void LoadLocalMaps();  // 加载LocalMaps文件夹内的所有.mbtiles文件
	bool GetTileFromLocalMaps(int zoom, int x, int y, std::vector<unsigned char>& outData, CString& outMimeType, bool& outIsGzip);  // 从局部地图获取瓦片
	static bool TileToLngLat(int zoom, int x, int y, double& lng, double& lat);  // 瓦片坐标转经纬度（瓦片中心点）
	void SendHudMessage(const UdpRecvDataPacket* pPacket);  // 向 WebView2 推送 HUD 数据
};
