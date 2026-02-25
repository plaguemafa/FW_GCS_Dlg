
// FW_GCS_DlgDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include <winsock2.h>
#include <ws2tcpip.h>  // 用于 inet_pton()
#include "UdpData.h"   // 必须在其他头文件之前包含，确保类型定义完整
#include "HudOverlay.h"
#include "FW_GCS_Dlg.h"
#include "FW_GCS_DlgDlg.h"
#include "resource.h"
#include "afxdialogex.h"
#include "Page1Dlg.h"
#include "Page2Dlg.h"
#include <objbase.h>
#include <atlbase.h>  // 用于CRegKey注册表操作
#include <string>
#include <vector>
#include <cstdarg>
#include <cmath>
// #include <algorithm>
#include <minwindef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	void LogMap(const wchar_t* format, ...)
	{
		wchar_t buffer[1024] = {};
		va_list args;
		va_start(args, format);
		_vsnwprintf_s(buffer, _countof(buffer), _TRUNCATE, format, args);
		va_end(args);
		::OutputDebugStringW(buffer);
		::OutputDebugStringW(L"\r\n");
	}

	bool ParseTileUrl(const std::wstring& url, int& z, int& x, int& y)
	{
		z = x = y = 0;
		const std::wstring token = L"/tiles/";
		size_t pos = url.find(token);
		if (pos == std::wstring::npos)
		{
			return false;
		}

		std::wstring path = url.substr(pos + token.size());
		size_t queryPos = path.find(L'?');
		if (queryPos != std::wstring::npos)
		{
			path = path.substr(0, queryPos);
		}

		std::vector<std::wstring> parts;
		size_t start = 0;
		while (start < path.size())
		{
			size_t slash = path.find(L'/', start);
			if (slash == std::wstring::npos)
			{
				parts.push_back(path.substr(start));
				break;
			}
			parts.push_back(path.substr(start, slash - start));
			start = slash + 1;
		}

		if (parts.size() < 3)
		{
			return false;
		}

		auto trimExtension = [](std::wstring value) {
			size_t dot = value.find(L'.');
			if (dot != std::wstring::npos)
			{
				value = value.substr(0, dot);
			}
			return value;
		};

		z = _wtoi(parts[0].c_str());
		x = _wtoi(parts[1].c_str());
		y = _wtoi(trimExtension(parts[2]).c_str());
		return z >= 0 && x >= 0 && y >= 0;
	}
}

// CFWGCSDlgDlg 对话框

CFWGCSDlgDlg::CFWGCSDlgDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FW_GCS_DLG_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
	// UDP初始化
	m_udpSocket = INVALID_SOCKET;				// 初始化UDP Socket为无效句柄
	m_bUdpConnected = FALSE; 					// 初始化UDP连接状态标志为未连接
	m_pUdpRecvThread = NULL; 					// 初始化UDP接收线程指针为空
	m_bUdpThreadRunning = FALSE; 				// 初始化UDP线程运行标志为未运行
	m_bUdpRemoteResponded = FALSE; 				// 初始化远程响应标志为未响应
	m_dwLastUdpUiUpdate = 0;                   // 上次UI更新时间（限频用）
	memset(&m_udpRemoteAddr, 0, sizeof(m_udpRemoteAddr)); // 清空远程地址结构
	
	// UDP配置初始化（使用宏定义的默认值）
	m_strUdpLocalIP = CString(UDP_LOCAL_IP);   // 本机IP默认值
	m_nUdpLocalPort = UDP_LOCAL_PORT;          // 本机端口默认值
	m_strUdpRemoteIP = CString(UDP_REMOTE_IP); // 远程IP默认值
	m_nUdpRemotePort = UDP_REMOTE_PORT;        // 远程端口默认值
	
	// 控制指令状态初始化（默认：地面测试流程，手动遥控模式）
	m_missionCommand_B0 = 0;  // 0=地面测试流程
	m_missionCommand_B1 = 0;   // 自检指令（未激活）
	m_missionCommand_B2 = 0;   // 参数装订指令（未激活）
	m_missionCommand_B3 = 0;   // 舵面检查指令（未激活）
	m_missionCommand_B4 = 0;   // 发动机检查指令（未激活）
	m_missionCommand_B5 = 0;   // 发射指令（未激活）
	m_controlMode_B0 = 0;      // 0=手动遥控
	
	// 串口初始化
	m_hSerialPort = INVALID_HANDLE_VALUE;       // 串口句柄初始化为无效值
	m_bSerialConnected = FALSE;                 // 串口连接状态标志：未连接
	m_pSerialRecvThread = NULL;                 // 串口接收线程指针：未创建
	m_bSerialThreadRunning = FALSE;             // 串口线程运行标志：未运行
	m_nSerialBufferSize = 0;                    // 串口接收缓冲区大小：空
	memset(m_serialBuffer, 0, sizeof(m_serialBuffer));  // 清空接收缓冲区
	
	// 子对话框初始化
	m_pPage1Dlg = NULL;
	m_pPage2Dlg = NULL;
	m_nCurrentPage = 0;                         // 默认显示第一页
}

CFWGCSDlgDlg::~CFWGCSDlgDlg()
{
	// 确保断开UDP连接
	DisconnectUdp();
	// 确保关闭串口
	CloseSerialPort();
	// 销毁子对话框
	DestroyChildDialogs();
}

void CFWGCSDlgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
#if 0  // 已移除地图上的传统数据显示控件，保留占位以兼容历史代码
	DDX_Control(pDX, IDC_Display0, m_editData1);   // 绑定data1显示控件
	DDX_Control(pDX, IDC_Display1, m_editData2);  // 绑定data2显示控件
	DDX_Control(pDX, IDC_Display2, m_editData3);  // 绑定data3显示控件
	DDX_Control(pDX, IDC_Display3, m_editData4);  // 绑定data4显示控件
	DDX_Control(pDX, IDC_Display4, m_editData5);  // 绑定data5显示控件
	
	// 视窗组1相关控件绑定
	DDX_Control(pDX, IDC_Display5, m_editDisplay5); 
	DDX_Control(pDX, IDC_Display6, m_editDisplay6);
	DDX_Control(pDX, IDC_Display7, m_editDisplay7);
	DDX_Control(pDX, IDC_Display8, m_editDisplay8);
	DDX_Control(pDX, IDC_Display9, m_editDisplay9);
	DDX_Control(pDX, IDC_Display10, m_editDisplay10);
	DDX_Control(pDX, IDC_Display11, m_editDisplay11);
	DDX_Control(pDX, IDC_Display12, m_editDisplay12);
	DDX_Control(pDX, IDC_Display13, m_editDisplay13);
	
	// 视窗组2相关控件绑定
	DDX_Control(pDX, IDC_Display14, m_editDisplay14);
	DDX_Control(pDX, IDC_Display15, m_editDisplay15);
	DDX_Control(pDX, IDC_Display16, m_editDisplay16);
	DDX_Control(pDX, IDC_Display17, m_editDisplay17);
	DDX_Control(pDX, IDC_Display18, m_editDisplay18);
	DDX_Control(pDX, IDC_Display19, m_editDisplay19);
	DDX_Control(pDX, IDC_Display20, m_editDisplay20);
	DDX_Control(pDX, IDC_Display21, m_editDisplay21);
	DDX_Control(pDX, IDC_Display22, m_editDisplay22);
	
	// 视窗组3相关控件绑定
	DDX_Control(pDX, IDC_Display23, m_editDisplay23);
	DDX_Control(pDX, IDC_Display24, m_editDisplay24);
	DDX_Control(pDX, IDC_Display25, m_editDisplay25);
	DDX_Control(pDX, IDC_Display26, m_editDisplay26);
	
	// 视窗组4相关控件绑定
	DDX_Control(pDX, IDC_Display27, m_editDisplay27);
	DDX_Control(pDX, IDC_Display28, m_editDisplay28);
	DDX_Control(pDX, IDC_Display29, m_editDisplay29);
	DDX_Control(pDX, IDC_Display30, m_editDisplay30);
	// IDC_Display31 在资源文件中不存在，已跳过绑定
	DDX_Control(pDX, IDC_Display32, m_editDisplay32);  // 转弯舵机指令
	
	// 视窗组5相关控件绑定（GPS）
	DDX_Control(pDX, IDC_Display33, m_editDisplay33);
	DDX_Control(pDX, IDC_Display34, m_editDisplay34);
	DDX_Control(pDX, IDC_Display35, m_editDisplay35);
	DDX_Control(pDX, IDC_Display36, m_editDisplay36);
	DDX_Control(pDX, IDC_Display37, m_editDisplay37);
	DDX_Control(pDX, IDC_Display38, m_editDisplay38);
	DDX_Control(pDX, IDC_Display39, m_editDisplay39);
	DDX_Control(pDX, IDC_Display40, m_editDisplay40);
	DDX_Control(pDX, IDC_Display41, m_editDisplay41);
	DDX_Control(pDX, IDC_Display42, m_editDisplay42);
	DDX_Control(pDX, IDC_Display43, m_editDisplay43);
	
	// 视窗组6相关控件绑定（导航）
	DDX_Control(pDX, IDC_Display44, m_editDisplay44);
	DDX_Control(pDX, IDC_Display45, m_editDisplay45);
	DDX_Control(pDX, IDC_Display46, m_editDisplay46);
	DDX_Control(pDX, IDC_Display47, m_editDisplay47);
	DDX_Control(pDX, IDC_Display48, m_editDisplay48);
	DDX_Control(pDX, IDC_Display49, m_editDisplay49);
	DDX_Control(pDX, IDC_Display50, m_editDisplay50);
	DDX_Control(pDX, IDC_Display51, m_editDisplay51);
	DDX_Control(pDX, IDC_Display52, m_editDisplay52);
	DDX_Control(pDX, IDC_Display53, m_editDisplay53);
	
	// 视窗组7相关控件绑定（目标）
	DDX_Control(pDX, IDC_Display54, m_editDisplay54);
	DDX_Control(pDX, IDC_Display55, m_editDisplay55);
	DDX_Control(pDX, IDC_Display56, m_editDisplay56);
	DDX_Control(pDX, IDC_Display57, m_editDisplay57);
	DDX_Control(pDX, IDC_Display58, m_editDisplay58);
	
	// 视窗组8相关控件绑定（载荷）
	DDX_Control(pDX, IDC_Display59, m_editDisplay59);
	DDX_Control(pDX, IDC_Display60, m_editDisplay60);
	DDX_Control(pDX, IDC_Display61, m_editDisplay61);
	DDX_Control(pDX, IDC_Display62, m_editDisplay62);
	
	// 扩展协议 Radio Button 控件绑定（在 OnInitDialog 中手动绑定，避免 DDX_Control 异常）
	// 注意：Radio Button 控件使用 SubclassWindow 方式绑定，更安全
#endif
}

BEGIN_MESSAGE_MAP(CFWGCSDlgDlg, CDialogEx) // 消息映射
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_UDPlink, &CFWGCSDlgDlg::OnBnClickedUdplink)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_UDP_DATA_RECEIVED, &CFWGCSDlgDlg::OnUdpDataReceivedMsg)
	ON_MESSAGE(WM_SERIAL_DATA_RECEIVED, &CFWGCSDlgDlg::OnSerialDataReceivedMsg)
	ON_BN_CLICKED(IDC_SerialLink, &CFWGCSDlgDlg::OnBnClickedSeriallink)
	ON_BN_CLICKED(IDC_BTN_PAGE1, &CFWGCSDlgDlg::OnBnClickedPage1)
	ON_BN_CLICKED(IDC_BTN_PAGE2, &CFWGCSDlgDlg::OnBnClickedPage2)
	ON_COMMAND(ID_MENU_UDP_SETTINGS, &CFWGCSDlgDlg::OnMenuUdpSettings)
	ON_COMMAND(ID_MENU_SERIAL_SETTINGS, &CFWGCSDlgDlg::OnMenuSerialSettings)
	// 控制模式菜单项
	ON_COMMAND(ID_MENU_CTRL_MODE_MANUAL, &CFWGCSDlgDlg::OnMenuCtrlModeManual)
	ON_COMMAND(ID_MENU_CTRL_MODE_SEMI, &CFWGCSDlgDlg::OnMenuCtrlModeSemi)
	ON_COMMAND(ID_MENU_CTRL_MODE_FULL, &CFWGCSDlgDlg::OnMenuCtrlModeFull)
	// 任务指令菜单项
	ON_COMMAND(ID_MENU_MISSION_TEST, &CFWGCSDlgDlg::OnMenuMissionTest)
	ON_COMMAND(ID_MENU_MISSION_LAUNCH_PROC, &CFWGCSDlgDlg::OnMenuMissionLaunchProc)
	ON_COMMAND(ID_MENU_MISSION_BIND_PARAM, &CFWGCSDlgDlg::OnMenuMissionBindParam)
	ON_COMMAND(ID_MENU_MISSION_LAUNCH_CMD, &CFWGCSDlgDlg::OnMenuMissionLaunchCmd)
	// 自检指令菜单项
	ON_COMMAND(ID_MENU_CHECK_SELF, &CFWGCSDlgDlg::OnMenuCheckSelf)
	ON_COMMAND(ID_MENU_CHECK_SURFACE, &CFWGCSDlgDlg::OnMenuCheckSurface)
	ON_COMMAND(ID_MENU_CHECK_ENGINE, &CFWGCSDlgDlg::OnMenuCheckEngine)
	// 菜单更新函数
	ON_UPDATE_COMMAND_UI(ID_MENU_CTRL_MODE_MANUAL, &CFWGCSDlgDlg::OnUpdateMenuCtrlModeManual)
	ON_UPDATE_COMMAND_UI(ID_MENU_CTRL_MODE_SEMI, &CFWGCSDlgDlg::OnUpdateMenuCtrlModeSemi)
	ON_UPDATE_COMMAND_UI(ID_MENU_CTRL_MODE_FULL, &CFWGCSDlgDlg::OnUpdateMenuCtrlModeFull)
	ON_UPDATE_COMMAND_UI(ID_MENU_MISSION_TEST, &CFWGCSDlgDlg::OnUpdateMenuMissionTest)
	ON_UPDATE_COMMAND_UI(ID_MENU_MISSION_LAUNCH_PROC, &CFWGCSDlgDlg::OnUpdateMenuMissionLaunchProc)
	ON_UPDATE_COMMAND_UI(ID_MENU_MISSION_BIND_PARAM, &CFWGCSDlgDlg::OnUpdateMenuMissionBindParam)
	ON_UPDATE_COMMAND_UI(ID_MENU_MISSION_LAUNCH_CMD, &CFWGCSDlgDlg::OnUpdateMenuMissionLaunchCmd)
	ON_UPDATE_COMMAND_UI(ID_MENU_CHECK_SELF, &CFWGCSDlgDlg::OnUpdateMenuCheckSelf)
	ON_UPDATE_COMMAND_UI(ID_MENU_CHECK_SURFACE, &CFWGCSDlgDlg::OnUpdateMenuCheckSurface)
	ON_UPDATE_COMMAND_UI(ID_MENU_CHECK_ENGINE, &CFWGCSDlgDlg::OnUpdateMenuCheckEngine)
	ON_WM_SIZE()
	ON_WM_MEASUREITEM()
	ON_WM_DRAWITEM()
	ON_WM_NCPAINT()
	ON_WM_INITMENUPOPUP()
END_MESSAGE_MAP()

// 在消息到达控件之前拦截鼠标点击，阻止只读 Radio Button 的交互
BOOL CFWGCSDlgDlg::PreTranslateMessage(MSG* pMsg)
{
	// // 拦截鼠标左键按下和弹起消息
	// if (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP)
	// {
	// 	// 检查消息的目标窗口是否是只读 Radio Button
	// 	CWnd* pWnd = CWnd::FromHandle(pMsg->hwnd);
	// 	if (pWnd != NULL)
	// 	{
	// 		UINT nID = pWnd->GetDlgCtrlID();
	// 		// 如果是只读显示的 Radio Button，阻止鼠标消息
	// 		if (nID >= IDC_RADIO_Flag1 && nID <= IDC_RADIO_Flag21)
	// 		{
	// 			return TRUE;  // 返回 TRUE 表示消息已处理，阻止进一步处理
	// 		}
	// 	}
	// }
	
	// 其他消息正常处理
	return CDialogEx::PreTranslateMessage(pMsg);
}

// 拦截只读 Radio Button 的点击消息，作为双重保护 
BOOL CFWGCSDlgDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// // 检查是否是 Radio Button 的点击消息（BN_CLICKED）
	// if (HIWORD(wParam) == BN_CLICKED)
	// {
	// 	UINT nID = LOWORD(wParam);
	// 	// 如果是只读显示的 Radio Button，阻止点击
	// 	if (nID >= IDC_RADIO_Flag1 && nID <= IDC_RADIO_Flag21)
	// 	{
	// 		// 获取对应的按钮并恢复之前的状态（防止状态被改变）
	// 		CButton* pBtn = NULL;
	// 		switch (nID)
	// 		{
	// 		case IDC_RADIO_Flag1: pBtn = &m_radioFlag1; break;
	// 		case IDC_RADIO_Flag2: pBtn = &m_radioFlag2; break;
	// 		case IDC_RADIO_Flag3: pBtn = &m_radioFlag3; break;
	// 		case IDC_RADIO_Flag4: pBtn = &m_radioFlag4; break;
	// 		case IDC_RADIO_Flag5: pBtn = &m_radioFlag5; break;
	// 		case IDC_RADIO_Flag6: pBtn = &m_radioFlag6; break;
	// 		case IDC_RADIO_Flag7: pBtn = &m_radioFlag7; break;
	// 		case IDC_RADIO_Flag8: pBtn = &m_radioFlag8; break;
	// 		case IDC_RADIO_Flag9: pBtn = &m_radioFlag9; break;
	// 		case IDC_RADIO_Flag10: pBtn = &m_radioFlag10; break;
	// 		case IDC_RADIO_Flag11: pBtn = &m_radioFlag11; break;
	// 		case IDC_RADIO_Flag12: pBtn = &m_radioFlag12; break;
	// 		case IDC_RADIO_Flag13: pBtn = &m_radioFlag13; break;
	// 		case IDC_RADIO_Flag14: pBtn = &m_radioFlag14; break;
	// 		case IDC_RADIO_Flag15: pBtn = &m_radioFlag15; break;
	// 		case IDC_RADIO_Flag16: pBtn = &m_radioFlag16; break;
	// 		case IDC_RADIO_Flag17: pBtn = &m_radioFlag17; break;
	// 		case IDC_RADIO_Flag18: pBtn = &m_radioFlag18; break;
	// 		case IDC_RADIO_Flag19: pBtn = &m_radioFlag19; break;
	// 		case IDC_RADIO_Flag20: pBtn = &m_radioFlag20; break;
	// 		case IDC_RADIO_Flag21: pBtn = &m_radioFlag21; break;
	// 		}
			
	// 		// 如果状态已经被改变，立即恢复（双重保护）
	// 		if (pBtn != NULL && pBtn->GetSafeHwnd() != NULL)
	// 		{
	// 			// 这里我们需要知道之前的状态，但由于 Radio Button 的特殊性
	// 			// 最好的方法是在 PreTranslateMessage 中完全阻止
	// 			// 这里作为备用保护
	// 		}

	// 		return TRUE;  // 返回 TRUE 表示已处理，阻止默认行为
	// 	}
	// }
	
	// 其他消息正常处理
	return CDialogEx::OnCommand(wParam, lParam);
}

// CFWGCSDlgDlg 消息处理程序

BOOL CFWGCSDlgDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动执行此操作
	SetIcon(m_hIcon, TRUE);		// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// 初始化菜单栏（顶部预留空间，多列左对齐）
	{
		if (m_mainMenu.CreateMenu())
		{
			CMenu menu1, menu2, menu3, menu4, menu5,menu6;
			menu1.CreatePopupMenu();
			menu2.CreatePopupMenu();
			menu3.CreatePopupMenu();
			menu4.CreatePopupMenu();
			menu5.CreatePopupMenu();
			menu6.CreatePopupMenu();
			// 控制模式菜单（默认选中手动遥控模式）
			menu1.AppendMenu(MF_STRING, ID_MENU_CTRL_MODE_MANUAL, _T("手动遥控模式"));
			menu1.AppendMenu(MF_STRING, ID_MENU_CTRL_MODE_SEMI, _T("半自动模式"));
			menu1.AppendMenu(MF_STRING, ID_MENU_CTRL_MODE_FULL, _T("全自动模式"));

			// 任务指令菜单（默认选中地面测试流程）
			menu2.AppendMenu(MF_STRING, ID_MENU_MISSION_TEST, _T("地面测试流程"));
			menu2.AppendMenu(MF_STRING, ID_MENU_MISSION_LAUNCH_PROC, _T("发射流程"));
			menu2.AppendMenu(MF_STRING, ID_MENU_MISSION_BIND_PARAM, _T("参数装订"));
			menu2.AppendMenu(MF_STRING, ID_MENU_MISSION_LAUNCH_CMD, _T("发射指令"));

			// 自检指令菜单（默认都不选中）
			menu3.AppendMenu(MF_STRING, ID_MENU_CHECK_SELF, _T("自检指令"));
			menu3.AppendMenu(MF_STRING, ID_MENU_CHECK_SURFACE, _T("舵面检查"));
			menu3.AppendMenu(MF_STRING, ID_MENU_CHECK_ENGINE, _T("发动机检查"));
			
			// 在创建菜单时就将子菜单项设置为 owner-drawn（确保 OnMeasureItem 能被调用）
			// 控制模式子菜单
			for (int i = 0; i < menu1.GetMenuItemCount(); i++)
			{
				UINT nMenuID = menu1.GetMenuItemID(i);
				if (nMenuID >= ID_MENU_CTRL_MODE_MANUAL && nMenuID <= ID_MENU_CTRL_MODE_FULL)
				{
					MENUITEMINFO mi = {};
					mi.cbSize = sizeof(mi);
					mi.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_ID;
					mi.fType = MFT_OWNERDRAW;
					mi.dwItemData = nMenuID;
					mi.wID = nMenuID;
					menu1.SetMenuItemInfo(i, &mi, TRUE);
				}
			}
			// 任务指令子菜单
			for (int i = 0; i < menu2.GetMenuItemCount(); i++)
			{
				UINT nMenuID = menu2.GetMenuItemID(i);
				if (nMenuID >= ID_MENU_MISSION_TEST && nMenuID <= ID_MENU_MISSION_LAUNCH_CMD)
				{
					MENUITEMINFO mi = {};
					mi.cbSize = sizeof(mi);
					mi.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_ID;
					mi.fType = MFT_OWNERDRAW;
					mi.dwItemData = nMenuID;
					mi.wID = nMenuID;
					menu2.SetMenuItemInfo(i, &mi, TRUE);
				}
			}
			// 自检指令子菜单
			for (int i = 0; i < menu3.GetMenuItemCount(); i++)
			{
				UINT nMenuID = menu3.GetMenuItemID(i);
				if (nMenuID >= ID_MENU_CHECK_SELF && nMenuID <= ID_MENU_CHECK_ENGINE)
				{
					MENUITEMINFO mi = {};
					mi.cbSize = sizeof(mi);
					mi.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_ID;
					mi.fType = MFT_OWNERDRAW;
					mi.dwItemData = nMenuID;
					mi.wID = nMenuID;
					menu3.SetMenuItemInfo(i, &mi, TRUE);
				}
			}

			menu4.AppendMenu(MF_STRING | MF_GRAYED, ID_MENU_OP_PLACEHOLDER, _T("航点设置"));
			menu4.AppendMenu(MF_STRING | MF_GRAYED, ID_MENU_OP_PLACEHOLDER, _T("占位符"));
			menu4.AppendMenu(MF_STRING | MF_GRAYED, ID_MENU_OP_PLACEHOLDER, _T("占位符"));
			
			menu5.AppendMenu(MF_STRING, ID_MENU_UDP_SETTINGS, _T("UDP通信设置"));
			menu5.AppendMenu(MF_STRING, ID_MENU_SERIAL_SETTINGS, _T("串口通信设置"));
			
			m_mainMenu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(menu1.Detach()), _T("控制模式"));
			m_mainMenu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(menu2.Detach()), _T("任务指令"));
			m_mainMenu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(menu3.Detach()), _T("自检指令"));
			m_mainMenu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(menu4.Detach()), _T("位置装订"));
			m_mainMenu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(menu5.Detach()), _T("通信设置"));

			// 计算菜单高度（在系统默认基础上加高，使中灰色菜单栏更易辨认）
			const int desiredHeight = GetSystemMetrics(SM_CYMENU);
			m_menuItemHeight = max(desiredHeight + 12, 36);  // 加高约 12 像素，且不低于 36 像素

			// 菜单字体按系统菜单栏实际高度计算，避免文字被裁切；加高效果由客户区顶部灰色条带体现
			LOGFONT lf = {};
			CFont* baseFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
			if (baseFont)
			{
				baseFont->GetLogFont(&lf);
			}
			lf.lfHeight = -(max(10, static_cast<int>(desiredHeight * 0.62f)));
			m_menuFont.DeleteObject();
			m_menuFont.CreateFontIndirect(&lf);

			// 设置顶层菜单为自绘，便于控制高度
			const int menuCount = m_mainMenu.GetMenuItemCount();
			for (int i = 0; i < menuCount; ++i)
			{
				MENUITEMINFO mi = {};
				mi.cbSize = sizeof(mi);
				mi.fMask = MIIM_FTYPE | MIIM_DATA;
				mi.fType = MFT_OWNERDRAW;
				mi.dwItemData = i; // 保存索引用于绘制
				m_mainMenu.SetMenuItemInfo(i, &mi, TRUE);
			}

			// 设置菜单栏背景刷与最大高度，确保整条灰色到最右端
			m_menuBrush.DeleteObject();
			m_menuBrush.CreateSolidBrush(RGB(100, 100, 100));
			MENUINFO menuInfo = {};
			menuInfo.cbSize = sizeof(menuInfo);
			menuInfo.fMask = MIM_BACKGROUND | MIM_MAXHEIGHT;
			menuInfo.hbrBack = (HBRUSH)m_menuBrush.GetSafeHandle();
			menuInfo.cyMax = m_menuItemHeight;

			// 先挂载菜单再 SetMenuInfo，便于系统按 cyMax 计算菜单栏高度
			SetMenu(nullptr);
			SetMenu(&m_mainMenu);
			::SetMenuInfo(m_mainMenu.GetSafeHmenu(), &menuInfo);
			DrawMenuBar();
			// 强制窗口重算非客户区（含菜单栏），使灰色条高度生效
			::SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);

			// 非最大化时，补偿菜单高度，保持客户区空间
			if (!IsZoomed())
			{
				RECT rcWindow;
				GetWindowRect(&rcWindow);
				const int menuH = m_menuItemHeight > 0 ? m_menuItemHeight : GetSystemMetrics(SM_CYMENU);
				SetWindowPos(nullptr, 0, 0,
					rcWindow.right - rcWindow.left,
					(rcWindow.bottom - rcWindow.top) + menuH,
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}
	}

	// 调整窗口样式，启用系统菜单/最小化/最大化，并允许调整大小
	{
		const LONG newStyleAdd = WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
		const LONG newStyleRemove = WS_HSCROLL | WS_VSCROLL;
		LONG style = ::GetWindowLong(m_hWnd, GWL_STYLE);
		style |= newStyleAdd;
		style &= ~newStyleRemove;
		::SetWindowLong(m_hWnd, GWL_STYLE, style);

		::SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
	}

	// 加载UDP配置（从注册表读取，如果没有则使用宏默认值）
	LoadUdpConfig();

	// 设置启动默认大小
	{
		// 期望的客户区大小 默认1920x1080
		const int reqClientW = 1920;
		const int reqClientH = 1080;

		// 获取当前屏幕工作区大小（不包含任务栏）
		RECT workArea;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
		int screenW = workArea.right - workArea.left;
		int screenH = workArea.bottom - workArea.top;

		// 计算需要的窗口总大小
		CRect rc(0, 0, reqClientW, reqClientH);
		AdjustWindowRectEx(&rc, GetStyle(), FALSE, GetExStyle());
		int reqWinW = rc.Width();
		int reqWinH = rc.Height();

		if (reqWinW <= screenW && reqWinH <= screenH)
		{
			// 屏幕够大，居中显示
			int x = workArea.left + (screenW - reqWinW) / 2;
			int y = workArea.top + (screenH - reqWinH) / 2;
			SetWindowPos(NULL, x, y, reqWinW, reqWinH, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		else
		{
			// 屏幕不够大，最大化
			ShowWindow(SW_SHOWMAXIMIZED);
		}
	}

	// 初始化Winsock库
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		MessageBox(_T("Winsock初始化失败！"), _T("错误"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	// 初始化UDP Socket（但不连接）
	InitUdpSocket();

#if 0
	// 初始化显示控件（保留用于兼容）
	m_editData1.SetWindowText(_T("0.00"));  // 初始化data1显示
	m_editData2.SetWindowText(_T("0.00"));  // 初始化data2显示
	m_editData3.SetWindowText(_T("0.00"));  // 初始化data3显示
	m_editData4.SetWindowText(_T("0.00"));  // 初始化data4显示
	m_editData5.SetWindowText(_T("0.00"));  // 初始化data5显示
	
	// 初始化视窗组1相关控件
	m_editDisplay5.SetWindowText(_T("0.00"));
	m_editDisplay6.SetWindowText(_T("0.00"));
	m_editDisplay7.SetWindowText(_T("0.00"));
	m_editDisplay8.SetWindowText(_T("0.00"));
	m_editDisplay9.SetWindowText(_T("0.00"));
	m_editDisplay10.SetWindowText(_T("0.00"));
	m_editDisplay11.SetWindowText(_T("0.00"));
	m_editDisplay12.SetWindowText(_T("0.00"));
	m_editDisplay13.SetWindowText(_T("0.00"));
	
	// 初始化视窗组2相关控件
	m_editDisplay14.SetWindowText(_T("0.00"));
	m_editDisplay15.SetWindowText(_T("0"));
	m_editDisplay16.SetWindowText(_T("0.00"));
	m_editDisplay17.SetWindowText(_T("0.00"));
	m_editDisplay18.SetWindowText(_T("0"));
	m_editDisplay19.SetWindowText(_T("0.00"));
	m_editDisplay20.SetWindowText(_T("0.00"));
	m_editDisplay21.SetWindowText(_T("0.00"));
	m_editDisplay22.SetWindowText(_T("0"));
	
	// 初始化视窗组3相关控件
	m_editDisplay23.SetWindowText(_T("0"));
	m_editDisplay24.SetWindowText(_T("0.00"));
	m_editDisplay25.SetWindowText(_T("0.00"));
	m_editDisplay26.SetWindowText(_T("0"));
	
	// 初始化视窗组4相关控件
	m_editDisplay27.SetWindowText(_T("0"));
	m_editDisplay28.SetWindowText(_T("0"));
	m_editDisplay29.SetWindowText(_T("0"));
	m_editDisplay30.SetWindowText(_T("0"));
	// m_editDisplay31 已移除，因为 IDC_Display31 在资源文件中不存在
	m_editDisplay32.SetWindowText(_T("0"));  // 转弯舵机指令
	
	// 初始化视窗组5相关控件（GPS）
	m_editDisplay33.SetWindowText(_T("0"));
	m_editDisplay34.SetWindowText(_T("0"));
	m_editDisplay35.SetWindowText(_T("0.00"));
	m_editDisplay36.SetWindowText(_T("0"));
	m_editDisplay37.SetWindowText(_T("0"));
	m_editDisplay38.SetWindowText(_T("0.00"));
	m_editDisplay39.SetWindowText(_T("0.00"));
	m_editDisplay40.SetWindowText(_T("0"));
	m_editDisplay41.SetWindowText(_T("0"));
	m_editDisplay42.SetWindowText(_T("0"));
	m_editDisplay43.SetWindowText(_T("0"));
	
	// 初始化视窗组6相关控件（导航）
	m_editDisplay44.SetWindowText(_T("0"));
	m_editDisplay45.SetWindowText(_T("0"));
	m_editDisplay46.SetWindowText(_T("0"));
	m_editDisplay47.SetWindowText(_T("0.00"));
	m_editDisplay48.SetWindowText(_T("0.00"));
	m_editDisplay49.SetWindowText(_T("0.00"));
	m_editDisplay50.SetWindowText(_T("0"));
	m_editDisplay51.SetWindowText(_T("0"));
	m_editDisplay52.SetWindowText(_T("0"));
	m_editDisplay53.SetWindowText(_T("0"));
	
	// 初始化视窗组7相关控件（目标）
	m_editDisplay54.SetWindowText(_T("0"));
	m_editDisplay55.SetWindowText(_T("0"));
	m_editDisplay56.SetWindowText(_T("0.00"));
	m_editDisplay57.SetWindowText(_T("0"));
	m_editDisplay58.SetWindowText(_T("0.00"));
	
	// 初始化视窗组8相关控件（载荷）
	m_editDisplay59.SetWindowText(_T("0"));
	m_editDisplay60.SetWindowText(_T("0"));
	m_editDisplay61.SetWindowText(_T("0"));
	m_editDisplay62.SetWindowText(_T("0"));
#endif
	
	// ============================================================
	// 绑定扩展协议 Radio Button 控件（使用 SubclassWindow 方式，只读显示0/1状态，不灰色但不可交互）
	// ============================================================
#if 0
	CWnd* pWnd = NULL;
	
	// 视窗组4-5：控制指令标志
	pWnd = GetDlgItem(IDC_RADIO_Flag1);
	if (pWnd != NULL) { m_radioFlag1.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag1.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag2);
	if (pWnd != NULL) { m_radioFlag2.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag2.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag3);
	if (pWnd != NULL) { m_radioFlag3.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag3.ModifyStyle(WS_TABSTOP, 0); }
	
	// 视窗组8-4：工作流程标志
	pWnd = GetDlgItem(IDC_RADIO_Flag4);
	if (pWnd != NULL) { m_radioFlag4.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag4.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag5);
	if (pWnd != NULL) { m_radioFlag5.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag5.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag6);
	if (pWnd != NULL) { m_radioFlag6.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag6.ModifyStyle(WS_TABSTOP, 0); }
	
	// 视窗组8-5：报警状态标志
	pWnd = GetDlgItem(IDC_RADIO_Flag7);
	if (pWnd != NULL) { m_radioFlag7.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag7.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag8);
	if (pWnd != NULL) { m_radioFlag8.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag8.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag9);
	if (pWnd != NULL) { m_radioFlag9.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag9.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag10);
	if (pWnd != NULL) { m_radioFlag10.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag10.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag11);
	if (pWnd != NULL) { m_radioFlag11.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag11.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag12);
	if (pWnd != NULL) { m_radioFlag12.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag12.ModifyStyle(WS_TABSTOP, 0); }
	
	// 视窗组8-6：开关量状态标志
	pWnd = GetDlgItem(IDC_RADIO_Flag13);
	if (pWnd != NULL) { m_radioFlag13.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag13.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag14);
	if (pWnd != NULL) { m_radioFlag14.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag14.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag15);
	if (pWnd != NULL) { m_radioFlag15.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag15.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag16);
	if (pWnd != NULL) { m_radioFlag16.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag16.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag17);
	if (pWnd != NULL) { m_radioFlag17.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag17.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag18);
	if (pWnd != NULL) { m_radioFlag18.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag18.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag19);
	if (pWnd != NULL) { m_radioFlag19.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag19.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag20);
	if (pWnd != NULL) { m_radioFlag20.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag20.ModifyStyle(WS_TABSTOP, 0); }
	pWnd = GetDlgItem(IDC_RADIO_Flag21);
	if (pWnd != NULL) { m_radioFlag21.SubclassWindow(pWnd->GetSafeHwnd()); m_radioFlag21.ModifyStyle(WS_TABSTOP, 0); }
#endif
	
	TRACE(_T("OnInitDialog: 所有数据显示控件已初始化\n"));
	
	// 初始化离线地图（MBTiles + WebView2）
	m_mbtilesPath = GetDefaultMbtilesPath();
	LogMap(L"[Map] MBTiles path: %s", m_mbtilesPath.GetString());
	DWORD mbtilesAttr = ::GetFileAttributesW(m_mbtilesPath);
	LogMap(L"[Map] MBTiles exists: %s", (mbtilesAttr != INVALID_FILE_ATTRIBUTES) ? L"yes" : L"no");
	CString sqlitePathLower;
	CString sqlitePathUpper;
	{
		wchar_t exePath[MAX_PATH] = {};
		::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		CString exeDir(exePath);
		int pos = exeDir.ReverseFind(L'\\');
		if (pos >= 0)
		{
			exeDir = exeDir.Left(pos);
		}
		sqlitePathLower = exeDir + _T("\\sqlite3.dll");
		sqlitePathUpper = exeDir + _T("\\SQLite3.dll");
	}
	DWORD sqliteAttrLower = ::GetFileAttributesW(sqlitePathLower);
	DWORD sqliteAttrUpper = ::GetFileAttributesW(sqlitePathUpper);
	LogMap(L"[Map] sqlite3.dll in exe dir: %s", (sqliteAttrLower != INVALID_FILE_ATTRIBUTES) ? L"yes" : L"no");
	LogMap(L"[Map] SQLite3.dll in exe dir: %s", (sqliteAttrUpper != INVALID_FILE_ATTRIBUTES) ? L"yes" : L"no");
	CString mbtilesError;
	if (!m_mbtilesReader.Open(m_mbtilesPath, mbtilesError))
	{
		LogMap(L"[Map] MBTiles open failed: %s", mbtilesError.GetString());
		LogMap(L"[Map] Attempted path: %s", m_mbtilesPath.GetString());
		// 尝试查找文件（按优先级顺序）
		wchar_t exePath[MAX_PATH] = {};
		::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		CString exeDir(exePath);
		int pos = exeDir.ReverseFind(L'\\');
		if (pos >= 0)
		{
			exeDir = exeDir.Left(pos);
		}
		// 优先级1: maps\OUTPUT_FILE.mbtiles（QGIS 默认输出）
		CString altPath1 = exeDir + _T("\\maps\\OUTPUT_FILE.mbtiles");
		// 优先级2: OUTPUT_FILE.mbtiles（exe 同目录）
		CString altPath2 = exeDir + _T("\\OUTPUT_FILE.mbtiles");
		// 优先级3: maps\osm-2020-02-10-v3.11_china_nanchang.mbtiles（旧文件名）
		CString altPath3 = exeDir + _T("\\maps\\osm-2020-02-10-v3.11_china_nanchang.mbtiles");
		
		DWORD attr1 = ::GetFileAttributesW(altPath1);
		DWORD attr2 = ::GetFileAttributesW(altPath2);
		DWORD attr3 = ::GetFileAttributesW(altPath3);
		LogMap(L"[Map] maps\\OUTPUT_FILE.mbtiles exists: %s", (attr1 != INVALID_FILE_ATTRIBUTES) ? L"yes" : L"no");
		LogMap(L"[Map] OUTPUT_FILE.mbtiles exists: %s", (attr2 != INVALID_FILE_ATTRIBUTES) ? L"yes" : L"no");
		LogMap(L"[Map] Old filename exists: %s", (attr3 != INVALID_FILE_ATTRIBUTES) ? L"yes" : L"no");
		
		if (attr1 != INVALID_FILE_ATTRIBUTES)
		{
			m_mbtilesPath = altPath1;
			if (m_mbtilesReader.Open(m_mbtilesPath, mbtilesError))
			{
				LogMap(L"[Map] Successfully opened maps\\OUTPUT_FILE.mbtiles");
				m_mbtilesReader.GetMetadata(m_mbtilesMetadata);
			}
		}
		else if (attr2 != INVALID_FILE_ATTRIBUTES)
		{
			m_mbtilesPath = altPath2;
			if (m_mbtilesReader.Open(m_mbtilesPath, mbtilesError))
			{
				LogMap(L"[Map] Successfully opened OUTPUT_FILE.mbtiles");
				m_mbtilesReader.GetMetadata(m_mbtilesMetadata);
			}
		}
		else if (attr3 != INVALID_FILE_ATTRIBUTES)
		{
			m_mbtilesPath = altPath3;
			if (m_mbtilesReader.Open(m_mbtilesPath, mbtilesError))
			{
				LogMap(L"[Map] Successfully opened old filename");
				m_mbtilesReader.GetMetadata(m_mbtilesMetadata);
			}
		}
	}
	else
	{
		m_mbtilesReader.GetMetadata(m_mbtilesMetadata);
		LogMap(L"[Map] metadata zoom=%d..%d default=%d center=(%.6f, %.6f) format=%s",
			m_mbtilesMetadata.minZoom,
			m_mbtilesMetadata.maxZoom,
			m_mbtilesMetadata.defaultZoom,
			m_mbtilesMetadata.centerLat,
			m_mbtilesMetadata.centerLng,
			m_mbtilesMetadata.format.GetString());
		
		// 如果 defaultZoom 太低，设置为 minZoom 和 maxZoom 的中间值
		if (m_mbtilesMetadata.defaultZoom < m_mbtilesMetadata.minZoom || 
			m_mbtilesMetadata.defaultZoom > m_mbtilesMetadata.maxZoom ||
			(m_mbtilesMetadata.defaultZoom == 10 && m_mbtilesMetadata.maxZoom > 15))
		{
			// 如果 maxZoom 很高但 defaultZoom 还是默认的 10，设置为更合适的值
			int suggestedZoom = (m_mbtilesMetadata.minZoom + m_mbtilesMetadata.maxZoom) / 2;
			if (suggestedZoom < m_mbtilesMetadata.minZoom)
			{
				suggestedZoom = m_mbtilesMetadata.minZoom;
			}
			else if (suggestedZoom > m_mbtilesMetadata.maxZoom)
			{
				suggestedZoom = m_mbtilesMetadata.maxZoom;
			}
			m_mbtilesMetadata.defaultZoom = suggestedZoom;
			LogMap(L"[Map] Adjusted defaultZoom to %d (midpoint of %d..%d)", 
				suggestedZoom, m_mbtilesMetadata.minZoom, m_mbtilesMetadata.maxZoom);
		}
	}
	
	// 加载局部精细地图
	LoadLocalMaps();
	
	// 合并元数据：maxZoom取所有源的最大值
	if (!m_localMaps.empty())
	{
		int maxZoomFromLocal = m_mbtilesMetadata.maxZoom;
		for (size_t i = 0; i < m_localMaps.size(); ++i)
		{
			if (m_localMaps[i].metadata.maxZoom > maxZoomFromLocal)
			{
				maxZoomFromLocal = m_localMaps[i].metadata.maxZoom;
			}
		}
		if (maxZoomFromLocal > m_mbtilesMetadata.maxZoom)
		{
			LogMap(L"[Map] Merged maxZoom: %d -> %d (from local maps)", 
				m_mbtilesMetadata.maxZoom, maxZoomFromLocal);
			m_mbtilesMetadata.maxZoom = maxZoomFromLocal;
		}
	}
	
#if FW_GCS_WITH_WEBVIEW2
	if (!m_comInitialized)
	{
		HRESULT hrCo = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (SUCCEEDED(hrCo))
		{
			m_comInitialized = true;
		}
		LogMap(L"[Map] CoInitializeEx result: 0x%08X", hrCo);
	}
	InitMapWebView();
#else
	LogMap(L"[Map] WebView2 headers not found; map disabled.");
#endif
	
	// 创建子对话框
	if (!CreateChildDialogs())
	{
		MessageBox(_T("创建子对话框失败！"), _T("错误"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	// 显示第一页
	ShowPage(0);
	
	// 初始化时禁用功能按钮（需要连接后才能使用）
	CWnd* pBtn = GetDlgItem(IDC_BTN_PAGE1);
	if (pBtn != NULL) pBtn->EnableWindow(FALSE);  // 详细自检信息
	pBtn = GetDlgItem(IDC_BTN_PAGE2);
	if (pBtn != NULL) pBtn->EnableWindow(FALSE);  // 地面站指令

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CFWGCSDlgDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
		// 客户区顶部：灰色条带（与菜单栏同色，用于“加高”菜单栏视觉效果）+ 底部分界线
		{
			CClientDC dc(this);
			CRect rc;
			GetClientRect(&rc);
			const COLORREF lineColor = ::GetSysColor(COLOR_3DSHADOW);
			if (m_menuBarExtraHeight > 0)
			{
				dc.FillSolidRect(0, 0, rc.Width(), m_menuBarExtraHeight, RGB(100, 100, 100));
				dc.FillSolidRect(0, m_menuBarExtraHeight, rc.Width(), 1, lineColor);  // 条带与地图之间的分界线
			}
			else
			{
				dc.FillSolidRect(0, 0, rc.Width(), 1, lineColor);
			}
		}
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CFWGCSDlgDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CFWGCSDlgDlg::OnBnClickedUdplink()
{
	if (!m_bUdpConnected)
	{
		// 当前未连接，执行连接操作
		if (ConnectUdp())
		{
			CString strMsg;
			strMsg.Format(_T("UDP连接成功！\n\n本地端口: %d\n远程地址: %s:%d"), 
				m_nUdpLocalPort, m_strUdpRemoteIP, m_nUdpRemotePort);
			MessageBox(strMsg, _T("UDP回报窗口"), MB_OK | MB_ICONINFORMATION);
			
			// 连接成功后启用功能按钮
			CWnd* pBtn = GetDlgItem(IDC_BTN_PAGE1);
			if (pBtn != NULL) pBtn->EnableWindow(TRUE);  // 详细自检信息
			pBtn = GetDlgItem(IDC_BTN_PAGE2);
			if (pBtn != NULL) pBtn->EnableWindow(TRUE);  // 地面站指令
		}
		else
		{
			int nError = WSAGetLastError(); // 获取错误代码
			CString strError; // 错误字符串
			strError.Format(_T("UDP连接失败！\n\n远程地址: %s:%d\n错误代码: %d\n\n可能的原因：\n1. 远程设备(%s)不存在或无法访问\n2. 远程设备未运行或未监听端口%d\n3. 本地端口%d是否被占用\n4. 网络连接问题\n5. 防火墙阻止了连接\n\n请检查网络连接和远程设备状态，查看调试输出获取详细信息"), 
				m_strUdpRemoteIP, m_nUdpRemotePort, nError, m_strUdpRemoteIP, m_nUdpRemotePort, m_nUdpLocalPort); // 格式化错误字符串
			MessageBox(strError, _T("错误"), MB_OK | MB_ICONERROR); // 显示错误消息
		}
	}
	else
	{
		// 当前已连接，执行断开操作
		DisconnectUdp();
		MessageBox(_T("UDP已断开！"), _T("UDP回报窗口"), MB_OK | MB_ICONINFORMATION);
		
		// 断开连接后禁用功能按钮
		CWnd* pBtn = GetDlgItem(IDC_BTN_PAGE1);
		if (pBtn != NULL) pBtn->EnableWindow(FALSE);  // 详细自检信息
		pBtn = GetDlgItem(IDC_BTN_PAGE2);
		if (pBtn != NULL) pBtn->EnableWindow(FALSE);  // 地面站指令
	}
}

void CFWGCSDlgDlg::OnDestroy()
{
	// 断开UDP连接
	DisconnectUdp();
	// 关闭串口
	CloseSerialPort();
	// 销毁子对话框
	DestroyChildDialogs();
	
	// 清理Winsock库
	WSACleanup();

	// 关闭离线地图资源
#if FW_GCS_WITH_WEBVIEW2
	if (m_webViewController)
	{
		m_webViewController->Close();
	}
	m_webView = nullptr;
	m_webViewController = nullptr;
	m_webViewEnvironment = nullptr;
#endif
	m_mbtilesReader.Close();
	// 关闭所有局部地图
	m_localMaps.clear();
	if (m_comInitialized)
	{
		::CoUninitialize();
		m_comInitialized = false;
	}
	
	CDialogEx::OnDestroy();
}

// 初始化UDP Socket
BOOL CFWGCSDlgDlg::InitUdpSocket()
{
	if (m_udpSocket != INVALID_SOCKET)
	{
		closesocket(m_udpSocket);
		m_udpSocket = INVALID_SOCKET;
	}

	// 创建UDP Socket
	m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_udpSocket == INVALID_SOCKET)
	{
		int nError = WSAGetLastError();
		TRACE(_T("UDP Socket创建失败，错误代码: %d\n"), nError);
		return FALSE;
	}

	// 设置Socket为非阻塞模式，避免sendto阻塞UI线程
	u_long mode = 1;  // 1 = 非阻塞模式，0 = 阻塞模式
	if (ioctlsocket(m_udpSocket, FIONBIO, &mode) == SOCKET_ERROR)
	{
		int nError = WSAGetLastError();
		TRACE(_T("UDP Socket设置非阻塞模式失败，错误代码: %d\n"), nError);
		// 继续执行，但sendto可能会阻塞
	}

	// 绑定本地端口（用于接收数据）
	sockaddr_in localAddr;
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	// 根据本机IP选择绑定网口：0.0.0.0 表示监听所有网口
	if (m_strUdpLocalIP.CompareNoCase(_T("0.0.0.0")) == 0 || m_strUdpLocalIP.IsEmpty())
	{
		localAddr.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		CT2A localIpA(m_strUdpLocalIP, CP_ACP);
		if (inet_pton(AF_INET, localIpA, &localAddr.sin_addr) != 1)
		{
			TRACE(_T("UDP端口绑定失败: 本机IP地址转换失败 (%s)\n"), m_strUdpLocalIP);
			return FALSE;
		}
	}
	localAddr.sin_port = htons(m_nUdpLocalPort);  // 使用配置的本地端口
	
	if (bind(m_udpSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
	{
		int nError = WSAGetLastError();
		TRACE(_T("UDP端口绑定失败 (端口%d)，错误代码: %d\n"), m_nUdpLocalPort, nError);
		// 如果端口被占用，尝试不绑定（UDP可以发送但不一定能接收）
		// 这里不返回错误，继续执行，但接收可能失败
	}
	else
	{
		TRACE(_T("UDP Socket初始化成功，本地端口: %d\n"), m_nUdpLocalPort);
	}

	return TRUE;
}

// 连接UDP并握手
BOOL CFWGCSDlgDlg::ConnectUdp()
{
	if (m_bUdpConnected)
	{
		return TRUE;  // 已经连接
	}

	// 初始化Socket（如果还未初始化）
	if (m_udpSocket == INVALID_SOCKET)
	{
		if (!InitUdpSocket())
		{
			TRACE(_T("UDP连接失败: Socket初始化失败\n"));
			return FALSE;
		}
	}

	// 设置远程地址（使用配置的远程IP和端口）
	memset(&m_udpRemoteAddr, 0, sizeof(m_udpRemoteAddr));
	m_udpRemoteAddr.sin_family = AF_INET;
	m_udpRemoteAddr.sin_port = htons(m_nUdpRemotePort);
	
	// 将CString转换为ANSI字符串
	CT2A remoteIpA(m_strUdpRemoteIP, CP_ACP);
	
	// 使用 inet_pton() 替代已弃用的 inet_addr()
	if (inet_pton(AF_INET, remoteIpA, &m_udpRemoteAddr.sin_addr) != 1)
	{
		// IP地址转换失败
		TRACE(_T("UDP连接失败: IP地址转换失败 (%s)\n"), m_strUdpRemoteIP);
		return FALSE;
	}

	// 重置响应标志
	{
		CSingleLock lock(&m_csUdpResponse);
		lock.Lock();
		m_bUdpRemoteResponded = FALSE;
		lock.Unlock();
	}

	// 启动接收线程（先启动线程，再发送握手）
	m_bUdpThreadRunning = TRUE; // 设置线程运行标志
	m_pUdpRecvThread = AfxBeginThread(UdpRecvThread, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED); // 创建UDP接收线程
	if (m_pUdpRecvThread == NULL) // 如果接收线程创建失败
	{
		m_bUdpThreadRunning = FALSE;
		TRACE(_T("UDP连接失败: 接收线程创建失败\n"));
		return FALSE;
	}
	m_pUdpRecvThread->ResumeThread();

	// 发送握手数据包
	if (!SendHandshake())
	{
		TRACE(_T("UDP连接失败: 握手包发送失败\n"));
		m_bUdpThreadRunning = FALSE;
		if (m_udpSocket != INVALID_SOCKET)
		{
			closesocket(m_udpSocket);
			m_udpSocket = INVALID_SOCKET;
		}
		return FALSE;
	}

	// 等待远程地址响应（最多等待5秒，给Simulink更多时间发送数据）
	// 使用消息泵保持UI响应，避免阻塞
	BOOL bReceived = FALSE;
	const int nWaitTimeMs = 5000;  // 等待5秒，给Simulink更多时间
	const int nCheckIntervalMs = 50;  // 每50ms检查一次
	DWORD dwStartTime = GetTickCount64();
	int nCheckCount = 0;

	TRACE(_T("ConnectUdp: 开始等待远程地址响应，最多等待 %d 毫秒\n"), nWaitTimeMs);

	while ((GetTickCount64() - dwStartTime) < nWaitTimeMs)
	{
		// 处理Windows消息，保持UI响应
		// 每次处理一条消息后立即检查响应标志，避免在处理大量消息时延迟检查
		MSG msg;
		BOOL bProcessedMsg = FALSE;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			bProcessedMsg = TRUE;
			
			// 处理完每条消息后立即检查响应标志
			// 这样可以快速响应，避免在处理大量消息时延迟
			CSingleLock lock(&m_csUdpResponse);
			lock.Lock();
			if (m_bUdpRemoteResponded)
			{
				bReceived = TRUE;
				DWORD dwElapsed = GetTickCount64() - dwStartTime;
				TRACE(_T("ConnectUdp: 检测到响应标志已设置（在处理消息后），耗时 %d 毫秒\n"), dwElapsed);
				lock.Unlock();
				break;
			}
			lock.Unlock();
		}
		
		// 如果已经收到响应，退出循环
		if (bReceived)
		{
			break;
		}

		// 如果没有处理消息，也检查一次响应标志（防止遗漏）
		if (!bProcessedMsg)
		{
			CSingleLock lock(&m_csUdpResponse);
			lock.Lock();
			if (m_bUdpRemoteResponded)
			{
				bReceived = TRUE;
				DWORD dwElapsed = GetTickCount64() - dwStartTime;
				TRACE(_T("ConnectUdp: 检测到响应标志已设置，耗时 %d 毫秒\n"), dwElapsed);
				lock.Unlock();
				break;
			}
			lock.Unlock();
		}

		// 每1秒输出一次调试信息
		nCheckCount++;
		if (nCheckCount % 20 == 0)  // 每20次检查（约1秒）输出一次
		{
			DWORD dwElapsed = GetTickCount64() - dwStartTime;
			TRACE(_T("ConnectUdp: 等待中... 已等待 %d 毫秒，响应标志=%d\n"), 
				dwElapsed, m_bUdpRemoteResponded);
		}

		// 短暂休眠，避免CPU占用过高
		Sleep(nCheckIntervalMs);
	}

	if (!bReceived)
	{
		// 超时未收到响应，连接失败
		DWORD dwElapsed = GetTickCount64() - dwStartTime;
		TRACE(_T("UDP连接失败: 等待远程地址响应超时 (%s:%d)，已等待 %d 毫秒，响应标志=%d\n"), 
			UDP_REMOTE_IP, UDP_REMOTE_PORT, dwElapsed, m_bUdpRemoteResponded);
		m_bUdpThreadRunning = FALSE;
		if (m_udpSocket != INVALID_SOCKET)
		{
			closesocket(m_udpSocket);
			m_udpSocket = INVALID_SOCKET;
		}
		return FALSE;
	}

	// 设置连接标志（只有在收到响应后才设置）
	m_bUdpConnected = TRUE;
	TRACE(_T("UDP连接成功: 已收到远程地址响应 (%s:%d)\n"), UDP_REMOTE_IP, UDP_REMOTE_PORT);

	return TRUE;
}

// 断开UDP连接
void CFWGCSDlgDlg::DisconnectUdp()
{
	if (!m_bUdpConnected)
	{
		return;
	}

	// 停止接收线程
	m_bUdpThreadRunning = FALSE;
	
	// 关闭Socket，使recvfrom()返回错误，线程退出
	if (m_udpSocket != INVALID_SOCKET)
	{
		closesocket(m_udpSocket);
		m_udpSocket = INVALID_SOCKET;
	}

	// 等待线程结束
	if (m_pUdpRecvThread != NULL)
	{
		WaitForSingleObject(m_pUdpRecvThread->m_hThread, 3000);  // 等待最多3秒
		m_pUdpRecvThread = NULL;
	}

	m_bUdpConnected = FALSE;
}

// 发送握手数据包
BOOL CFWGCSDlgDlg::SendHandshake()
{
	UdpHandshakePacket handshake;	//
	memcpy(handshake.magic, "GCS", 3); // 将"GCS"复制到handshake.magic中
	handshake.magic[3] = '\0';
	handshake.version = 1;

	return SendUdpData(&handshake, sizeof(handshake));
}

// 发送UDP数据
BOOL CFWGCSDlgDlg::SendUdpData(const void* pData, int nSize)
{
	if (m_udpSocket == INVALID_SOCKET)
	{
		TRACE(_T("SendUdpData: Socket无效\n"));
		return FALSE;
	}
	
	// 发送数据到远程地址，函数参数依序为：Socket句柄、数据、数据长度、标志、远程地址、远程地址长度。返回值为发送的字节数，如果发送失败返回SOCKET_ERROR。
	int nSent = sendto(m_udpSocket, (const char*)pData, nSize, 0, 
		(sockaddr*)&m_udpRemoteAddr, sizeof(m_udpRemoteAddr)); 

	if (nSent == SOCKET_ERROR)
	{
		int nError = WSAGetLastError();
		// 在非阻塞模式下，如果发送缓冲区满，会返回WSAEWOULDBLOCK
		// 这种情况下，数据可能已经部分发送，但通常UDP不会出现这种情况
		if (nError == WSAEWOULDBLOCK)
		{
			TRACE(_T("SendUdpData: 发送缓冲区满，数据未发送\n"));
		}
		else
		{
			TRACE(_T("SendUdpData: 发送失败，错误代码: %d\n"), nError);
		}
		return FALSE;
	}

	if (nSent != nSize)
	{
		TRACE(_T("SendUdpData: 部分发送，期望 %d 字节，实际发送 %d 字节\n"), nSize, nSent);
		return FALSE;
	}

	return TRUE;
}

// UDP接收线程函数
UINT CFWGCSDlgDlg::UdpRecvThread(LPVOID pParam)
{
	CFWGCSDlgDlg* pDlg = (CFWGCSDlgDlg*)pParam;
	char buffer[1024];
	sockaddr_in fromAddr;
	int nFromLen = sizeof(fromAddr);

	while (pDlg->m_bUdpThreadRunning)//判断UDP线程是否运行
	{
		// 阻塞等待接收数据
		int nReceived = recvfrom(pDlg->m_udpSocket, buffer, sizeof(buffer), 0,
			(sockaddr*)&fromAddr, &nFromLen);

		if (nReceived > 0)
		{
			// 使用 inet_ntop() 替代已弃用的 inet_ntoa()
			char szIpAddr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &fromAddr.sin_addr, szIpAddr, INET_ADDRSTRLEN);
			//TRACE(_T("UDP接收线程: 收到 %d 字节数据，来源: %s:%d\n"), 
				//nReceived, CString(szIpAddr), ntohs(fromAddr.sin_port));
			
			// UDP是无连接协议，只要收到任何数据包就认为连接成功
			// 如果还没有设置响应标志，则设置它（用于连接验证）
			CSingleLock lock(&pDlg->m_csUdpResponse);
			lock.Lock();
			BOOL bWasSet = pDlg->m_bUdpRemoteResponded;
			if (!pDlg->m_bUdpRemoteResponded)
			{
				pDlg->m_bUdpRemoteResponded = TRUE;
				TRACE(_T("UDP接收线程: [首次]检测到数据接收，设置响应标志=TRUE（来源: %s:%d）\n"), 
					CString(szIpAddr), ntohs(fromAddr.sin_port));
			}
			else
			{
				//TRACE(_T("UDP接收线程: 收到数据，但响应标志已设置（来源: %s:%d）\n"), 
					//CString(szIpAddr), ntohs(fromAddr.sin_port));
			}
			lock.Unlock();
			
			// 如果数据包来自配置的远程地址，更新远程地址信息（用于后续发送）
			if (fromAddr.sin_addr.s_addr == pDlg->m_udpRemoteAddr.sin_addr.s_addr &&
				fromAddr.sin_port == pDlg->m_udpRemoteAddr.sin_port)
			{
				//TRACE(_T("UDP接收线程: 数据包来自配置的远程地址\n"));
			}
			else
			{
				// 如果数据包来自其他地址，更新远程地址（适应动态IP场景）
				TRACE(_T("UDP接收线程: 数据包来自新地址，更新远程地址信息\n"));
				pDlg->m_udpRemoteAddr.sin_addr.s_addr = fromAddr.sin_addr.s_addr;
				pDlg->m_udpRemoteAddr.sin_port = fromAddr.sin_port;
			}
			
			// 检查数据包大小是否匹配
			//TRACE(_T("UDP接收: 数据包大小检查 - 期望=%d字节, 实际收到=%d字节\n"), 
				//sizeof(UdpRecvDataPacket), nReceived);
			
			if (nReceived == sizeof(UdpRecvDataPacket))
			{
				// 动态分配内存保存数据包，注意堆栈释放
				UdpRecvDataPacket* pPacket = new UdpRecvDataPacket;
				memcpy(pPacket, buffer, sizeof(UdpRecvDataPacket));
				
				// 字节序转换（UDP网络数据通常是大端字节序，需要转换）
				// Windows是小端系统，如果发送端也是小端，则不需要转换
				// 如果发送端是大端，需要取消下面注释来启用字节序转换
				// pPacket->pitchAngle = ntohs(pPacket->pitchAngle);
				// pPacket->rollAngle = ntohs(pPacket->rollAngle);
				// pPacket->yawAngle = ntohs(pPacket->yawAngle);
				// pPacket->attackAngle = ntohs(pPacket->attackAngle);
				// pPacket->sideslipAngle = ntohs(pPacket->sideslipAngle);
				
				// 调试测试输出：显示接收到的前部分数据（原始int16_t值）
				//TRACE(_T("UDP接收: pitchAngle=%d, rollAngle=%d, yawAngle=%d, attackAngle=%d, sideslipAngle=%d\n"),
				//	pPacket->pitchAngle, pPacket->rollAngle, pPacket->yawAngle, pPacket->attackAngle, pPacket->sideslipAngle);
				
				// 调试输出：显示原始字节值（用于诊断字节序问题）
				BYTE* pBytes = (BYTE*)pPacket;
				//TRACE(_T("UDP接收原始字节[前10字节]: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n"),
				//	pBytes[0], pBytes[1], pBytes[2], pBytes[3], pBytes[4], 
				//	pBytes[5], pBytes[6], pBytes[7], pBytes[8], pBytes[9]);
				
				// 调试输出：显示结构体大小和字段偏移（用于诊断）
				size_t longitudeOffset = (BYTE*)&(pPacket->longitude) - (BYTE*)pPacket;
				size_t latitudeOffset = (BYTE*)&(pPacket->latitude) - (BYTE*)pPacket;
				size_t gpsCourseOffset = (BYTE*)&(pPacket->gpsCourse) - (BYTE*)pPacket;
				//TRACE(_T("UDP接收[字段偏移]: longitude偏移=%d, latitude偏移=%d, gpsCourse偏移=%d\n"),
				//	longitudeOffset, latitudeOffset, gpsCourseOffset);
				
				// 调试输出：检查数据包中longitude和latitude位置前后的字节（用于诊断数据包结构是否匹配）
				// TRACE(_T("UDP接收[数据包上下文]: longitude位置前后字节: [偏移%d-%d] %02X %02X %02X %02X | [偏移%d-%d] %02X %02X %02X %02X | [偏移%d-%d] %02X %02X %02X %02X\n"),
					// longitudeOffset-4, longitudeOffset-1,
					// pBytes[longitudeOffset-4], pBytes[longitudeOffset-3], pBytes[longitudeOffset-2], pBytes[longitudeOffset-1],
					// longitudeOffset, longitudeOffset+3,
					// pBytes[longitudeOffset], pBytes[longitudeOffset+1], pBytes[longitudeOffset+2], pBytes[longitudeOffset+3],
					// longitudeOffset+4, longitudeOffset+7,
					// pBytes[longitudeOffset+4], pBytes[longitudeOffset+5], pBytes[longitudeOffset+6], pBytes[longitudeOffset+7]);
				
				// 调试输出：显示经纬度原始值（用于诊断）
				// TRACE(_T("UDP接收[经纬度原始值]: longitude=%d (0x%08X), latitude=%d (0x%08X), gpsCourse=%d\n"),
				// 	pPacket->longitude, (unsigned int)pPacket->longitude,
				// 	pPacket->latitude, (unsigned int)pPacket->latitude,
				// 	pPacket->gpsCourse);
				
				// 调试输出：显示longitude和latitude的原始字节值（用于诊断字节序问题）
				//TRACE(_T("UDP接收[经纬度字节]: longitude偏移=%d, 字节值: %02X %02X %02X %02X (小端解析=%d)\n"),
					//longitudeOffset, pBytes[longitudeOffset], pBytes[longitudeOffset+1], 
					//pBytes[longitudeOffset+2], pBytes[longitudeOffset+3], pPacket->longitude);
				//TRACE(_T("UDP接收[经纬度字节]: latitude偏移=%d, 字节值: %02X %02X %02X %02X (小端解析=%d)\n"),
				//	latitudeOffset, pBytes[latitudeOffset], pBytes[latitudeOffset+1], 
				//	pBytes[latitudeOffset+2], pBytes[latitudeOffset+3], pPacket->latitude);
				
				// 调试输出：手动解析大端字节序（仅发送端是大端时适用）
				// 注意：先按无符号解析，避免符号扩展问题
				// uint32_t longitudeBigEndianU = (uint32_t)((pBytes[longitudeOffset] << 24) | 
				// 	(pBytes[longitudeOffset+1] << 16) | 
				// 	(pBytes[longitudeOffset+2] << 8) | 
				// 	pBytes[longitudeOffset+3]);
				// uint32_t latitudeBigEndianU = (uint32_t)((pBytes[latitudeOffset] << 24) | 
				// 	(pBytes[latitudeOffset+1] << 16) | 
				// 	(pBytes[latitudeOffset+2] << 8) | 
				// 	pBytes[latitudeOffset+3]);
				// int32_t longitudeBigEndian = (int32_t)longitudeBigEndianU;
				// int32_t latitudeBigEndian = (int32_t)latitudeBigEndianU;
				// TRACE(_T("UDP接收[大端解析测试]: longitude=%d (0x%08X), latitude=%d (0x%08X) (若此值看起来正确，启用字节序转换来解包)\n"),
				// 	longitudeBigEndian, longitudeBigEndianU, latitudeBigEndian, latitudeBigEndianU);
				
				// 调试输出：计算期望值范围（用于判断数据是否正确）
				// 例如：116.123度 × 100000 = 11612300
				//TRACE(_T("UDP接收[数据验证]: 当前小端值(longitude=%d, latitude=%d)\n"),
					//pPacket->longitude, pPacket->latitude);
				
				// 发送消息到主线程处理
				pDlg->PostMessage(WM_UDP_DATA_RECEIVED, (WPARAM)pPacket, 0);
			}
			else
			{
				//TRACE(_T("UDP接收: 数据包大小不匹配！期望 %d 字节，实际收到 %d 字节\n"), 
					//sizeof(UdpRecvDataPacket), nReceived);
			}
		}
		else if (nReceived == SOCKET_ERROR)
		{
			// Socket错误，可能是Socket已关闭
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				break;  // 退出线程
			}
		}
	}

	return 0;
}

// 处理接收到的UDP数据消息
LRESULT CFWGCSDlgDlg::OnUdpDataReceivedMsg(WPARAM wParam, LPARAM lParam)
{
	UdpRecvDataPacket* pPacket = (UdpRecvDataPacket*)wParam;
	if (pPacket != NULL)
	{
		// ============================================================
		// 除消息队列中的旧消息（避免UI频繁刷新和消息积压）
		// ============================================================
		// 原因：如果接收速度很快，消息队列可能积压多个数据包
		// 只处理最新的数据包，避免UI频繁刷新和内存泄漏
		MSG msg;
		while (PeekMessage(&msg, m_hWnd, WM_UDP_DATA_RECEIVED, WM_UDP_DATA_RECEIVED, PM_REMOVE))
		{
			// ============================================================
			// 释放旧消息中的数据包内存
			// ============================================================
			// 注意：当前消息（wParam）的数据包不要在这里删除，后面还要使用
			if (msg.wParam != NULL && msg.wParam != wParam)
			{
				UdpRecvDataPacket* pOldPacket = (UdpRecvDataPacket*)msg.wParam;
				delete pOldPacket;  // 释放旧数据包内存
			}
		}

		// ============================================================
		// 处理接收到的数据包（更新UI显示）
		// ============================================================
		// 限制UI刷新频率，避免高频数据导致UI卡顿（例如拖动窗口困难）
		const DWORD kUiUpdateIntervalMs = UI_UPDATE_INTERVAL_MS;
		DWORD dwNow = GetTickCount64();
		if (m_dwLastUdpUiUpdate != 0 && (dwNow - m_dwLastUdpUiUpdate) < kUiUpdateIntervalMs)
		{
			// 过于频繁，丢弃本次数据包以保护GUI响应
			delete pPacket;
			return 0;
		}
		m_dwLastUdpUiUpdate = dwNow;

		ProcessReceivedData(pPacket);
		
		// ============================================================
		// 释放当前数据包内存
		// ============================================================
		delete pPacket;
	}
	return 0;
}

// 将UDP接收的数据推送至webview前端引擎，用于JS图层绘制
void CFWGCSDlgDlg::SendHudMessage(const UdpRecvDataPacket* pPacket)
{
#if FW_GCS_WITH_WEBVIEW2
	if (!pPacket || !m_webView)
	{
		return;
	}

	//根据HUD以及其他js图层显示需求提取数据并转换类型
	//主HUD部分
	const float pitch = pPacket->pitchAngle / 10.0f;
	const float roll  = pPacket->rollAngle / 10.0f;
	const float yaw   = pPacket->yawAngle / 10.0f;
	const float ias   = pPacket->indicatedAirspeed / 10.0f;
	const float tas   = pPacket->baroAirspeed / 10.0f;
	const float alt   = pPacket->baroAltitude / 10.0f;
	const float mach  = pPacket->machNumber / 10.0f;        // 马赫数
	const float aoa   = pPacket->attackAngle / 10.0f;       // 攻角
	const float g     = pPacket->normalOverload / 10.0f;    // 法向过载
	const float rpm   = pPacket->engineRPM / 10.0f;         // 发动机转速
    //地图飞机标识部分
	const float longitude = pPacket->longitude / 1000000.0f; 
    const float latitude = pPacket->latitude / 1000000.0f;
	//底部信息栏部分
    const float gpsCourse = pPacket->gpsCourse / 10.0f;
	const float gpsGroundSpeed = pPacket->gpsGroundSpeed / 10.0f;
    const float gpsVerticalSpeed = pPacket->gpsVerticalSpeed / 10.0f;
    const uint8_t gpsHour = pPacket->gpsHour;
    const uint8_t gpsMinute = pPacket->gpsMinute;
    const uint8_t gpsSecond = pPacket->gpsSecond;
    //地图目标机标识部分
    const float targetLongitude = pPacket->targetLongitude / 1000000.0f;
    const float targetLatitude = pPacket->targetLatitude / 1000000.0f;
    const float targetCourse = pPacket->targetCourse / 10.0f;
    
    // HUD组2数据 - 第一批：添加第一行数据 (1-1, 1-2, 2-1, 2-2)
    const uint8_t throttle = pPacket->throttle;                    // (1-1) 油门控制
    const uint8_t batteryVoltage = pPacket->batteryVoltage;       // (1-2) 电池电压
    const uint8_t fuelRemaining = pPacket->fuelRemaining;          // (2-1) 剩余油量
    const float engineTemp = pPacket->engineTemp / 10.0f;          // (2-2) 发动机缸温
    // HUD组2数据 - 第二批：添加第二行数据 (3-1, 3-2, 4-1, 4-2)
    const uint8_t satelitesNum = pPacket->satelitesNum;            // (3-1) 卫星收星数
    const uint8_t gpsStatus = pPacket->gpsStatus;                   // (3-2) 卫星定位状态
    const uint8_t navStatus = pPacket->navStatus;                   // (4-1) 导航状态
    const uint8_t targetWaypoint = pPacket->targetWaypoint;         // (4-2) 目标航点
    // HUD组2数据 - 第三批：添加第三行数据 (5-1, 5-2, 6-1, 6-2)
    const float distanceToGo = pPacket->distanceToGo / 10.0f;       // (5-1) 待飞距
    const float crossTrackError = pPacket->crossTrackError / 10.0f; // (5-2) 偏航距
    const uint8_t commandHeading = pPacket->commandHeading;        // (6-1) 应飞航向
    const float courseDeviation = pPacket->courseDeviation / 10.0f; // (6-2) 偏航角
    // HUD组2数据 - 第四批：添加剩余数据 (7-1, 7-2, 8-1, 8-2, 9-1, 9-2)
    const float commandSpeed = pPacket->commandSpeed / 10.0f;       // (7-1) 应飞速度
    const float commandAltitude = pPacket->commandAltitude / 10.0f; // (7-2) 应飞高度
    const uint8_t commandTime = pPacket->commandTime;               // (8-1) 应飞时间
    const uint8_t payloadType = pPacket->payloadType;               // (8-2) 载荷类型
    const uint8_t ammoRemaining = pPacket->ammoRemaining;          // (9-1) 剩余弹量
    const uint8_t selfTestResult = pPacket->selfTestResult;        // (9-2) 自检结果
    
    // HUD组3数据（开关状态显示区域）
    const uint8_t switchStatus_B1 = pPacket->switchStatus_B1;      // (1-1) 发动机启动状态
    const uint8_t switchStatus_B4 = pPacket->switchStatus_B4;      // (1-2) 关车状态
    const uint8_t switchStatus_B2 = pPacket->switchStatus_B2;      // (2-1) 盘旋状态
    const uint8_t switchStatus_B3 = pPacket->switchStatus_B3;      // (2-2) 归航状态
    const uint8_t switchStatus_B0 = pPacket->switchStatus_B0;      // (3-1) 发动机并网状态
    const uint8_t switchStatus_B6 = pPacket->switchStatus_B6;      // (3-2) 开伞状态
    const uint8_t switchStatus_B5 = pPacket->switchStatus_B5;      // (4-1) 起落架收放状态
    const uint8_t switchStatus_B7 = pPacket->switchStatus_B7;      // (4-2) 夜航灯开关状态
    
    // 调试输出：显示经纬度原始值和转换后的值（用于诊断）
    // TRACE(_T("SendHudMessage[经纬度调试]: longitude原始=%d, 转换后=%.6f度; latitude原始=%d, 转换后=%.6f度; gpsCourse原始=%d, 转换后=%.2f度\n"),
        // pPacket->longitude, longitude, pPacket->latitude, latitude, pPacket->gpsCourse, gpsCourse);
    // TRACE(_T("SendHudMessage[数据验证]: 如果原始数据是'经纬度×1e5'，期望值应该在±18000000范围内\n"));
    // TRACE(_T("SendHudMessage[数据验证]: 当前值(longitude=%d, latitude=%d)太小，请检查发送端是否正确发送了'经纬度×1e5'的值\n"),
        // pPacket->longitude, pPacket->latitude);

 // 将数据添加到JSON格式字符串中
	CStringA json;
	json.Format(R"({"pitch":%.3f,"roll":%.3f,"yaw":%.3f,"ias":%.3f,"tas":%.3f,"alt":%.3f,"mach":%.3f,"aoa":%.3f,"g":%.3f,"rpm":%.1f,"longitude":%.6f,"latitude":%.6f,"gpsCourse":%.2f,"gpsGroundSpeed":%.1f,"gpsVerticalSpeed":%.1f,"gpsHour":%u,"gpsMinute":%u,"gpsSecond":%u,"targetLongitude":%.6f,"targetLatitude":%.6f,"targetCourse":%.2f,"alarmStatus_B0":%u,"alarmStatus_B1":%u,"alarmStatus_B2":%u,"alarmStatus_B3":%u,"alarmStatus_B4":%u,"alarmStatus_B5":%u,"throttle":%u,"batteryVoltage":%u,"fuelRemaining":%u,"engineTemp":%.1f,"satelitesNum":%u,"gpsStatus":%u,"navStatus":%u,"targetWaypoint":%u,"distanceToGo":%.1f,"crossTrackError":%.1f,"commandHeading":%u,"courseDeviation":%.1f,"commandSpeed":%.1f,"commandAltitude":%.1f,"commandTime":%u,"payloadType":%u,"ammoRemaining":%u,"selfTestResult":%u,"switchStatus_B1":%u,"switchStatus_B4":%u,"switchStatus_B2":%u,"switchStatus_B3":%u,"switchStatus_B0":%u,"switchStatus_B6":%u,"switchStatus_B5":%u,"switchStatus_B7":%u})",
		pitch, roll, yaw, ias, tas, alt, mach, aoa, g, rpm, longitude, latitude, gpsCourse, 
		gpsGroundSpeed, gpsVerticalSpeed, gpsHour, gpsMinute, gpsSecond,
		targetLongitude, targetLatitude, targetCourse,
		pPacket->alarmStatus_B0, pPacket->alarmStatus_B1, pPacket->alarmStatus_B2,
		pPacket->alarmStatus_B3, pPacket->alarmStatus_B4, pPacket->alarmStatus_B5,
		throttle, batteryVoltage, fuelRemaining, engineTemp, satelitesNum, gpsStatus, navStatus, targetWaypoint,
		distanceToGo, crossTrackError, commandHeading, courseDeviation,
		commandSpeed, commandAltitude, commandTime, payloadType, ammoRemaining, selfTestResult,
		switchStatus_B1, switchStatus_B4, switchStatus_B2, switchStatus_B3, switchStatus_B0,
		switchStatus_B6, switchStatus_B5, switchStatus_B7);

	std::wstring jsonW(CA2W(json.GetString()));
	m_webView->PostWebMessageAsJson(jsonW.c_str());
#endif
}

// 处理接收到的数据包
void CFWGCSDlgDlg::ProcessReceivedData(const UdpRecvDataPacket* pPacket)
{
	if (pPacket == NULL)
	{
		TRACE(_T("ProcessReceivedData: 数据包指针为空！\n"));
		return;
	}

	// 直接推送 HUD 数据（已移除传统控件/子对话框显示）
	SendHudMessage(pPacket);
}

// ============================================================================
// 串口连接按钮事件处理函数（RS422串口）
// ============================================================================
// 功能：切换串口连接状态
//   - 未连接时：打开串口，启动接收线程
//   - 已连接时：停止接收线程，关闭串口
// ============================================================================
void CFWGCSDlgDlg::OnBnClickedSeriallink()
{
	if (!m_bSerialConnected)
	{
		// ============================================================
		// 当前未连接，执行连接操作
		// ============================================================
		if (OpenSerialPort())
		{
			// 连接成功，显示成功消息
			CString strMsg;
			strMsg.Format(_T("串口连接成功！\n\n串口: %s\n波特率: %d"), 
				_T(SERIAL_PORT_NAME), SERIAL_BAUD_RATE);
			MessageBox(strMsg, _T("串口回报窗口"), MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			// 连接失败，显示错误信息
			int nError = GetLastError();
			CString strError;
			strError.Format(_T("串口连接失败！\n\n错误代码: %d\n\n请检查：\n1. 串口%s是否存在\n2. 串口是否被其他程序占用\n3. 查看调试输出获取详细信息"), 
				nError, _T(SERIAL_PORT_NAME));
			MessageBox(strError, _T("错误"), MB_OK | MB_ICONERROR);
		}
	}
	else
	{
		// ============================================================
		// 当前已连接，执行断开操作
		// ============================================================
		CloseSerialPort();
		MessageBox(_T("串口已断开！"), _T("串口回报窗口"), MB_OK | MB_ICONINFORMATION);
	}
}

// ============================================================================
// 打开串口函数
// ============================================================================
// 功能：打开RS422串口并配置参数，启动接收线程
// 返回值：
//   - TRUE：串口打开成功
//   - FALSE：串口打开失败（已记录错误信息）
// 步骤：
//   1. 检查是否已连接
//   2. 打开串口设备（使用Windows API CreateFile）
//   3. 配置串口参数（波特率、数据位、校验位、停止位等）
//   4. 设置读写超时参数
//   5. 创建并启动串口接收线程
// ============================================================================
BOOL CFWGCSDlgDlg::OpenSerialPort()
{
	// ============================================================
	// 步骤1：检查连接状态
	// ============================================================
	if (m_bSerialConnected)
	{
		return TRUE;  // 已经连接，直接返回成功
	}

	// ============================================================
	// 步骤2：打开串口设备
	// ============================================================
	// Windows串口名称格式：COM1-COM9 使用 "COMx"，COM10及以上使用 "\\\\.\\COMx"
	// 为兼容性，统一使用 "\\\\.\\COMx" 格式
	CString strPortPath;
	strPortPath.Format(_T("\\\\.\\%s"), _T(SERIAL_PORT_NAME));

	// 打开串口（读写模式，独占访问）
	m_hSerialPort = CreateFile(
		strPortPath,                           // 串口路径
		GENERIC_READ | GENERIC_WRITE,          // 读写权限
		0,                                     // 共享模式：独占访问
		NULL,                                  // 安全属性：默认
		OPEN_EXISTING,                         // 打开已存在的设备
		FILE_ATTRIBUTE_NORMAL,                 // 文件属性：普通
		NULL                                   // 模板文件句柄：无
	);

	if (m_hSerialPort == INVALID_HANDLE_VALUE)
	{
		// 打开失败，记录错误并返回
		int nError = GetLastError();
		TRACE(_T("串口打开失败 (%s)，错误代码: %d\n"), _T(SERIAL_PORT_NAME), nError);
		return FALSE;
	}

	// ============================================================
	// 步骤3：配置串口参数（DCB结构）
	// ============================================================
	DCB dcb = {0};
	dcb.DCBlength = sizeof(DCB);
	
	// 获取当前串口配置
	if (!GetCommState(m_hSerialPort, &dcb))
	{
		TRACE(_T("串口获取状态失败，错误代码: %d\n"), GetLastError());
		CloseHandle(m_hSerialPort);
		m_hSerialPort = INVALID_HANDLE_VALUE;
		return FALSE;
	}

	// 配置基本参数
	dcb.BaudRate = SERIAL_BAUD_RATE;          // 波特率：115200
	dcb.ByteSize = 8;                          // 数据位：8位
	dcb.Parity = NOPARITY;                     // 校验位：无校验
	dcb.StopBits = ONESTOPBIT;                 // 停止位：1位
	
	// 配置流控制参数（RS422通常不使用硬件流控制）
	dcb.fBinary = TRUE;                        // 二进制模式
	dcb.fParity = FALSE;                       // 不使用校验
	dcb.fOutxCtsFlow = FALSE;                  // 输出CTS流控制：禁用
	dcb.fOutxDsrFlow = FALSE;                  // 输出DSR流控制：禁用
	dcb.fDtrControl = DTR_CONTROL_DISABLE;     // DTR控制：禁用
	dcb.fDsrSensitivity = FALSE;               // DSR敏感度：禁用
	dcb.fTXContinueOnXoff = FALSE;             // XOFF后继续发送：禁用
	dcb.fOutX = FALSE;                         // 输出XON/XOFF流控制：禁用
	dcb.fInX = FALSE;                          // 输入XON/XOFF流控制：禁用
	dcb.fErrorChar = FALSE;                    // 错误字符替换：禁用
	dcb.fNull = FALSE;                         // 丢弃NULL字符：禁用
	dcb.fRtsControl = RTS_CONTROL_DISABLE;     // RTS控制：禁用
	dcb.fAbortOnError = FALSE;                 // 错误时中止：禁用

	// 应用串口配置
	if (!SetCommState(m_hSerialPort, &dcb))
	{
		TRACE(_T("串口设置状态失败，错误代码: %d\n"), GetLastError());
		CloseHandle(m_hSerialPort);
		m_hSerialPort = INVALID_HANDLE_VALUE;
		return FALSE;
	}

	// ============================================================
	// 步骤4：设置读写超时参数
	// ============================================================
	COMMTIMEOUTS timeouts = {0};
	timeouts.ReadIntervalTimeout = 50;                    // 字符间超时：50ms
	timeouts.ReadTotalTimeoutConstant = 50;              // 读取总超时常数：50ms
	timeouts.ReadTotalTimeoutMultiplier = 10;            // 读取总超时倍数：每字节10ms
	timeouts.WriteTotalTimeoutConstant = 50;              // 写入总超时常数：50ms
	timeouts.WriteTotalTimeoutMultiplier = 10;           // 写入总超时倍数：每字节10ms
	
	if (!SetCommTimeouts(m_hSerialPort, &timeouts))
	{
		TRACE(_T("串口设置超时失败，错误代码: %d\n"), GetLastError());
		CloseHandle(m_hSerialPort);
		m_hSerialPort = INVALID_HANDLE_VALUE;
		return FALSE;
	}

	// ============================================================
	// 步骤5：创建并启动串口接收线程
	// ============================================================
	m_bSerialThreadRunning = TRUE;  // 设置线程运行标志
	
	// 创建接收线程（挂起状态）
	m_pSerialRecvThread = AfxBeginThread(
		SerialRecvThread,           // 线程函数
		this,                        // 线程参数（传递对话框指针）
		THREAD_PRIORITY_NORMAL,      // 线程优先级：普通
		0,                           // 栈大小：默认
		CREATE_SUSPENDED             // 创建标志：挂起状态
	);
	
	if (m_pSerialRecvThread == NULL)
	{
		// 线程创建失败，清理资源并返回
		m_bSerialThreadRunning = FALSE;
		TRACE(_T("串口接收线程创建失败\n"));
		CloseHandle(m_hSerialPort);
		m_hSerialPort = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	
	// 恢复线程运行
	m_pSerialRecvThread->ResumeThread();

	// ============================================================
	// 连接成功，更新状态标志
	// ============================================================
	m_bSerialConnected = TRUE;
	TRACE(_T("串口打开成功 (%s, %d baud)\n"), _T(SERIAL_PORT_NAME), SERIAL_BAUD_RATE);
	return TRUE;
}

// ============================================================================
// 关闭串口函数
// ============================================================================
// 功能：停止接收线程，关闭串口设备，清理资源
// 步骤：
//   1. 检查连接状态
//   2. 停止接收线程（设置运行标志为FALSE）
//   3. 关闭串口句柄（使ReadFile返回错误，线程自动退出）
//   4. 等待线程结束（最多等待3秒）
//   5. 清空接收缓冲区
//   6. 更新连接状态标志
// ============================================================================
void CFWGCSDlgDlg::CloseSerialPort()
{
	// ============================================================
	// 步骤1：检查连接状态
	// ============================================================
	if (!m_bSerialConnected)
	{
		return;  // 未连接，直接返回
	}

	// ============================================================
	// 步骤2：停止接收线程
	// ============================================================
	m_bSerialThreadRunning = FALSE;  // 设置线程运行标志为FALSE，线程循环将退出

	// ============================================================
	// 步骤3：关闭串口句柄
	// ============================================================
	// 关闭串口后，ReadFile会返回错误，线程检测到错误后退出循环
	if (m_hSerialPort != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hSerialPort);
		m_hSerialPort = INVALID_HANDLE_VALUE;
	}

	// ============================================================
	// 步骤4：等待线程结束
	// ============================================================
	// 等待接收线程完全退出（最多等待3秒，避免程序卡死）
	if (m_pSerialRecvThread != NULL)
	{
		WaitForSingleObject(m_pSerialRecvThread->m_hThread, 3000);
		m_pSerialRecvThread = NULL;  // 清空线程指针
	}

	// ============================================================
	// 步骤5：清空接收缓冲区
	// ============================================================
	m_nSerialBufferSize = 0;  // 重置缓冲区大小
	memset(m_serialBuffer, 0, sizeof(m_serialBuffer));  // 清空缓冲区内容

	// ============================================================
	// 步骤6：更新连接状态标志
	// ============================================================
	m_bSerialConnected = FALSE;
	TRACE(_T("串口已关闭\n"));
}

// ============================================================================
// 发送串口数据函数
// ============================================================================
// 功能：通过串口发送数据
// 参数：
//   - pData：要发送的数据指针
//   - nSize：要发送的数据大小（字节数）
// 返回值：
//   - TRUE：发送成功（发送的字节数等于请求的字节数）
//   - FALSE：发送失败（串口未打开、未连接或WriteFile失败）
// ============================================================================
BOOL CFWGCSDlgDlg::SendSerialData(const void* pData, int nSize)
{
	// ============================================================
	// 检查串口状态
	// ============================================================
	if (m_hSerialPort == INVALID_HANDLE_VALUE || !m_bSerialConnected)
	{
		return FALSE;  // 串口未打开或未连接，返回失败
	}

	// ============================================================
	// 发送数据
	// ============================================================
	DWORD dwBytesWritten = 0;  // 实际写入的字节数
	if (!WriteFile(m_hSerialPort, pData, nSize, &dwBytesWritten, NULL))
	{
		// WriteFile失败，记录错误并返回
		TRACE(_T("串口发送失败，错误代码: %d\n"), GetLastError());
		return FALSE;
	}

	// ============================================================
	// 验证发送结果
	// ============================================================
	// 检查实际发送的字节数是否等于请求的字节数
	return (dwBytesWritten == nSize);
}

// ============================================================================
// 串口接收线程函数（静态函数，工作线程）
// ============================================================================
// 功能：在独立线程中持续接收串口数据，处理数据包边界，发送到主线程显示
// 参数：
//   - pParam：对话框指针（CFWGCSDlgDlg*）
// 返回值：
//   - 0：线程正常退出
// 工作流程：
//   1. 循环读取串口数据，ReadFile阻塞等待，流式接收
//   2. 将新数据追加到接收缓冲区（处理不完整数据包）
//   3. 从缓冲区中提取完整的数据包
//   4. 只处理最后一个完整数据包，避免消息队列积压，以及UI频繁刷新
//   5. 通过PostMessage发送到主线程处理
//   6. 保留不完整数据到下次接收时拼接
// ============================================================================
UINT CFWGCSDlgDlg::SerialRecvThread(LPVOID pParam)
{
	CFWGCSDlgDlg* pDlg = (CFWGCSDlgDlg*)pParam;  // 获取对话框指针
	BYTE buffer[1024];                            // 临时接收缓冲区（每次ReadFile的最大读取量）
	DWORD dwBytesRead;                            // 实际读取的字节数
	int nPacketSize = sizeof(UdpRecvDataPacket);  // 数据包大小：20字节（5个float × 4字节）
	
	TRACE(_T("串口接收线程启动，数据包大小: %d 字节\n"), nPacketSize);

	// =============================================================
	// 主循环：持续接收数据直到线程停止标志为FALSE
	// =============================================================
	while (pDlg->m_bSerialThreadRunning)
	{
		// ============================================================
		// 步骤1：阻塞等待接收数据
		// ============================================================
		// ReadFile会阻塞等待，直到有数据到达或超时
		if (ReadFile(pDlg->m_hSerialPort, buffer, sizeof(buffer), &dwBytesRead, NULL))
		{
			if (dwBytesRead > 0)
			{
				TRACE(_T("串口接收线程: 收到 %d 字节数据\n"), dwBytesRead);

				// ============================================================
				// 步骤2：合并新数据到接收缓冲区
				// ============================================================
				// 处理两种情况：
				//   a) 缓冲区中已有不完整数据：追加新数据
				//   b) 缓冲区为空：直接使用新数据
				if (pDlg->m_nSerialBufferSize > 0)
				{
					// 情况a：缓冲区中已有数据，追加新数据
					if (pDlg->m_nSerialBufferSize + dwBytesRead <= sizeof(pDlg->m_serialBuffer))
					{
						// 缓冲区空间足够，追加新数据
						memcpy(pDlg->m_serialBuffer + pDlg->m_nSerialBufferSize, buffer, dwBytesRead);
						pDlg->m_nSerialBufferSize += dwBytesRead;
					}
					else
					{
						// 缓冲区空间不足，丢弃旧的不完整数据，使用新数据
						// 这种情况不应该发生（缓冲区2048字节足够大），但需要处理
						TRACE(_T("串口接收: 警告！缓冲区空间不足，丢弃旧的不完整数据\n"));
						memcpy(pDlg->m_serialBuffer, buffer, dwBytesRead);
						pDlg->m_nSerialBufferSize = dwBytesRead;
					}
				}
				else
				{
					// 情况b：缓冲区为空，直接使用新接收的数据
					memcpy(pDlg->m_serialBuffer, buffer, dwBytesRead);
					pDlg->m_nSerialBufferSize = dwBytesRead;
				}

				// ============================================================
				// 步骤3：从缓冲区中提取完整的数据包
				// ============================================================
				int nPacketCount = pDlg->m_nSerialBufferSize / nPacketSize;        // 完整数据包数量
				int nProcessedBytes = nPacketCount * nPacketSize;                   // 已处理字节数
				int nRemainingBytes = pDlg->m_nSerialBufferSize - nProcessedBytes; // 剩余不完整数据字节数

				TRACE(_T("串口接收: 完整数据包 %d 个，剩余 %d 字节\n"), nPacketCount, nRemainingBytes);

				// ============================================================
				// 步骤4：只处理最后一个完整数据包
				// ============================================================
				// 原因：避免UI频繁刷新，只显示最新的数据
				if (nPacketCount > 0)
				{
				// 分配内存保存最后一个数据包（通过消息传递到主线程）
				UdpRecvDataPacket* pPacket = new UdpRecvDataPacket;
				// 复制最后一个完整数据包
				memcpy(pPacket, pDlg->m_serialBuffer + (nPacketCount - 1) * nPacketSize, nPacketSize);

				// 字节序转换（如果串口数据是大端字节序，需要转换）
				// Windows是小端系统，如果发送端也是小端，则不需要转换
				// 如果发送端是大端，需要取消下面注释来启用字节序转换
				// pPacket->pitchAngle = _byteswap_ushort(pPacket->pitchAngle);
				// pPacket->rollAngle = _byteswap_ushort(pPacket->rollAngle);
				// pPacket->yawAngle = _byteswap_ushort(pPacket->yawAngle);
				// pPacket->attackAngle = _byteswap_ushort(pPacket->attackAngle);
				// pPacket->sideslipAngle = _byteswap_ushort(pPacket->sideslipAngle);

				// 调试输出：显示接收到的最后一个数据包内容（原始int16_t值）
				TRACE(_T("串口接收[最后/%d]: pitchAngle=%d, rollAngle=%d, yawAngle=%d, attackAngle=%d, sideslipAngle=%d\n"),
					nPacketCount,
					pPacket->pitchAngle, pPacket->rollAngle, pPacket->yawAngle, pPacket->attackAngle, pPacket->sideslipAngle);
				
				// 调试输出：显示原始字节值（用于诊断字节序问题）
				BYTE* pBytes = (BYTE*)pPacket;
				TRACE(_T("串口接收原始字节[前10字节]: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n"),
					pBytes[0], pBytes[1], pBytes[2], pBytes[3], pBytes[4], 
					pBytes[5], pBytes[6], pBytes[7], pBytes[8], pBytes[9]);

					// 发送消息到主线程处理（只发送最后一个数据包）
					// 主线程会在OnSerialDataReceivedMsg中清除队列中的旧消息
					pDlg->PostMessage(WM_SERIAL_DATA_RECEIVED, (WPARAM)pPacket, 0);
				}

				// ============================================================
				// 步骤5：处理剩余的不完整数据
				// ============================================================
				// 如果剩余数据小于一个完整数据包，保留到缓冲区，等待下次接收时拼接
				// 如果剩余数据大于等于一个完整数据包，说明计算错误，清空缓冲区
				if (nRemainingBytes > 0 && nRemainingBytes < nPacketSize)
				{
					// 保留剩余数据：移动到缓冲区开头，等待下次接收时拼接
					memmove(pDlg->m_serialBuffer, pDlg->m_serialBuffer + nProcessedBytes, nRemainingBytes);
					pDlg->m_nSerialBufferSize = nRemainingBytes;
					TRACE(_T("串口接收: 保留 %d 字节不完整数据到缓冲区\n"), nRemainingBytes);
				}
				else
				{
					// 没有剩余数据，或剩余数据异常（不应该发生）
					pDlg->m_nSerialBufferSize = 0;
					if (nRemainingBytes >= nPacketSize)
					{
						// 剩余数据异常：理论上不应该发生，清空缓冲区
						TRACE(_T("串口接收: 警告！剩余数据异常 (%d 字节)，已清空\n"), nRemainingBytes);
					}
				}
			}
		}
		else
		{
			// ============================================================
			// ReadFile失败，可能是串口已关闭
			// ============================================================
			DWORD dwError = GetLastError();
			if (dwError != ERROR_IO_PENDING)
			{
				// 非IO_PENDING错误，记录并退出线程
				TRACE(_T("串口读取失败，错误代码: %d\n"), dwError);
				break;  // 退出线程循环
			}
		}
	}

	// ============================================================
	// 线程退出
	// ============================================================
	TRACE(_T("串口接收线程退出\n"));
	return 0;
}

// ============================================================================
// 串口数据接收消息处理函数（主线程）
// ============================================================================
// 功能：处理从串口接收线程发送过来的数据包消息
// 参数：
//   - wParam：数据包指针（UdpRecvDataPacket*）
//   - lParam：未使用
// 返回值：
//   - 0：消息已处理
// 工作流程：
//   1. 清除消息队列中的旧消息（避免UI频繁刷新）
//   2. 释放旧消息中的数据包内存
//   3. 处理当前数据包（更新UI显示）
//   4. 释放当前数据包内存
// 注意事项：
//   - 此函数在主线程中执行，可以安全访问UI控件
//   - 需要手动释放数据包内存（由接收线程new分配）
// ============================================================================
LRESULT CFWGCSDlgDlg::OnSerialDataReceivedMsg(WPARAM wParam, LPARAM lParam)
{
	UdpRecvDataPacket* pPacket = (UdpRecvDataPacket*)wParam;
	if (pPacket != NULL)
	{
		// ============================================================
		// 步骤1：清除消息队列中的旧消息
		// ============================================================
		// 原因：如果接收速度很快，消息队列可能积压多个数据包
		// 只处理最新的数据包，避免UI频繁刷新和内存泄漏
		MSG msg;
		while (PeekMessage(&msg, m_hWnd, WM_SERIAL_DATA_RECEIVED, WM_SERIAL_DATA_RECEIVED, PM_REMOVE))
		{
			// ============================================================
			// 步骤2：释放旧消息中的数据包内存
			// ============================================================
			// 注意：当前消息（wParam）的数据包不要在这里删除，后面还要使用
			if (msg.wParam != NULL && msg.wParam != wParam)
			{
				UdpRecvDataPacket* pOldPacket = (UdpRecvDataPacket*)msg.wParam;
				delete pOldPacket;  // 释放旧数据包内存
			}
		}

		// ============================================================
		// 步骤3：处理接收到的数据包（更新UI显示）
		// ============================================================
		ProcessSerialReceivedData(pPacket);

		// ============================================================
		// 步骤4：释放当前数据包内存
		// ============================================================
		delete pPacket;
	}
	return 0;
}

// ============================================================================
// 辅助函数：更新控件文本（优先在子对话框中查找）
// ============================================================================
// 功能：更新指定ID的控件文本，优先在子对话框中查找，如果找不到再在主对话框中查找
// 参数：
//   - nID：控件ID
//   - strText：要设置的文本
// ============================================================================
void CFWGCSDlgDlg::UpdateControlText(UINT nID, const CString& strText)
{
	BOOL bUpdated = FALSE;
	// 优先在子对话框中查找控件
	if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
	{
		CWnd* pWnd = m_pPage1Dlg->GetDlgItem(nID);
		if (pWnd != NULL)
		{
			pWnd->SetWindowText(strText);
			bUpdated = TRUE;
		}
	}
	// 如果子对话框未创建或未找到控件，在主对话框中查找
	if (!bUpdated)
	{
		CWnd* pWnd = GetDlgItem(nID);
		if (pWnd != NULL)
		{
			pWnd->SetWindowText(strText);
		}
	}
}

// ============================================================================
// 处理串口接收到的数据包函数
// ============================================================================
// 功能：格式化串口接收到的数据包，更新UI显示控件
// 参数：
//   - pPacket：接收到的数据包指针（UdpRecvDataPacket*）
// 工作流程：
//   1. 验证数据包指针有效性
//   2. 格式化5个float数据为字符串（保留2位小数）
//   3. 更新对应的Edit Control控件显示
//   4. 如果控件未绑定，使用GetDlgItem获取控件并更新
// 注意事项：
//   - 此函数在主线程中执行，可以安全访问UI控件
//   - 与UDP数据处理使用相同的显示逻辑和控件
//   - 使用双重检查：先检查控件句柄，失败则使用GetDlgItem
//   - 优先在子对话框中查找控件，如果找不到再在主对话框中查找
// ============================================================================
void CFWGCSDlgDlg::ProcessSerialReceivedData(const UdpRecvDataPacket* pPacket)
{
	// ========================================================================
	// 步骤1：验证数据包指针有效性
	// ========================================================================
	if (pPacket == NULL)
	{
		TRACE(_T("ProcessSerialReceivedData: 数据包指针为空！\n"));
		return;
	}

	// 将关键数据推送给 WebView2，用于 HUD 渲染（UI 控件已移除）
	SendHudMessage(pPacket);
	return;

#if 0  // 以下为旧版控件更新逻辑，已停用
	// ============================================================
	// 步骤2：格式化数据为字符串（保留2位小数）
	// ============================================================
	// 注意：int16_t值需要转换为浮点数显示（单位1度）
	CString strData1, strData2, strData3, strData4, strData5;
	strData1.Format(_T("%.2f"), pPacket->pitchAngle / 1.0f);   // 转换为度（0.01度单位）
	strData2.Format(_T("%.2f"), pPacket->rollAngle / 1.0f);
	strData3.Format(_T("%.2f"), pPacket->yawAngle / 1.0f);
	strData4.Format(_T("%.2f"), pPacket->attackAngle / 1.0f);
	strData5.Format(_T("%.2f"), pPacket->sideslipAngle / 1.0f);
	
	// 格式化所有剩余字段
	CString strData6, strData7, strData8, strData9, strData10, strData11, strData12, strData13;
	CString strData14, strData15, strData16, strData17, strData18, strData19, strData20, strData21, strData22;
	CString strData23, strData24, strData25, strData26;
	CString strData27, strData28, strData29, strData30, strData31, strData32;
	CString strData33, strData34, strData35, strData36, strData37, strData38, strData39, strData40, strData41, strData42, strData43;
	CString strData44, strData45, strData46, strData47, strData48, strData49, strData50, strData51, strData52, strData53;
	CString strData54, strData55, strData56, strData57, strData58;
	CString strData59, strData60, strData61, strData62, strData63, strData64, strData65;
	CString strData66;
	CString strData67, strData68, strData69, strData70, strData71, strData72, strData73;
	
	// 视窗组1相关字段
	strData6.Format(_T("%.2f"), pPacket->pitchRate / 1.0f);
	strData7.Format(_T("%.2f"), pPacket->rollRate / 1.0f);
	strData8.Format(_T("%.2f"), pPacket->yawRate / 1.0f);
	strData9.Format(_T("%.2f"), pPacket->pitchAcceleration / 1.0f);
	strData10.Format(_T("%.2f"), pPacket->rollAcceleration / 1.0f);
	strData11.Format(_T("%.2f"), pPacket->yawAcceleration / 1.0f);
	strData12.Format(_T("%.2f"), pPacket->normalOverload / 1.0f);
	strData13.Format(_T("%.2f"), pPacket->longitudinalOverload / 1.0f);
	strData14.Format(_T("%.2f"), pPacket->lateralOverload / 1.0f);
	
	// 视窗组2相关字段
	strData15.Format(_T("%.2f"), pPacket->baroAltitude / 1.0f);
	strData16.Format(_T("%u"), pPacket->radioAltitude);
	strData17.Format(_T("%.2f"), pPacket->baroAirspeed / 1.0f);
	strData18.Format(_T("%.2f"), pPacket->indicatedAirspeed / 1.0f);
	strData19.Format(_T("%u"), pPacket->machNumber);
	strData20.Format(_T("%.2f"), pPacket->eastVelocity / 1.0f);
	strData21.Format(_T("%.2f"), pPacket->northVelocity / 1.0f);
	strData22.Format(_T("%.2f"), pPacket->verticalVelocity / 1.0f);
	strData23.Format(_T("%d"), pPacket->airTemperature);
	
	// 视窗组3相关字段
	strData24.Format(_T("%u"), pPacket->throttle);
	strData25.Format(_T("%.2f"), pPacket->engineTemp / 1.0f);
	strData26.Format(_T("%.2f"), pPacket->engineRPM / 1.0f);
	strData27.Format(_T("%u"), pPacket->fuelRemaining);
	
	// 视窗组4相关字段
	strData28.Format(_T("%d"), pPacket->rudderCmd1);
	strData29.Format(_T("%d"), pPacket->rudderCmd2);
	strData30.Format(_T("%d"), pPacket->rudderCmd3);
	strData31.Format(_T("%d"), pPacket->rudderCmd4);
	// 注意：IDC_Display31 在资源文件中不存在，turnRudderCmd 应该显示在 IDC_Display32
	strData32.Format(_T("%d"), pPacket->turnRudderCmd);  // 转弯舵机指令 -> IDC_Display32
	// controlCommand 已弃用，使用扩展协议的 bool 字段替代（IDC_RADIO_Flag1~3）
	strData33.Format(_T(""));  // IDC_Display32 已用于显示转弯舵机指令，不再显示 controlCommand
	
	// 视窗组5相关字段（GPS）
	strData34.Format(_T("%d"), pPacket->longitude);
	strData35.Format(_T("%d"), pPacket->latitude);
	strData36.Format(_T("%.2f"), pPacket->gpsAltitude / 1.0f);
	strData37.Format(_T("%u"), pPacket->gpsStatus);
	strData38.Format(_T("%u"), pPacket->satelitesNum);
	strData39.Format(_T("%.2f"), pPacket->gpsCourse / 1.0f);
	strData40.Format(_T("%.2f"), pPacket->gpsGroundSpeed / 1.0f);
	strData41.Format(_T("%u"), pPacket->gpsVerticalSpeed);
	strData42.Format(_T("%u"), pPacket->gpsHour);
	strData43.Format(_T("%u"), pPacket->gpsMinute);
	strData44.Format(_T("%u"), pPacket->gpsSecond);
	
	// 视窗组6相关字段（导航）
	strData45.Format(_T("%u"), pPacket->navStatus);
	strData46.Format(_T("%u"), pPacket->routeNumber);
	strData47.Format(_T("%u"), pPacket->targetWaypoint);
	strData48.Format(_T("%.2f"), pPacket->crossTrackError / 1.0f);
	strData49.Format(_T("%.2f"), pPacket->courseDeviation / 1.0f);
	strData50.Format(_T("%.2f"), pPacket->distanceToGo / 1.0f);
	strData51.Format(_T("%u"), pPacket->commandHeading);
	strData52.Format(_T("%u"), pPacket->commandSpeed);
	strData53.Format(_T("%u"), pPacket->commandAltitude);
	strData54.Format(_T("%u"), pPacket->commandTime);
	
	// 视窗组7相关字段（目标）
	strData55.Format(_T("%d"), pPacket->targetLongitude);
	strData56.Format(_T("%d"), pPacket->targetLatitude);
	strData57.Format(_T("%.2f"), pPacket->targetAltitude / 1.0f);
	strData58.Format(_T("%d"), pPacket->targetSpeed);
	strData59.Format(_T("%.2f"), pPacket->targetCourse / 1.0f);
	
	// 视窗组8相关字段（载荷）
	strData60.Format(_T("%u"), pPacket->payloadType);
	strData61.Format(_T("%u"), pPacket->ammoRemaining);
	strData62.Format(_T("%u"), pPacket->selfTestResult);
	strData63.Format(_T("%u"), pPacket->batteryVoltage);
	// workflowStatus, alarmStatus, switchStatus 已弃用，使用扩展协议的 bool 字段替代
	strData64.Format(_T("0"));  // 已弃用字段，保留用于兼容
	strData65.Format(_T("0"));  // 已弃用字段，保留用于兼容
	strData66.Format(_T("0"));  // 已弃用字段，保留用于兼容
	
	// 视窗组9相关字段（自检结果）
	strData67.Format(_T("%u"), pPacket->YIS100A_result);
	strData68.Format(_T("%u"), pPacket->HP5804_result);
	strData69.Format(_T("%u"), pPacket->MS4525D_result);
	strData70.Format(_T("%u"), pPacket->M401_result);
	strData71.Format(_T("%u"), pPacket->GPS_result);
	strData72.Format(_T("%u"), pPacket->PAC1931_result1);
	strData73.Format(_T("%u"), pPacket->can_to_pw_result);
	CString strData74;
	strData74.Format(_T("%u"), pPacket->SBUS_result);

	// ============================================================
	// 步骤3：更新所有数据显示控件
	// ============================================================
	// 使用双重检查：先检查控件句柄是否有效，失败则使用GetDlgItem获取控件
	
	// 更新data1显示控件（IDC_Display0）
	UpdateControlText(IDC_Display0, strData1);

	// 更新data2-5显示控件
	UpdateControlText(IDC_Display1, strData2);
	UpdateControlText(IDC_Display2, strData3);
	UpdateControlText(IDC_Display3, strData4);
	UpdateControlText(IDC_Display4, strData5);

	// 更新data6显示控件（IDC_Display5）
	if (m_editDisplay5.GetSafeHwnd() != NULL)
	{
		m_editDisplay5.SetWindowText(strData6);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display5);
		if (pWnd != NULL) pWnd->SetWindowText(strData6);
	}

	// 更新data7显示控件（IDC_Display6）
	if (m_editDisplay6.GetSafeHwnd() != NULL)
	{
		m_editDisplay6.SetWindowText(strData7);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display6);
		if (pWnd != NULL) pWnd->SetWindowText(strData7);
	}

	// 更新data8显示控件（IDC_Display7）
	if (m_editDisplay7.GetSafeHwnd() != NULL)
	{
		m_editDisplay7.SetWindowText(strData8);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display7);
		if (pWnd != NULL) pWnd->SetWindowText(strData8);
	}

	// 更新data9显示控件（IDC_Display8）
	if (m_editDisplay8.GetSafeHwnd() != NULL)
	{
		m_editDisplay8.SetWindowText(strData9);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display8);
		if (pWnd != NULL) pWnd->SetWindowText(strData9);
	}

	// 更新data10显示控件（IDC_Display9）
	if (m_editDisplay9.GetSafeHwnd() != NULL)
	{
		m_editDisplay9.SetWindowText(strData10);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display9);
		if (pWnd != NULL) pWnd->SetWindowText(strData10);
	}

	// 更新data11显示控件（IDC_Display10）
	if (m_editDisplay10.GetSafeHwnd() != NULL)
	{
		m_editDisplay10.SetWindowText(strData11);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display10);
		if (pWnd != NULL) pWnd->SetWindowText(strData11);
	}

	// 更新data12显示控件（IDC_Display11）
	if (m_editDisplay11.GetSafeHwnd() != NULL)
	{
		m_editDisplay11.SetWindowText(strData12);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display11);
		if (pWnd != NULL) pWnd->SetWindowText(strData12);
	}

	// 更新data13显示控件（IDC_Display12）
	if (m_editDisplay12.GetSafeHwnd() != NULL)
	{
		m_editDisplay12.SetWindowText(strData13);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display12);
		if (pWnd != NULL) pWnd->SetWindowText(strData13);
	}

	// 更新data14显示控件（IDC_Display13）
	if (m_editDisplay13.GetSafeHwnd() != NULL)
	{
		m_editDisplay13.SetWindowText(strData14);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display13);
		if (pWnd != NULL) pWnd->SetWindowText(strData14);
	}

	// 更新data15显示控件（IDC_Display14）
	if (m_editDisplay14.GetSafeHwnd() != NULL)
	{
		m_editDisplay14.SetWindowText(strData15);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display14);
		if (pWnd != NULL) pWnd->SetWindowText(strData15);
	}

	// 更新data16显示控件（IDC_Display15）
	if (m_editDisplay15.GetSafeHwnd() != NULL)
	{
		m_editDisplay15.SetWindowText(strData16);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display15);
		if (pWnd != NULL) pWnd->SetWindowText(strData16);
	}

	// 更新data17显示控件（IDC_Display16）
	if (m_editDisplay16.GetSafeHwnd() != NULL)
	{
		m_editDisplay16.SetWindowText(strData17);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display16);
		if (pWnd != NULL) pWnd->SetWindowText(strData17);
	}

	// 更新data18显示控件（IDC_Display17）
	if (m_editDisplay17.GetSafeHwnd() != NULL)
	{
		m_editDisplay17.SetWindowText(strData18);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display17);
		if (pWnd != NULL) pWnd->SetWindowText(strData18);
	}

	// 更新data19显示控件（IDC_Display18）
	if (m_editDisplay18.GetSafeHwnd() != NULL)
	{
		m_editDisplay18.SetWindowText(strData19);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display18);
		if (pWnd != NULL) pWnd->SetWindowText(strData19);
	}

	// 更新data20显示控件（IDC_Display19）
	if (m_editDisplay19.GetSafeHwnd() != NULL)
	{
		m_editDisplay19.SetWindowText(strData20);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display19);
		if (pWnd != NULL) pWnd->SetWindowText(strData20);
	}

	// 更新data21显示控件（IDC_Display20）
	if (m_editDisplay20.GetSafeHwnd() != NULL)
	{
		m_editDisplay20.SetWindowText(strData21);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display20);
		if (pWnd != NULL) pWnd->SetWindowText(strData21);
	}

	// 更新data22显示控件（IDC_Display21）
	if (m_editDisplay21.GetSafeHwnd() != NULL)
	{
		m_editDisplay21.SetWindowText(strData22);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display21);
		if (pWnd != NULL) pWnd->SetWindowText(strData22);
	}

	// 更新data23显示控件（IDC_Display22）
	if (m_editDisplay22.GetSafeHwnd() != NULL)
	{
		m_editDisplay22.SetWindowText(strData23);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display22);
		if (pWnd != NULL) pWnd->SetWindowText(strData23);
	}

	// 更新data24显示控件（IDC_Display23）
	if (m_editDisplay23.GetSafeHwnd() != NULL)
	{
		m_editDisplay23.SetWindowText(strData24);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display23);
		if (pWnd != NULL) pWnd->SetWindowText(strData24);
	}

	// 更新data25显示控件（IDC_Display24）
	if (m_editDisplay24.GetSafeHwnd() != NULL)
	{
		m_editDisplay24.SetWindowText(strData25);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display24);
		if (pWnd != NULL) pWnd->SetWindowText(strData25);
	}

	// 更新data26显示控件（IDC_Display25）
	if (m_editDisplay25.GetSafeHwnd() != NULL)
	{
		m_editDisplay25.SetWindowText(strData26);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display25);
		if (pWnd != NULL) pWnd->SetWindowText(strData26);
	}

	// 更新data27显示控件（IDC_Display26）
	if (m_editDisplay26.GetSafeHwnd() != NULL)
	{
		m_editDisplay26.SetWindowText(strData27);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display26);
		if (pWnd != NULL) pWnd->SetWindowText(strData27);
	}

	// 更新data28显示控件（IDC_Display27）
	if (m_editDisplay27.GetSafeHwnd() != NULL)
	{
		m_editDisplay27.SetWindowText(strData28);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display27);
		if (pWnd != NULL) pWnd->SetWindowText(strData28);
	}

	// 更新data29显示控件（IDC_Display28）
	if (m_editDisplay28.GetSafeHwnd() != NULL)
	{
		m_editDisplay28.SetWindowText(strData29);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display28);
		if (pWnd != NULL) pWnd->SetWindowText(strData29);
	}

	// 更新data30显示控件（IDC_Display29）
	if (m_editDisplay29.GetSafeHwnd() != NULL)
	{
		m_editDisplay29.SetWindowText(strData30);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display29);
		if (pWnd != NULL) pWnd->SetWindowText(strData30);
	}

	// 更新data31显示控件（IDC_Display30）
	if (m_editDisplay30.GetSafeHwnd() != NULL)
	{
		m_editDisplay30.SetWindowText(strData31);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display30);
		if (pWnd != NULL) pWnd->SetWindowText(strData31);
	}

	// 更新转弯舵机指令显示控件（IDC_Display32）
	// 注意：IDC_Display31 在资源文件中不存在，turnRudderCmd 显示在 IDC_Display32
	if (m_editDisplay32.GetSafeHwnd() != NULL)
	{
		m_editDisplay32.SetWindowText(strData32);  // turnRudderCmd -> IDC_Display32
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display32);
		if (pWnd != NULL) pWnd->SetWindowText(strData32);
	}
	// IDC_Display32 已用于显示转弯舵机指令，不再显示 controlCommand（已改为 Radio Button）

	// 更新data34显示控件（IDC_Display33）
	if (m_editDisplay33.GetSafeHwnd() != NULL)
	{
		m_editDisplay33.SetWindowText(strData34);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display33);
		if (pWnd != NULL) pWnd->SetWindowText(strData34);
	}

	// 更新data35显示控件（IDC_Display34）
	if (m_editDisplay34.GetSafeHwnd() != NULL)
	{
		m_editDisplay34.SetWindowText(strData35);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display34);
		if (pWnd != NULL) pWnd->SetWindowText(strData35);
	}

	// 更新data36显示控件（IDC_Display35）
	if (m_editDisplay35.GetSafeHwnd() != NULL)
	{
		m_editDisplay35.SetWindowText(strData36);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display35);
		if (pWnd != NULL) pWnd->SetWindowText(strData36);
	}

	// 更新data37显示控件（IDC_Display36）
	if (m_editDisplay36.GetSafeHwnd() != NULL)
	{
		m_editDisplay36.SetWindowText(strData37);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display36);
		if (pWnd != NULL) pWnd->SetWindowText(strData37);
	}

	// 更新data38显示控件（IDC_Display37）
	if (m_editDisplay37.GetSafeHwnd() != NULL)
	{
		m_editDisplay37.SetWindowText(strData38);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display37);
		if (pWnd != NULL) pWnd->SetWindowText(strData38);
	}

	// 更新data39显示控件（IDC_Display38）
	if (m_editDisplay38.GetSafeHwnd() != NULL)
	{
		m_editDisplay38.SetWindowText(strData39);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display38);
		if (pWnd != NULL) pWnd->SetWindowText(strData39);
	}

	// 更新data40显示控件（IDC_Display39）
	if (m_editDisplay39.GetSafeHwnd() != NULL)
	{
		m_editDisplay39.SetWindowText(strData40);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display39);
		if (pWnd != NULL) pWnd->SetWindowText(strData40);
	}

	// 更新data41显示控件（IDC_Display40）
	if (m_editDisplay40.GetSafeHwnd() != NULL)
	{
		m_editDisplay40.SetWindowText(strData41);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display40);
		if (pWnd != NULL) pWnd->SetWindowText(strData41);
	}

	// 更新data42显示控件（IDC_Display41）
	if (m_editDisplay41.GetSafeHwnd() != NULL)
	{
		m_editDisplay41.SetWindowText(strData42);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display41);
		if (pWnd != NULL) pWnd->SetWindowText(strData42);
	}

	// 更新data43显示控件（IDC_Display42）
	if (m_editDisplay42.GetSafeHwnd() != NULL)
	{
		m_editDisplay42.SetWindowText(strData43);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display42);
		if (pWnd != NULL) pWnd->SetWindowText(strData43);
	}

	// 更新data44显示控件（IDC_Display43）
	if (m_editDisplay43.GetSafeHwnd() != NULL)
	{
		m_editDisplay43.SetWindowText(strData44);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display43);
		if (pWnd != NULL) pWnd->SetWindowText(strData44);
	}

	// 更新data45显示控件（IDC_Display44）
	if (m_editDisplay44.GetSafeHwnd() != NULL)
	{
		m_editDisplay44.SetWindowText(strData45);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display44);
		if (pWnd != NULL) pWnd->SetWindowText(strData45);
	}

	// 更新data46显示控件（IDC_Display45）
	if (m_editDisplay45.GetSafeHwnd() != NULL)
	{
		m_editDisplay45.SetWindowText(strData46);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display45);
		if (pWnd != NULL) pWnd->SetWindowText(strData46);
	}

	// 更新data47显示控件（IDC_Display46）
	if (m_editDisplay46.GetSafeHwnd() != NULL)
	{
		m_editDisplay46.SetWindowText(strData47);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display46);
		if (pWnd != NULL) pWnd->SetWindowText(strData47);
	}

	// 更新data48显示控件（IDC_Display47）
	if (m_editDisplay47.GetSafeHwnd() != NULL)
	{
		m_editDisplay47.SetWindowText(strData48);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display47);
		if (pWnd != NULL) pWnd->SetWindowText(strData48);
	}

	// 更新data49显示控件（IDC_Display48）
	if (m_editDisplay48.GetSafeHwnd() != NULL)
	{
		m_editDisplay48.SetWindowText(strData49);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display48);
		if (pWnd != NULL) pWnd->SetWindowText(strData49);
	}

	// 更新data50显示控件（IDC_Display49）
	if (m_editDisplay49.GetSafeHwnd() != NULL)
	{
		m_editDisplay49.SetWindowText(strData50);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display49);
		if (pWnd != NULL) pWnd->SetWindowText(strData50);
	}

	// 更新data51显示控件（IDC_Display50）
	if (m_editDisplay50.GetSafeHwnd() != NULL)
	{
		m_editDisplay50.SetWindowText(strData51);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display50);
		if (pWnd != NULL) pWnd->SetWindowText(strData51);
	}

	// 更新data52显示控件（IDC_Display51）
	if (m_editDisplay51.GetSafeHwnd() != NULL)
	{
		m_editDisplay51.SetWindowText(strData52);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display51);
		if (pWnd != NULL) pWnd->SetWindowText(strData52);
	}

	// 更新data53显示控件（IDC_Display52）
	if (m_editDisplay52.GetSafeHwnd() != NULL)
	{
		m_editDisplay52.SetWindowText(strData53);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display52);
		if (pWnd != NULL) pWnd->SetWindowText(strData53);
	}

	// 更新data54显示控件（IDC_Display53）
	if (m_editDisplay53.GetSafeHwnd() != NULL)
	{
		m_editDisplay53.SetWindowText(strData54);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display53);
		if (pWnd != NULL) pWnd->SetWindowText(strData54);
	}

	// 更新data55显示控件（IDC_Display54）
	if (m_editDisplay54.GetSafeHwnd() != NULL)
	{
		m_editDisplay54.SetWindowText(strData55);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display54);
		if (pWnd != NULL) pWnd->SetWindowText(strData55);
	}

	// 更新data56显示控件（IDC_Display55）
	if (m_editDisplay55.GetSafeHwnd() != NULL)
	{
		m_editDisplay55.SetWindowText(strData56);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display55);
		if (pWnd != NULL) pWnd->SetWindowText(strData56);
	}

	// 更新data57显示控件（IDC_Display56）
	if (m_editDisplay56.GetSafeHwnd() != NULL)
	{
		m_editDisplay56.SetWindowText(strData57);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display56);
		if (pWnd != NULL) pWnd->SetWindowText(strData57);
	}

	// 更新data58显示控件（IDC_Display57）
	if (m_editDisplay57.GetSafeHwnd() != NULL)
	{
		m_editDisplay57.SetWindowText(strData58);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display57);
		if (pWnd != NULL) pWnd->SetWindowText(strData58);
	}

	// 更新data59显示控件（IDC_Display58）
	if (m_editDisplay58.GetSafeHwnd() != NULL)
	{
		m_editDisplay58.SetWindowText(strData59);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display58);
		if (pWnd != NULL) pWnd->SetWindowText(strData59);
	}

	// 更新data60显示控件（IDC_Display59）
	if (m_editDisplay59.GetSafeHwnd() != NULL)
	{
		m_editDisplay59.SetWindowText(strData60);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display59);
		if (pWnd != NULL) pWnd->SetWindowText(strData60);
	}

	// 更新data61显示控件（IDC_Display60）
	if (m_editDisplay60.GetSafeHwnd() != NULL)
	{
		m_editDisplay60.SetWindowText(strData61);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display60);
		if (pWnd != NULL) pWnd->SetWindowText(strData61);
	}

	// 更新data62显示控件（IDC_Display61）
	if (m_editDisplay61.GetSafeHwnd() != NULL)
	{
		m_editDisplay61.SetWindowText(strData62);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display61);
		if (pWnd != NULL) pWnd->SetWindowText(strData62);
	}

	// 更新data63显示控件（IDC_Display62）
	if (m_editDisplay62.GetSafeHwnd() != NULL)
	{
		m_editDisplay62.SetWindowText(strData63);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display62);
		if (pWnd != NULL) pWnd->SetWindowText(strData63);
	}

	// 更新data64显示控件（IDC_Display63）
	if (m_editDisplay63.GetSafeHwnd() != NULL)
	{
		m_editDisplay63.SetWindowText(strData64);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display63);
		if (pWnd != NULL) pWnd->SetWindowText(strData64);
	}

	// 更新data65显示控件（IDC_Display64）
	if (m_editDisplay64.GetSafeHwnd() != NULL)
	{
		m_editDisplay64.SetWindowText(strData65);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display64);
		if (pWnd != NULL) pWnd->SetWindowText(strData65);
	}

	// 更新data66显示控件（IDC_Display65）
	if (m_editDisplay65.GetSafeHwnd() != NULL)
	{
		m_editDisplay65.SetWindowText(strData66);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display65);
		if (pWnd != NULL) pWnd->SetWindowText(strData66);
	}

	// 更新data67显示控件（IDC_Display66）- 在子对话框中
	if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
	{
		CWnd* pWnd = m_pPage1Dlg->GetDlgItem(IDC_Display66);
		if (pWnd != NULL) pWnd->SetWindowText(strData67);
	}

	// 更新data68显示控件（IDC_Display67）- 在子对话框中
	if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
	{
		CWnd* pWnd = m_pPage1Dlg->GetDlgItem(IDC_Display67);
		if (pWnd != NULL) pWnd->SetWindowText(strData68);
	}

	// 更新data69显示控件（IDC_Display68）- 在子对话框中
	if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
	{
		CWnd* pWnd = m_pPage1Dlg->GetDlgItem(IDC_Display68);
		if (pWnd != NULL) pWnd->SetWindowText(strData69);
	}

	// 更新data70显示控件（IDC_Display69）- 在子对话框中
	if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
	{
		CWnd* pWnd = m_pPage1Dlg->GetDlgItem(IDC_Display69);
		if (pWnd != NULL) pWnd->SetWindowText(strData70);
	}

	// 更新data71显示控件（IDC_Display70）- 在子对话框中
	if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
	{
		CWnd* pWnd = m_pPage1Dlg->GetDlgItem(IDC_Display70);
		if (pWnd != NULL) pWnd->SetWindowText(strData71);
	}

	// 更新data72显示控件（IDC_Display71）- 在子对话框中
	if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
	{
		CWnd* pWnd = m_pPage1Dlg->GetDlgItem(IDC_Display71);
		if (pWnd != NULL) pWnd->SetWindowText(strData72);
	}

	// 更新data73显示控件（IDC_Display72）- 在子对话框中
	if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
	{
		CWnd* pWnd = m_pPage1Dlg->GetDlgItem(IDC_Display72);
		if (pWnd != NULL) pWnd->SetWindowText(strData73);
	}

	// 更新data74显示控件（IDC_Display73）- 在子对话框中
	if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
	{
		CWnd* pWnd = m_pPage1Dlg->GetDlgItem(IDC_Display73);
		if (pWnd != NULL) pWnd->SetWindowText(strData74);
	}
	
	// ============================================================
	// 步骤4：更新扩展协议 Radio Button 控件（指示灯显示）
	// ============================================================
	// 视窗组4-5：控制指令标志
	if (m_radioFlag1.GetSafeHwnd() != NULL)
		m_radioFlag1.SetCheck(pPacket->controlCommand_D0 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag2.GetSafeHwnd() != NULL)
		m_radioFlag2.SetCheck(pPacket->controlCommand_D1 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag3.GetSafeHwnd() != NULL)
		m_radioFlag3.SetCheck(pPacket->controlCommand_D2 ? BST_CHECKED : BST_UNCHECKED);
	
	// 视窗组8-4：工作流程标志（特殊处理：workflowStatus_B0 根据值激活不同的 Radio Button）
	if (m_radioFlag4.GetSafeHwnd() != NULL)
		m_radioFlag4.SetCheck(!pPacket->workflowStatus_B0 ? BST_CHECKED : BST_UNCHECKED);  // 0时激活
	if (m_radioFlag5.GetSafeHwnd() != NULL)
		m_radioFlag5.SetCheck(pPacket->workflowStatus_B0 ? BST_CHECKED : BST_UNCHECKED);   // 1时激活
	if (m_radioFlag6.GetSafeHwnd() != NULL)
		m_radioFlag6.SetCheck(pPacket->workflowStatus_B1 ? BST_CHECKED : BST_UNCHECKED);
	
	// 视窗组8-5：报警状态标志
	if (m_radioFlag7.GetSafeHwnd() != NULL)
		m_radioFlag7.SetCheck(pPacket->alarmStatus_B0 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag8.GetSafeHwnd() != NULL)
		m_radioFlag8.SetCheck(pPacket->alarmStatus_B1 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag9.GetSafeHwnd() != NULL)
		m_radioFlag9.SetCheck(pPacket->alarmStatus_B2 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag10.GetSafeHwnd() != NULL)
		m_radioFlag10.SetCheck(pPacket->alarmStatus_B3 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag11.GetSafeHwnd() != NULL)
		m_radioFlag11.SetCheck(pPacket->alarmStatus_B4 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag12.GetSafeHwnd() != NULL)
		m_radioFlag12.SetCheck(pPacket->alarmStatus_B5 ? BST_CHECKED : BST_UNCHECKED);
	
	// 视窗组8-6：开关量状态标志（特殊处理：switchStatus_B5 根据值激活不同的 Radio Button）
	if (m_radioFlag13.GetSafeHwnd() != NULL)
		m_radioFlag13.SetCheck(pPacket->switchStatus_B0 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag14.GetSafeHwnd() != NULL)
		m_radioFlag14.SetCheck(pPacket->switchStatus_B1 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag15.GetSafeHwnd() != NULL)
		m_radioFlag15.SetCheck(pPacket->switchStatus_B2 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag16.GetSafeHwnd() != NULL)
		m_radioFlag16.SetCheck(pPacket->switchStatus_B3 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag17.GetSafeHwnd() != NULL)
		m_radioFlag17.SetCheck(pPacket->switchStatus_B4 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag18.GetSafeHwnd() != NULL)
		m_radioFlag18.SetCheck(!pPacket->switchStatus_B5 ? BST_CHECKED : BST_UNCHECKED);  // 0时激活
	if (m_radioFlag19.GetSafeHwnd() != NULL)
		m_radioFlag19.SetCheck(pPacket->switchStatus_B5 ? BST_CHECKED : BST_UNCHECKED);   // 1时激活
	if (m_radioFlag20.GetSafeHwnd() != NULL)
		m_radioFlag20.SetCheck(pPacket->switchStatus_B6 ? BST_CHECKED : BST_UNCHECKED);
	if (m_radioFlag21.GetSafeHwnd() != NULL)
		m_radioFlag21.SetCheck(pPacket->switchStatus_B7 ? BST_CHECKED : BST_UNCHECKED);
	
	// 更新子对话框显示（优先使用子对话框）
	if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
	{
		m_pPage1Dlg->UpdateDisplay(pPacket);
	}
	if (m_pPage2Dlg != NULL && m_pPage2Dlg->GetSafeHwnd() != NULL)
	{
		m_pPage2Dlg->UpdateDisplay(pPacket);
	}
	
	TRACE(_T("ProcessSerialReceivedData: 已更新所有控件显示\n"));
#endif
}

// 计算校验和函数 (非必须，仅用于数据完整性校验)
uint8_t CFWGCSDlgDlg::calculateChecksum(const void* data, size_t len) {
    const uint8_t *bytes = (const uint8_t*)data;
    uint8_t sum = 0;
    for(size_t i = 0; i < len; i++) {
        sum += bytes[i];
    }
    return sum;
}

// ============================================================================
// 子对话框分页功能实现
// ============================================================================

// 创建子对话框
BOOL CFWGCSDlgDlg::CreateChildDialogs()
{
	// 检查资源是否存在（避免断言失败）
	HINSTANCE hInst = AfxGetResourceHandle();
	HRSRC hResource1 = FindResource(hInst, MAKEINTRESOURCE(IDD_PAGE1_DIALOG), RT_DIALOG);
	HRSRC hResource2 = FindResource(hInst, MAKEINTRESOURCE(IDD_PAGE2_DIALOG), RT_DIALOG);
	
	if (hResource1 == NULL || hResource2 == NULL)
	{
		// 资源不存在，显示提示信息
		TRACE(_T("警告：子对话框资源不存在！请在资源编辑器中创建 IDD_PAGE1_DIALOG 和 IDD_PAGE2_DIALOG\n"));
		MessageBox(_T("子对话框资源未创建！\n\n请在资源编辑器中创建以下对话框资源：\n- IDD_PAGE1_DIALOG (ID: 130)\n- IDD_PAGE2_DIALOG (ID: 131)\n\n程序将继续运行，但分页功能不可用。"), 
			_T("资源缺失警告"), MB_OK | MB_ICONWARNING);
		// 不返回FALSE，让程序继续运行（使用主对话框的控件）
		return TRUE;  // 返回TRUE但不创建子对话框
	}
	
	// 创建第一页子对话框（独立弹窗，无模态）
	m_pPage1Dlg = new CPage1Dlg(this);  // 传入this作为父窗口，对话框独立
	if (!m_pPage1Dlg->Create(IDD_PAGE1_DIALOG, this))
	{
		TRACE(_T("创建第一页子对话框失败，错误代码: %d\n"), GetLastError());
		delete m_pPage1Dlg;
		m_pPage1Dlg = NULL;
		// 不返回FALSE，让程序继续运行
		return TRUE;
	}
	// 设置为独立窗口，居中显示在主窗口（只设置位置，大小使用资源中的设置）
	CRect rectMain, rectDlg;
	GetWindowRect(&rectMain);
	m_pPage1Dlg->GetWindowRect(&rectDlg);
	int x = rectMain.left + (rectMain.Width() - rectDlg.Width()) / 2;
	int y = rectMain.top + (rectMain.Height() - rectDlg.Height()) / 2;
	m_pPage1Dlg->SetWindowPos(NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
	m_pPage1Dlg->SetWindowText(_T("第一页"));
	
	// 创建第二页子对话框（独立弹窗，无模态）
	m_pPage2Dlg = new CPage2Dlg(this);  // 传入this作为父窗口，对话框独立
	if (!m_pPage2Dlg->Create(IDD_PAGE2_DIALOG, this))
	{
		TRACE(_T("创建第二页子对话框失败，错误代码: %d\n"), GetLastError());
		delete m_pPage2Dlg;
		m_pPage2Dlg = NULL;
		// 如果第一页创建成功但第二页失败，销毁第一页
		if (m_pPage1Dlg != NULL)
		{
			m_pPage1Dlg->DestroyWindow();
			delete m_pPage1Dlg;
			m_pPage1Dlg = NULL;
		}
		// 不返回FALSE，让程序继续运行
		return TRUE;
	}
	// 设置主对话框指针（避免每次使用dynamic_cast）
	m_pPage2Dlg->SetMainDlg(this);
	// 设置为独立窗口，居中显示在主窗口（只设置位置，大小使用资源中的设置）
	m_pPage2Dlg->GetWindowRect(&rectDlg);
	x = rectMain.left + (rectMain.Width() - rectDlg.Width()) / 2;
	y = rectMain.top + (rectMain.Height() - rectDlg.Height()) / 2;
	m_pPage2Dlg->SetWindowPos(NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
	m_pPage2Dlg->SetWindowText(_T("第二页"));
	
	TRACE(_T("子对话框创建成功\n"));
	return TRUE;
}

// 显示指定页面，隐藏其他页面（如果只想显示一个）
// 或者切换指定页面的显示/隐藏状态（允许同时显示多个）
void CFWGCSDlgDlg::ShowPage(int nPage)
{
	if (nPage < 0 || nPage > 1)
		return;
	
	m_nCurrentPage = nPage;
	
	if (nPage == 0)
	{
		// 切换第一页的显示/隐藏状态（不隐藏第二页）
		if (m_pPage1Dlg != NULL && m_pPage1Dlg->GetSafeHwnd() != NULL)
		{
			BOOL bVisible = (m_pPage1Dlg->GetStyle() & WS_VISIBLE) != 0;
			m_pPage1Dlg->ShowWindow(bVisible ? SW_HIDE : SW_SHOW);
		}
	}
	else if (nPage == 1)
	{
		// 切换第二页的显示/隐藏状态（不隐藏第一页）
		if (m_pPage2Dlg != NULL && m_pPage2Dlg->GetSafeHwnd() != NULL)
		{
			BOOL bVisible = (m_pPage2Dlg->GetStyle() & WS_VISIBLE) != 0;
			m_pPage2Dlg->ShowWindow(bVisible ? SW_HIDE : SW_SHOW);
		}
	}
}

// 销毁子对话框
void CFWGCSDlgDlg::DestroyChildDialogs()
{
	if (m_pPage1Dlg != NULL)
	{
		if (m_pPage1Dlg->GetSafeHwnd() != NULL)
		{
			m_pPage1Dlg->DestroyWindow();
		}
		delete m_pPage1Dlg;
		m_pPage1Dlg = NULL;
	}
	
	if (m_pPage2Dlg != NULL)
	{
		if (m_pPage2Dlg->GetSafeHwnd() != NULL)
		{
			m_pPage2Dlg->DestroyWindow();
		}
		delete m_pPage2Dlg;
		m_pPage2Dlg = NULL;
	}
}

// 切换到第一页
void CFWGCSDlgDlg::OnBnClickedPage1()
{
	ShowPage(0);
}

// 切换到第二页
void CFWGCSDlgDlg::OnBnClickedPage2()
{
	ShowPage(1);
}

// 窗口大小改变时调整子对话框位置（独立弹窗不需要调整位置）
// 底部按钮按资源设计尺寸(686x588)的相对比例随当前客户区缩放，保证不同分辨率下位置一致
void CFWGCSDlgDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	
	// 独立弹窗不需要跟随主窗口调整位置
	// 如果需要让弹窗始终居中，可以在这里实现
	ResizeMapWebView(cx, cy);

	// 按设计尺寸与当前客户区的比例重排底部控件，使在不同分辨率/DPI 下保持相对位置
	const int designW = 686;  // 主对话框资源 IDD_FW_GCS_DLG_DIALOG 设计宽度 (DLU)
	const int designH = 588;  // 主对话框资源设计高度 (DLU)
	if (cx <= 0 || cy <= 0) return;

	auto placeBottomControl = [this, cx, cy, designW, designH](UINT nID, int dx, int dy, int dw, int dh)
	{
		CWnd* p = GetDlgItem(nID);
		if (!p || !p->GetSafeHwnd()) return;
		int x = (int)((long)dx * cx / designW);
		int y = (int)((long)dy * cy / designH);
		int w = (int)((long)dw * cx / designW);
		int h = (int)((long)dh * cy / designH);
		if (w < 1) w = 1;
		if (h < 1) h = 1;
		p->SetWindowPos(NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
	};
	// rc资源中底部按钮设计位置与大小：x, y, width, height位置依据窗口展开大小（基于1920x1080比例分配）
	placeBottomControl(IDC_UDPlink,    9, 568, 40, 16);
	placeBottomControl(IDC_SerialLink, 52, 568, 40, 16);
	placeBottomControl(IDC_BTN_PAGE2, 176, 568, 40, 16);
	placeBottomControl(IDC_BTN_PAGE1, 226, 568, 40, 16);
}

void CFWGCSDlgDlg::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (lpMeasureItemStruct && lpMeasureItemStruct->CtlType == ODT_MENU)
	{
		CString text;
		const UINT nMenuID = lpMeasureItemStruct->itemID;
		const DWORD_PTR itemData = lpMeasureItemStruct->itemData;

		// 判断是顶层菜单栏还是子菜单项
		// 方法1：通过 itemData 判断（顶层菜单栏：itemData 是索引0,1,2,3,4；子菜单项：itemData 是菜单ID）
		// 方法2：通过 nMenuID 判断（子菜单项：nMenuID 是菜单ID）
		bool bIsSubMenuItem = (nMenuID >= ID_MENU_CTRL_MODE_MANUAL && nMenuID <= ID_MENU_CHECK_ENGINE) ||
		                       (itemData >= ID_MENU_CTRL_MODE_MANUAL && itemData <= ID_MENU_CHECK_ENGINE);

		if (bIsSubMenuItem)
		{
			// 子菜单项：通过菜单ID获取文本
			CMenu* pSubMenu = NULL;
			UINT nActualMenuID = (nMenuID >= ID_MENU_CTRL_MODE_MANUAL && nMenuID <= ID_MENU_CHECK_ENGINE) 
				? nMenuID : static_cast<UINT>(itemData);
			
			if (nActualMenuID >= ID_MENU_CTRL_MODE_MANUAL && nActualMenuID <= ID_MENU_CTRL_MODE_FULL)
				pSubMenu = m_mainMenu.GetSubMenu(0);
			else if (nActualMenuID >= ID_MENU_MISSION_TEST && nActualMenuID <= ID_MENU_MISSION_LAUNCH_CMD)
				pSubMenu = m_mainMenu.GetSubMenu(1);
			else if (nActualMenuID >= ID_MENU_CHECK_SELF && nActualMenuID <= ID_MENU_CHECK_ENGINE)
				pSubMenu = m_mainMenu.GetSubMenu(2);

			if (pSubMenu)
			{
				pSubMenu->GetMenuString(nActualMenuID, text, MF_BYCOMMAND);
			}
		}
		else
		{
			// 顶层菜单栏项：itemData 是索引（0, 1, 2, 3, 4），包括第一个菜单项（itemData=0）
			UINT nIndex = static_cast<UINT>(itemData);
			m_mainMenu.GetMenuString(nIndex, text, MF_BYPOSITION);
		}

		// 如果文本为空，使用默认值
		if (text.IsEmpty())
		{
			text = _T("菜单项");
		}

		CClientDC dc(this);
		CFont* pFont = m_menuFont.GetSafeHandle()
			? &m_menuFont
			: CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
		CFont* pOld = dc.SelectObject(pFont);
		
		// 使用 DrawText 计算文本尺寸（更准确，特别是对于中文）
		CRect textRect(0, 0, 0, 0);
		dc.DrawText(text, &textRect, DT_SINGLELINE | DT_CALCRECT | DT_LEFT);
		CSize sz(textRect.Width(), textRect.Height());
		
		dc.SelectObject(pOld);

		// 设置宽度和高度
		if (bIsSubMenuItem)
		{
			// 子菜单项：增加宽度以容纳勾选标记和确保文字完整显示
			// 勾选标记区域(24) + 左边距(16) + 右边距(40) = 80
			lpMeasureItemStruct->itemWidth = sz.cx + 100;  // 进一步增加宽度确保中文文字完整显示
		}
		else
		{
			// 顶层菜单栏项：增加更多宽度以确保中文文字完整显示
			lpMeasureItemStruct->itemWidth = sz.cx + 60;  // 大幅增加左右边距，确保"控制模式"等文字完整显示
		}
		// 顶层项高度用系统菜单高度，避免系统栏内文字被裁切；加高由客户区灰色条带体现
		const int menuBarHeight = GetSystemMetrics(SM_CYMENU);
		lpMeasureItemStruct->itemHeight = max(menuBarHeight, sz.cy + 12);
		return;
	}

	CDialogEx::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

void CFWGCSDlgDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct && lpDrawItemStruct->CtlType == ODT_MENU)
	{
		CDC dc;
		dc.Attach(lpDrawItemStruct->hDC);
		CRect rc(lpDrawItemStruct->rcItem);
		// 确保绘制区域足够宽，防止文字被裁剪
		// rc 的宽度应该已经在 OnMeasureItem 中设置好了
		const bool selected = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0;
		const bool checked = (lpDrawItemStruct->itemState & ODS_CHECKED) != 0;
		const UINT nMenuID = lpDrawItemStruct->itemID;

		// 判断是顶层菜单栏还是子菜单项
		// 顶层菜单栏：itemData 是索引（0, 1, 2, 3, 4），且不在菜单ID范围内
		// 子菜单项：itemData 是菜单ID（在 OnInitMenuPopup 中设置），nMenuID 也是菜单ID
		bool bIsSubMenuItem = (nMenuID >= ID_MENU_CTRL_MODE_MANUAL && nMenuID <= ID_MENU_CHECK_ENGINE) ||
		                       (lpDrawItemStruct->itemData >= ID_MENU_CTRL_MODE_MANUAL && lpDrawItemStruct->itemData <= ID_MENU_CHECK_ENGINE);
		
		if (!bIsSubMenuItem)
		{
			// 顶层菜单栏项（包括第一个菜单项，itemData=0）
			const COLORREF bg = selected ? ::GetSysColor(COLOR_HIGHLIGHT) : RGB(100, 100, 100);
			const COLORREF fg = RGB(255, 255, 255);
			dc.FillSolidRect(&rc, bg);

			CString text;
			// itemData 是菜单项的索引（0, 1, 2, 3, 4）
			UINT nIndex = static_cast<UINT>(lpDrawItemStruct->itemData);
			m_mainMenu.GetMenuString(nIndex, text, MF_BYPOSITION);

			// 如果文本为空，尝试通过 MENUITEMINFO 获取
			if (text.IsEmpty())
			{
				MENUITEMINFO mi = {};
				mi.cbSize = sizeof(mi);
				mi.fMask = MIIM_STRING;
				TCHAR szBuffer[256] = {0};
				mi.dwTypeData = szBuffer;
				mi.cch = 255;
				if (m_mainMenu.GetMenuItemInfo(nIndex, &mi, TRUE))
				{
					text = szBuffer;
				}
			}

			// 如果仍然为空，使用默认文本（用于调试）
			if (text.IsEmpty())
			{
				text.Format(_T("菜单%d"), nIndex);
			}

			CFont* pFont = m_menuFont.GetSafeHandle()
				? &m_menuFont
				: CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
			CFont* pOld = dc.SelectObject(pFont);
			dc.SetTextColor(fg);
			dc.SetBkMode(TRANSPARENT);

			// 确保绘制区域足够宽，不裁剪文字
			// rc 的宽度应该已经在 OnMeasureItem 中设置好了
			CRect textRect = rc;
			// 确保左边距足够，避免靠近屏幕边缘时被裁剪
			textRect.left = max(rc.left + 20, rc.left + 10);  // 至少留出10像素左边距
			// 确保右边有足够空间，不裁剪文字
			textRect.right = rc.right;  // 使用完整的 rc.right
			
			// 调试输出（可以后续删除）
			//TRACE(_T("OnDrawItem 顶层菜单: index=%u, text='%s', rc=(%d,%d,%d,%d), textRect=(%d,%d,%d,%d)\n"),
				//nIndex, text, rc.left, rc.top, rc.right, rc.bottom, textRect.left, textRect.top, textRect.right, textRect.bottom);
			
			dc.DrawText(text, &textRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOCLIP);  // DT_NOCLIP 防止文字被裁剪
			dc.SelectObject(pOld);
		}
		else if (nMenuID >= ID_MENU_CTRL_MODE_MANUAL && nMenuID <= ID_MENU_CHECK_ENGINE)
		{
			// 子菜单项：检查是否需要显示深蓝色（选中状态）
			BOOL bShouldHighlight = FALSE;
			if (nMenuID == ID_MENU_CTRL_MODE_MANUAL && m_controlMode_B0 == 0) bShouldHighlight = TRUE;
			else if (nMenuID == ID_MENU_CTRL_MODE_SEMI && m_controlMode_B0 == 1) bShouldHighlight = TRUE;
			else if (nMenuID == ID_MENU_CTRL_MODE_FULL && m_controlMode_B0 == 2) bShouldHighlight = TRUE;
			else if (nMenuID == ID_MENU_MISSION_TEST && m_missionCommand_B0 == 0) bShouldHighlight = TRUE;
			else if (nMenuID == ID_MENU_MISSION_LAUNCH_PROC && m_missionCommand_B0 == 1) bShouldHighlight = TRUE;
			else if (nMenuID == ID_MENU_MISSION_BIND_PARAM && m_missionCommand_B2 == 1) bShouldHighlight = TRUE;
			else if (nMenuID == ID_MENU_MISSION_LAUNCH_CMD && m_missionCommand_B5 == 1) bShouldHighlight = TRUE;
			else if (nMenuID == ID_MENU_CHECK_SELF && m_missionCommand_B1 == 1) bShouldHighlight = TRUE;
			else if (nMenuID == ID_MENU_CHECK_SURFACE && m_missionCommand_B3 == 1) bShouldHighlight = TRUE;
			else if (nMenuID == ID_MENU_CHECK_ENGINE && m_missionCommand_B4 == 1) bShouldHighlight = TRUE;

			// 设置背景色：选中状态用深蓝色，悬停用系统高亮色
			COLORREF bg;
			if (bShouldHighlight)
				bg = RGB(0, 0, 139);  // 深蓝色
			else if (selected)
				bg = ::GetSysColor(COLOR_HIGHLIGHT);
			else
				bg = RGB(255, 255, 255);  // 白色背景

			const COLORREF fg = (bShouldHighlight || selected) ? RGB(255, 255, 255) : RGB(0, 0, 0);
			dc.FillSolidRect(&rc, bg);

			// 获取菜单项文本（通过菜单ID从对应的子菜单获取）
			CString text;
			CMenu* pSubMenu = NULL;
			if (nMenuID >= ID_MENU_CTRL_MODE_MANUAL && nMenuID <= ID_MENU_CTRL_MODE_FULL)
				pSubMenu = m_mainMenu.GetSubMenu(0);
			else if (nMenuID >= ID_MENU_MISSION_TEST && nMenuID <= ID_MENU_MISSION_LAUNCH_CMD)
				pSubMenu = m_mainMenu.GetSubMenu(1);
			else if (nMenuID >= ID_MENU_CHECK_SELF && nMenuID <= ID_MENU_CHECK_ENGINE)
				pSubMenu = m_mainMenu.GetSubMenu(2);

			if (pSubMenu)
			{
				pSubMenu->GetMenuString(nMenuID, text, MF_BYCOMMAND);
			}
			else
			{
				// 如果无法获取，使用菜单ID对应的默认文本
				text = _T("菜单项");
			}

			CFont* pFont = m_menuFont.GetSafeHandle()
				? &m_menuFont
				: CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
			CFont* pOld = dc.SelectObject(pFont);
			dc.SetTextColor(fg);
			dc.SetBkMode(TRANSPARENT);

			// 绘制勾选标记（如果选中）
			if (bShouldHighlight)
			{
				rc.left += 24;  // 为勾选标记留出更多空间
				// 绘制勾选标记 "✓"
				CRect checkRect(rc.left - 22, rc.top, rc.left - 6, rc.bottom);
				dc.DrawText(_T("✓"), &checkRect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
			}
			else
			{
				rc.left += 24;  // 保持对齐
			}

			dc.DrawText(text, &rc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOCLIP);  // 添加 DT_NOCLIP 防止文字被裁剪
			dc.SelectObject(pOld);
		}

		dc.Detach();
		return;
	}

	CDialogEx::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

// 初始化菜单弹出时，将子菜单项设置为 owner-drawn
void CFWGCSDlgDlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	CDialogEx::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	if (bSysMenu || pPopupMenu == NULL)
		return;

	// 只处理控制指令相关的子菜单（前3个子菜单）
	if (nIndex >= 0 && nIndex <= 2)
	{
		const int nItemCount = pPopupMenu->GetMenuItemCount();
		for (int i = 0; i < nItemCount; i++)
		{
			UINT nMenuID = pPopupMenu->GetMenuItemID(i);
			// 只处理控制指令相关的菜单项（排除分隔符和弹出菜单）
			if (nMenuID != (UINT)-1 && nMenuID != 0 && 
				nMenuID >= ID_MENU_CTRL_MODE_MANUAL && nMenuID <= ID_MENU_CHECK_ENGINE)
			{
				MENUITEMINFO mi = {};
				mi.cbSize = sizeof(mi);
				mi.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_ID;
				mi.fType = MFT_OWNERDRAW;
				mi.dwItemData = nMenuID;  // 保存菜单ID用于绘制和测量
				mi.wID = nMenuID;  // 确保菜单ID正确设置
				pPopupMenu->SetMenuItemInfo(i, &mi, TRUE);
			}
		}
	}
}

void CFWGCSDlgDlg::OnNcPaint()
{
	CDialogEx::OnNcPaint();

	// 在菜单栏上下绘制分界线（非客户区绘制）
	CWindowDC dc(this);
	CRect rcWindow;
	CRect rcClient;
	GetWindowRect(&rcWindow);
	GetClientRect(&rcClient);
	ClientToScreen(&rcClient);

	// 客户区顶边在屏幕坐标中的 Y
	const int yClientTop = rcClient.top - rcWindow.top;
	const COLORREF lineColor = ::GetSysColor(COLOR_3DSHADOW);

	// 尝试获取菜单栏真实矩形，确保顶部线可见（灰色条高度由系统菜单栏 rcBar 决定）
	MENUBARINFO mbi = {};
	mbi.cbSize = sizeof(mbi);
	if (GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi))
	{
		const int yMenuTop = mbi.rcBar.top - rcWindow.top;
		const int yMenuBottom = mbi.rcBar.bottom - rcWindow.top;
		// 整条菜单栏灰色填充（半透明效果用实色近似）
		dc.FillSolidRect(0, yMenuTop, rcWindow.Width(), yMenuBottom - yMenuTop, RGB(100, 100, 100));
		// 菜单栏顶部边
		if (yMenuTop >= 0)
		{
			dc.FillSolidRect(0, yMenuTop, rcWindow.Width(), 1, lineColor);
		}
		// 菜单栏底部边
		dc.FillSolidRect(0, yMenuBottom, rcWindow.Width(), 1, lineColor);
	}
	else
	{
		// 兜底：只画客户区顶边
		dc.FillSolidRect(0, yClientTop, rcWindow.Width(), 1, lineColor);
	}
}

#if FW_GCS_WITH_WEBVIEW2
void CFWGCSDlgDlg::InitMapWebView()
{
	if (m_hMapHostWnd != nullptr)
	{
		return;
	}

	CRect clientRect;
	GetClientRect(&clientRect);
	m_hMapHostWnd = ::CreateWindowExW(
		0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		0, 0, clientRect.Width(), clientRect.Height(),
		m_hWnd, nullptr, AfxGetInstanceHandle(), nullptr);
	if (m_hMapHostWnd == nullptr)
	{
		LogMap(L"[Map] Create host window failed (GetLastError=%lu)", ::GetLastError());
		return;
	}
	LogMap(L"[Map] Host window created");

	auto envCompletedHandler = Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
		[this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
		{
			if (FAILED(result) || env == nullptr)
			{
				LogMap(L"[Map] WebView2 Environment failed: 0x%08X", result);
				return S_OK;
			}
			m_webViewEnvironment = env;
			LogMap(L"[Map] WebView2 Environment OK");

			auto controllerHandler = Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
				[this](HRESULT controllerResult, ICoreWebView2Controller* controller) -> HRESULT
				{
					if (FAILED(controllerResult) || controller == nullptr)
					{
						LogMap(L"[Map] WebView2 Controller failed: 0x%08X", controllerResult);
						return S_OK;
					}

					m_webViewController = controller;
					m_webViewController->get_CoreWebView2(&m_webView);
					LogMap(L"[Map] WebView2 Controller OK");

					// 设置深色不透明背景（兼容旧版 SDK：尝试 ICoreWebView2Controller2）
					{
						Microsoft::WRL::ComPtr<ICoreWebView2Controller2> ctrl2;
						if (SUCCEEDED(m_webViewController.As(&ctrl2)) && ctrl2)
						{
							COREWEBVIEW2_COLOR bg{};
							bg.A = 255;
							bg.R = 0x0b;
							bg.G = 0x0f;
							bg.B = 0x14;
							ctrl2->put_DefaultBackgroundColor(bg);
						}
					}

					RECT bounds{};
					::GetClientRect(m_hMapHostWnd, &bounds);
					m_webViewController->put_Bounds(bounds);
					m_webViewController->put_IsVisible(TRUE);

					if (m_webView != nullptr)
					{
						LogMap(L"[Map] WebView2 CoreWebView2 OK");
						// 拦截离线瓦片请求
						m_webView->AddWebResourceRequestedFilter(
							L"https://tiles.local/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE);

						m_webView->add_WebResourceRequested(
							Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(
								[this](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT
								{
									UNREFERENCED_PARAMETER(sender);
									Microsoft::WRL::ComPtr<ICoreWebView2WebResourceRequest> request;
									if (FAILED(args->get_Request(&request)) || request == nullptr)
									{
										return S_OK;
									}

									LPWSTR uri = nullptr;
									if (FAILED(request->get_Uri(&uri)) || uri == nullptr)
									{
										return S_OK;
									}
									std::wstring url(uri);
									::CoTaskMemFree(uri);

									int z = 0, x = 0, y = 0;
									if (!ParseTileUrl(url, z, x, y))
									{
										return S_OK;
									}

									std::vector<unsigned char> tileData;
									CString mimeType;
									bool isGzip = false;
									bool tileFound = false;
									const wchar_t* source = nullptr;
									
									// 优先从局部精细地图获取瓦片
									if (GetTileFromLocalMaps(z, x, y, tileData, mimeType, isGzip))
									{
										tileFound = true;
										source = L"LOCAL";
									}
									// 如果局部地图没有，则从全国底图获取
									else if (m_mbtilesReader.GetTile(z, x, y, tileData, mimeType, isGzip))
									{
										tileFound = true;
										source = L"BASE";
									}
									
									static int s_tileServedCount = 0;
									static int s_localServedCount = 0;
									static int s_baseServedCount = 0;
									
									if (tileFound)
									{
										s_tileServedCount++;
										if (source == L"LOCAL")
										{
											s_localServedCount++;
										}
										else
										{
											s_baseServedCount++;
										}
										
										if (s_tileServedCount <= 20 || (s_tileServedCount % 100 == 0))
										{
											LogMap(L"[Map] [TileRequest] z=%d x=%d y=%d -> %s (size=%d, mime=%s) [Stats: LOCAL=%d BASE=%d TOTAL=%d]",
												z, x, y, source, tileData.size(), mimeType.GetString(),
												s_localServedCount, s_baseServedCount, s_tileServedCount);
										}
									}
									else
									{
										static int s_missingCount = 0;
										s_missingCount++;
										if (s_missingCount <= 20 || (s_missingCount % 50 == 0))
										{
											LogMap(L"[Map] [TileRequest] ✗ Missing tile z=%d x=%d y=%d (missing count: %d)", 
												z, x, y, s_missingCount);
										}
										Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> response;
										m_webViewEnvironment->CreateWebResourceResponse(nullptr, 404, L"Not Found", L"", &response);
										args->put_Response(response.Get());
										return S_OK;
									}

									HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, tileData.size());
									if (hGlobal == nullptr)
									{
										return S_OK;
									}
									void* buffer = ::GlobalLock(hGlobal);
									if (buffer != nullptr)
									{
										memcpy(buffer, tileData.data(), tileData.size());
										::GlobalUnlock(hGlobal);
									}

									Microsoft::WRL::ComPtr<IStream> stream;
									if (FAILED(::CreateStreamOnHGlobal(hGlobal, TRUE, &stream)))
									{
										return S_OK;
									}

									CString headers;
									if (isGzip)
									{
										headers.Format(_T("Content-Type: %s\r\nContent-Encoding: gzip\r\n"), mimeType.GetString());
									}
									else
									{
										headers.Format(_T("Content-Type: %s\r\n"), mimeType.GetString());
									}
									Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> response;
									m_webViewEnvironment->CreateWebResourceResponse(
										stream.Get(), 200, L"OK", headers, &response);
									args->put_Response(response.Get());
									return S_OK;
								}).Get(),
							nullptr);

						CString html = BuildMapHtml();
						m_webView->NavigateToString(html);
						LogMap(L"[Map] NavigateToString done");
					}

					// 保证地图在最底层显示
					::SetWindowPos(m_hMapHostWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
					return S_OK;
				});

			env->CreateCoreWebView2Controller(m_hMapHostWnd, controllerHandler.Get());
			return S_OK;
		});

	CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, envCompletedHandler.Get());
	LogMap(L"[Map] CreateCoreWebView2EnvironmentWithOptions called");
}
#else
void CFWGCSDlgDlg::InitMapWebView()
{
}
#endif

void CFWGCSDlgDlg::ResizeMapWebView(int cx, int cy)
{
	// 在客户区顶部始终加一段灰色条带，使“总灰色高度”= m_menuItemHeight（不依赖系统是否真的加高菜单栏）
	const int systemMenuHeight = GetSystemMetrics(SM_CYMENU);
	int extraHeight = max(0, m_menuItemHeight - systemMenuHeight);
	m_menuBarExtraHeight = extraHeight;

	const int mapY = extraHeight;
	const int mapCy = cy - extraHeight;
	if (mapCy <= 0)
		return;

	if (m_hMapHostWnd != nullptr)
	{
		::SetWindowPos(m_hMapHostWnd, HWND_BOTTOM, 0, mapY, cx, mapCy, SWP_NOACTIVATE);
	}
#if FW_GCS_WITH_WEBVIEW2
	if (m_webViewController)
	{
		RECT bounds{};
		bounds.left = 0;
		bounds.top = 0;
		bounds.right = cx;
		bounds.bottom = mapCy;  // 相对于地图宿主窗口，高度为客户区减去顶部灰色条带
		m_webViewController->put_Bounds(bounds);
	}
#endif
}

// 辅助函数：从 exe 嵌入资源（RCDATA）读取内容为 UTF-8/ANSI 字符串
static CStringA ReadResourceContentA(HMODULE hModule, UINT resourceId)
{
	CStringA content;
	HRSRC hRes = ::FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
	if (!hRes)
		return content;
	HGLOBAL hLoaded = ::LoadResource(hModule, hRes);
	if (!hLoaded)
		return content;
	const char* pData = (const char*)::LockResource(hLoaded);
	DWORD size = ::SizeofResource(hModule, hRes);
	if (!pData || size == 0 || size > 10 * 1024 * 1024)
		return content;
	content.SetString(pData, (int)size);
	return content;
}

CString CFWGCSDlgDlg::BuildMapHtml() const
{
	// 从 exe 嵌入资源加载 map.html 和 script.js（无需 exe 同目录下的 Scripts 文件夹）
	HMODULE hInst = AfxGetResourceHandle();
	CStringA htmlTemplate = ReadResourceContentA(hInst, IDR_MAP_HTML);
	CStringA jsContent = ReadResourceContentA(hInst, IDR_SCRIPT_JS);

	if (htmlTemplate.IsEmpty())
	{
		CStringA errorHtml(
			"<!doctype html><html><body style='background:#1a1a1a;color:#ff6b6b;font-family:Segoe UI;padding:20px;'>"
			"<h2>Error: Embedded map resources not found</h2>"
			"<p>map.html/script.js should be compiled into the executable. Rebuild the project.</p>"
			"</body></html>");
		return CString(errorHtml);
	}

	// 构建配置字符串
	CStringA config;
	config.Format(
		"const mapConfig={tileUrl:\"https://tiles.local/tiles/{z}/{x}/{y}\",minZoom:%d,maxZoom:%d,zoom:%d,centerLat:%0.8f,centerLng:%0.8f,hasCenter:%s};",
		m_mbtilesMetadata.minZoom,
		m_mbtilesMetadata.maxZoom,
		m_mbtilesMetadata.defaultZoom,
		m_mbtilesMetadata.centerLat,
		m_mbtilesMetadata.centerLng,
		m_mbtilesMetadata.hasCenter ? "true" : "false");

	// 构建状态文本
	CStringA status;
	if (!m_mbtilesReader.IsOpen())
	{
		CStringA pathA(m_mbtilesPath);
		status.Format("const statusText='MBTiles open failed: %s';", pathA.GetString());
	}
	else if (m_mbtilesMetadata.format.CompareNoCase(_T("pbf")) == 0)
	{
		status = "const statusText='MBTiles format is PBF (vector). Current renderer supports raster only.';";
	}
	else
	{
		status = "const statusText='Offline Map Loaded';";
	}

	// 替换占位符
	CStringA html = htmlTemplate;
	html.Replace("/*{{MAP_CONFIG}}*/", config);
	html.Replace("/*{{STATUS_TEXT}}*/", status);
	html.Replace("/*{{MAP_JS}}*/", jsContent);

	return CString(html);
}

CString CFWGCSDlgDlg::GetDefaultMbtilesPath() const
{
	wchar_t buffer[MAX_PATH] = {};
	DWORD len = ::GetEnvironmentVariableW(L"FW_GCS_MBTILES", buffer, MAX_PATH);
	if (len > 0 && len < MAX_PATH)
	{
		return CString(buffer);
	}

	wchar_t exePath[MAX_PATH] = {};
	::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	CString exeDir(exePath);
	int pos = exeDir.ReverseFind(L'\\');
	if (pos >= 0)
	{
		exeDir = exeDir.Left(pos);
	}

	// 优先查找 OUTPUT_FILE.mbtiles（QGIS 默认输出文件名）
	CString outputFile = exeDir + _T("\\maps\\OUTPUT_FILE.mbtiles");
	DWORD attr = ::GetFileAttributesW(outputFile);
	if (attr != INVALID_FILE_ATTRIBUTES)
	{
		return outputFile;
	}

	// 如果 maps 文件夹不存在，尝试直接在 exe 目录查找
	outputFile = exeDir + _T("\\OUTPUT_FILE.mbtiles");
	attr = ::GetFileAttributesW(outputFile);
	if (attr != INVALID_FILE_ATTRIBUTES)
	{
		return outputFile;
	}

	// 回退到原来的文件名
	CString oldFile = exeDir + _T("\\maps\\osm-2020-02-10-v3.11_china_nanchang.mbtiles");
	attr = ::GetFileAttributesW(oldFile);
	if (attr != INVALID_FILE_ATTRIBUTES)
	{
		return oldFile;
	}

	// 如果都不存在，返回 OUTPUT_FILE.mbtiles 作为默认（用户需要创建）
	return exeDir + _T("\\maps\\OUTPUT_FILE.mbtiles");
}

// 加载LocalMaps文件夹内的所有.mbtiles文件
void CFWGCSDlgDlg::LoadLocalMaps()
{
	m_localMaps.clear();
	
	wchar_t exePath[MAX_PATH] = {};
	::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	CString exeDir(exePath);
	int pos = exeDir.ReverseFind(L'\\');
	if (pos >= 0)
	{
		exeDir = exeDir.Left(pos);
	}
	
	CString localMapsDir = exeDir + _T("\\maps\\LocalMaps");
	LogMap(L"[Map] [LoadLocalMaps] Searching directory: %s", localMapsDir.GetString());
	
	DWORD attr = ::GetFileAttributesW(localMapsDir);
	if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		LogMap(L"[Map] [LoadLocalMaps] Directory not found or not a directory: %s", localMapsDir.GetString());
		return;
	}
	
	CString searchPath = localMapsDir + _T("\\*.mbtiles");
	LogMap(L"[Map] [LoadLocalMaps] Search pattern: %s", searchPath.GetString());
	
	WIN32_FIND_DATAW findData;
	HANDLE hFind = ::FindFirstFileW(searchPath, &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		LogMap(L"[Map] [LoadLocalMaps] No .mbtiles files found in LocalMaps (FindFirstFileW failed)");
		return;
	}
	
	int fileCount = 0;
	do
	{
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			fileCount++;
			CString filePath = localMapsDir + _T("\\") + findData.cFileName;
			LogMap(L"[Map] [LoadLocalMaps] Found file #%d: %s", fileCount, findData.cFileName);
			
			LocalMapInfo localMap;
			localMap.path = filePath;
			
			// 使用临时reader读取metadata，然后关闭
			CMbtilesReader tempReader;
			CString errorMsg;
			LogMap(L"[Map] [LoadLocalMaps] Attempting to open: %s", filePath.GetString());
			if (tempReader.Open(filePath, errorMsg))
			{
				LogMap(L"[Map] [LoadLocalMaps] File opened successfully, reading metadata...");
				if (tempReader.GetMetadata(localMap.metadata))
				{
					m_localMaps.push_back(localMap);
					LogMap(L"[Map] [LoadLocalMaps] ✓ Loaded local map #%d: %s", m_localMaps.size(), findData.cFileName);
					LogMap(L"[Map] [LoadLocalMaps]   - Zoom range: %d..%d", 
						localMap.metadata.minZoom, localMap.metadata.maxZoom);
					LogMap(L"[Map] [LoadLocalMaps]   - Bounds: [%.6f, %.6f] to [%.6f, %.6f] (hasBounds=%s)", 
						localMap.metadata.minLng, localMap.metadata.minLat,
						localMap.metadata.maxLng, localMap.metadata.maxLat,
						localMap.metadata.hasBounds ? L"yes" : L"no");
					LogMap(L"[Map] [LoadLocalMaps]   - Format: %s", localMap.metadata.format.GetString());
				}
				else
				{
					LogMap(L"[Map] [LoadLocalMaps] ✗ Failed to get metadata from: %s", findData.cFileName);
				}
				tempReader.Close();
			}
			else
			{
				LogMap(L"[Map] [LoadLocalMaps] ✗ Failed to open local map: %s - Error: %s", 
					findData.cFileName, errorMsg.GetString());
			}
		}
	} while (::FindNextFileW(hFind, &findData));
	
	::FindClose(hFind);
	LogMap(L"[Map] [LoadLocalMaps] Scan complete: found %d file(s), successfully loaded %d local map(s)", 
		fileCount, m_localMaps.size());
}

// 瓦片坐标转经纬度（返回瓦片中心点的经纬度）
bool CFWGCSDlgDlg::TileToLngLat(int zoom, int x, int y, double& lng, double& lat)
{
	if (zoom < 0 || x < 0 || y < 0)
	{
		return false;
	}
	const int maxIndex = 1 << zoom;
	if (x >= maxIndex || y >= maxIndex)
	{
		return false;
	}
	
	// 计算瓦片中心点的经纬度
	const double n = 1 << zoom;
	lng = (x + 0.5) / n * 360.0 - 180.0;
	const double lat_rad = atan(sinh(M_PI * (1.0 - 2.0 * (y + 0.5) / n)));
	lat = lat_rad * 180.0 / M_PI;
	return true;
}

// 计算瓦片的边界（四个角的经纬度）
static void TileToBounds(int zoom, int x, int y, double& minLng, double& minLat, double& maxLng, double& maxLat)
{
	const double n = 1 << zoom;
	// 左边界（x）
	minLng = x / n * 360.0 - 180.0;
	// 右边界（x+1）
	maxLng = (x + 1) / n * 360.0 - 180.0;
	// 上边界（y）- 注意：Web Mercator中y=0在顶部
	const double lat1_rad = atan(sinh(M_PI * (1.0 - 2.0 * y / n)));
	const double lat2_rad = atan(sinh(M_PI * (1.0 - 2.0 * (y + 1) / n)));
	maxLat = lat1_rad * 180.0 / M_PI;  // 上边界（y值小）
	minLat = lat2_rad * 180.0 / M_PI;  // 下边界（y值大）
}

// 检查两个矩形是否重叠
static bool BoundsOverlap(double minLng1, double minLat1, double maxLng1, double maxLat1,
						  double minLng2, double minLat2, double maxLng2, double maxLat2)
{
	// 检查是否不重叠：一个矩形完全在另一个的左边、右边、上边或下边
	if (maxLng1 < minLng2 || minLng1 > maxLng2 || maxLat1 < minLat2 || minLat1 > maxLat2)
	{
		return false;
	}
	return true;
}

// 从局部地图获取瓦片（如果该瓦片在局部地图范围内）
bool CFWGCSDlgDlg::GetTileFromLocalMaps(int zoom, int x, int y, std::vector<unsigned char>& outData, CString& outMimeType, bool& outIsGzip)
{
	static int s_tileRequestCount = 0;
	static int s_localMapHitCount = 0;
	s_tileRequestCount++;
	
	if (m_localMaps.empty())
	{
		if (s_tileRequestCount <= 5 || (s_tileRequestCount % 100 == 0))
		{
			LogMap(L"[Map] [GetTileFromLocalMaps] z=%d x=%d y=%d - No local maps available", zoom, x, y);
		}
		return false;
	}
	
	// 计算瓦片的边界
	double tileMinLng, tileMinLat, tileMaxLng, tileMaxLat;
	TileToBounds(zoom, x, y, tileMinLng, tileMinLat, tileMaxLng, tileMaxLat);
	
	if (s_tileRequestCount <= 10 || (s_tileRequestCount % 50 == 0))
	{
		LogMap(L"[Map] [GetTileFromLocalMaps] Request #%d: z=%d x=%d y=%d, tile bounds: [%.6f,%.6f] to [%.6f,%.6f]",
			s_tileRequestCount, zoom, x, y, tileMinLng, tileMinLat, tileMaxLng, tileMaxLat);
	}
	
	// 遍历所有局部地图，查找包含该瓦片的
	for (size_t i = 0; i < m_localMaps.size(); ++i)
	{
		LocalMapInfo& localMap = m_localMaps[i];
		
		if (s_tileRequestCount <= 10 || (s_tileRequestCount % 50 == 0))
		{
			LogMap(L"[Map] [GetTileFromLocalMaps] Checking local map #%d: zoom range %d..%d, bounds [%.6f,%.6f] to [%.6f,%.6f]",
				i + 1, localMap.metadata.minZoom, localMap.metadata.maxZoom,
				localMap.metadata.minLng, localMap.metadata.minLat,
				localMap.metadata.maxLng, localMap.metadata.maxLat);
		}
		
		// 检查zoom是否在范围内
		if (zoom < localMap.metadata.minZoom || zoom > localMap.metadata.maxZoom)
		{
			if (s_tileRequestCount <= 10 || (s_tileRequestCount % 50 == 0))
			{
				LogMap(L"[Map] [GetTileFromLocalMaps]   Zoom %d out of range [%d..%d] for map #%d",
					zoom, localMap.metadata.minZoom, localMap.metadata.maxZoom, i + 1);
			}
			continue;
		}
		
		// 检查bounds（如果局部地图有bounds信息）
		bool overlaps = true;  // 如果没有bounds信息，默认认为重叠
		if (localMap.metadata.hasBounds)
		{
			overlaps = BoundsOverlap(tileMinLng, tileMinLat, tileMaxLng, tileMaxLat,
									 localMap.metadata.minLng, localMap.metadata.minLat,
									 localMap.metadata.maxLng, localMap.metadata.maxLat);
			
			if (s_tileRequestCount <= 10 || (s_tileRequestCount % 50 == 0) || overlaps)
			{
				LogMap(L"[Map] [GetTileFromLocalMaps]   Bounds check: tile[%.6f,%.6f to %.6f,%.6f] vs map[%.6f,%.6f to %.6f,%.6f] -> %s",
					tileMinLng, tileMinLat, tileMaxLng, tileMaxLat,
					localMap.metadata.minLng, localMap.metadata.minLat,
					localMap.metadata.maxLng, localMap.metadata.maxLat,
					overlaps ? L"OVERLAP" : L"NO OVERLAP");
			}
			
			if (!overlaps)
			{
				continue;
			}
		}
		else
		{
			if (s_tileRequestCount <= 10 || (s_tileRequestCount % 50 == 0))
			{
				LogMap(L"[Map] [GetTileFromLocalMaps]   Map #%d has no bounds info, checking tile existence...", i + 1);
			}
		}
		
		// 每次请求使用独立的临时 reader，避免多线程共享同一连接导致 rc=21
		CMbtilesReader tempReader;
		CString openErr;
		if (!tempReader.Open(localMap.path, openErr))
		{
			if (s_tileRequestCount <= 10 || (s_tileRequestCount % 50 == 0))
			{
				LogMap(L"[Map] [GetTileFromLocalMaps]   Open local map failed #%d: %s", i + 1, openErr.GetString());
			}
			continue;
		}

		// 尝试从该局部地图获取瓦片
		// 注意：MbtilesReader内部会将Y转换为TMS坐标系
		const int maxIndex = 1 << zoom;
		const int tmsY = (maxIndex - 1) - y;
		if (s_tileRequestCount <= 10 || (s_tileRequestCount % 50 == 0) || overlaps)
		{
			LogMap(L"[Map] [GetTileFromLocalMaps]   Querying tile: z=%d x=%d y=%d (Web Mercator) -> TMS y=%d (maxIndex=%d)", 
				zoom, x, y, tmsY, maxIndex);
		}
		
		if (tempReader.GetTile(zoom, x, y, outData, outMimeType, outIsGzip))
		{
			s_localMapHitCount++;
			LogMap(L"[Map] [GetTileFromLocalMaps] ✓✓✓ FOUND in local map #%d (z=%d x=%d y=%d, TMS y=%d) size=%d bytes [Hit rate: %d/%d=%.1f%%]",
				i + 1, zoom, x, y, tmsY, outData.size(), s_localMapHitCount, s_tileRequestCount,
				s_tileRequestCount > 0 ? (100.0 * s_localMapHitCount / s_tileRequestCount) : 0.0);
			return true;
		}
		else
		{
			// 如果bounds重叠但没找到，总是输出日志
			if (overlaps || s_tileRequestCount <= 10 || (s_tileRequestCount % 50 == 0))
			{
				LogMap(L"[Map] [GetTileFromLocalMaps]   ✗✗✗ Tile NOT FOUND in local map #%d (z=%d x=%d y=%d, TMS y=%d) - bounds overlapped but tile missing!",
					i + 1, zoom, x, y, tmsY);
			}
		}
	}
	
	if (s_tileRequestCount <= 10 || (s_tileRequestCount % 100 == 0))
	{
		LogMap(L"[Map] [GetTileFromLocalMaps] ✗ Not found in any local map (z=%d x=%d y=%d)", zoom, x, y);
	}
	return false;
}

// UDP通信设置菜单项处理函数
void CFWGCSDlgDlg::OnMenuUdpSettings()
{
	CUdpSettingsDlg dlg(this);
	
	// 设置当前UDP配置值（从当前配置读取，而不是宏定义）
	dlg.SetLocalIP(m_strUdpLocalIP);
	dlg.SetLocalPort(m_nUdpLocalPort);
	dlg.SetRemoteIP(m_strUdpRemoteIP);
	dlg.SetRemotePort(m_nUdpRemotePort);
	
	// 显示对话框
	if (dlg.DoModal() == IDOK)
	{
		// 用户点击了确定按钮，获取设置值
		CString strLocalIP = dlg.GetLocalIP();
		int nLocalPort = dlg.GetLocalPort();
		CString strRemoteIP = dlg.GetRemoteIP();
		int nRemotePort = dlg.GetRemotePort();
		
		// 检查是否有变化
		BOOL bChanged = (strLocalIP != m_strUdpLocalIP) || 
		                (nLocalPort != m_nUdpLocalPort) ||
		                (strRemoteIP != m_strUdpRemoteIP) ||
		                (nRemotePort != m_nUdpRemotePort);
		
		if (bChanged)
		{
			// 更新配置
			m_strUdpLocalIP = strLocalIP;
			m_nUdpLocalPort = nLocalPort;
			m_strUdpRemoteIP = strRemoteIP;
			m_nUdpRemotePort = nRemotePort;
			
			// 如果UDP已连接，提示用户需要重新连接以应用新设置
			if (m_bUdpConnected)
			{
				CString strMsg;
				strMsg.Format(_T("UDP设置已更新：\n\n本机IP: %s\n本机端口: %d\n远程IP: %s\n远程端口: %d\n\n注意：当前UDP已连接，新设置将在下次连接时生效。\n是否立即断开并重新连接？"), 
					strLocalIP, nLocalPort, strRemoteIP, nRemotePort);
				
				if (MessageBox(strMsg, _T("UDP设置"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					// 断开当前连接
					DisconnectUdp();
					// 重新连接
					if (ConnectUdp())
					{
						MessageBox(_T("UDP已重新连接，新设置已生效！"), _T("UDP设置"), MB_OK | MB_ICONINFORMATION);
					}
					else
					{
						MessageBox(_T("UDP重新连接失败，请检查设置后手动连接。"), _T("UDP设置"), MB_OK | MB_ICONWARNING);
					}
				}
			}
			else
			{
				CString strMsg;
				strMsg.Format(_T("UDP设置已更新（本次运行有效）：\n\n本机IP: %s\n本机端口: %d\n远程IP: %s\n远程端口: %d\n\n设置将在下次连接时生效。"), 
					strLocalIP, nLocalPort, strRemoteIP, nRemotePort);
				MessageBox(strMsg, _T("UDP设置"), MB_OK | MB_ICONINFORMATION);
			}
		}
		else
		{
			// 没有变化，不需要保存
			MessageBox(_T("UDP设置未更改。"), _T("UDP设置"), MB_OK | MB_ICONINFORMATION);
		}
	}
}

// 串口通信设置菜单项处理函数
void CFWGCSDlgDlg::OnMenuSerialSettings()
{
	CSerialSettingsDlg dlg(this);
	dlg.DoModal();  // 占位对话框会自动显示提示信息
}

// ============================================================
// 控制模式菜单项处理函数
// ============================================================
void CFWGCSDlgDlg::OnMenuCtrlModeManual()
{
	m_controlMode_B0 = 0;  // 手动遥控模式
	SendControlCommand();
}

void CFWGCSDlgDlg::OnMenuCtrlModeSemi()
{
	m_controlMode_B0 = 1;  // 半自主模式
	SendControlCommand();
}

void CFWGCSDlgDlg::OnMenuCtrlModeFull()
{
	m_controlMode_B0 = 2;  // 全自主模式
	SendControlCommand();
}

// ============================================================
// 任务指令菜单项处理函数
// ============================================================
void CFWGCSDlgDlg::OnMenuMissionTest()
{
	m_missionCommand_B0 = 0;  // 地面测试流程
	SendControlCommand();
}

void CFWGCSDlgDlg::OnMenuMissionLaunchProc()
{
	m_missionCommand_B0 = 1;  // 发射流程
	SendControlCommand();
}

void CFWGCSDlgDlg::OnMenuMissionBindParam()
{
	// 参数装订指令（切换状态）
	m_missionCommand_B2 = (m_missionCommand_B2 == 0) ? 1 : 0;
	SendControlCommand();
}

void CFWGCSDlgDlg::OnMenuMissionLaunchCmd()
{
	// 发射指令（切换状态）
	m_missionCommand_B5 = (m_missionCommand_B5 == 0) ? 1 : 0;
	SendControlCommand();
}

// ============================================================
// 自检指令菜单项处理函数（B1, B3, B4 切换逻辑：点击已激活项取消激活，点击未激活项激活并互斥其他项）
// ============================================================
void CFWGCSDlgDlg::OnMenuCheckSelf()
{
	// 自检指令：如果当前已激活（B1=1），则取消激活（B1=0）
	// 如果当前未激活（B1=0），则激活（B1=1）并互斥其他项（B3=0, B4=0）
	if (m_missionCommand_B1 == 1)
	{
		m_missionCommand_B1 = 0;  // 取消激活
	}
	else
	{
		m_missionCommand_B1 = 1;  // 激活
		m_missionCommand_B3 = 0;  // 互斥：取消其他项
		m_missionCommand_B4 = 0;  // 互斥：取消其他项
	}
	SendControlCommand();
}

void CFWGCSDlgDlg::OnMenuCheckSurface()
{
	// 舵面检查指令：如果当前已激活（B3=1），则取消激活（B3=0）
	// 如果当前未激活（B3=0），则激活（B3=1）并互斥其他项（B1=0, B4=0）
	if (m_missionCommand_B3 == 1)
	{
		m_missionCommand_B3 = 0;  // 取消激活
	}
	else
	{
		m_missionCommand_B3 = 1;  // 激活
		m_missionCommand_B1 = 0;  // 互斥：取消其他项
		m_missionCommand_B4 = 0;  // 互斥：取消其他项
	}
	SendControlCommand();
}

void CFWGCSDlgDlg::OnMenuCheckEngine()
{
	// 发动机检查指令：如果当前已激活（B4=1），则取消激活（B4=0）
	// 如果当前未激活（B4=0），则激活（B4=1）并互斥其他项（B1=0, B3=0）
	if (m_missionCommand_B4 == 1)
	{
		m_missionCommand_B4 = 0;  // 取消激活
	}
	else
	{
		m_missionCommand_B4 = 1;  // 激活
		m_missionCommand_B1 = 0;  // 互斥：取消其他项
		m_missionCommand_B3 = 0;  // 互斥：取消其他项
	}
	SendControlCommand();
}

// ============================================================
// 菜单更新函数（用于显示选中状态）
// ============================================================
void CFWGCSDlgDlg::OnUpdateMenuCtrlModeManual(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_controlMode_B0 == 0);
}

void CFWGCSDlgDlg::OnUpdateMenuCtrlModeSemi(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_controlMode_B0 == 1);
}

void CFWGCSDlgDlg::OnUpdateMenuCtrlModeFull(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_controlMode_B0 == 2);
}

void CFWGCSDlgDlg::OnUpdateMenuMissionTest(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_missionCommand_B0 == 0);
}

void CFWGCSDlgDlg::OnUpdateMenuMissionLaunchProc(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_missionCommand_B0 == 1);
}

void CFWGCSDlgDlg::OnUpdateMenuMissionBindParam(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_missionCommand_B2 == 1);
}

void CFWGCSDlgDlg::OnUpdateMenuMissionLaunchCmd(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_missionCommand_B5 == 1);
}

void CFWGCSDlgDlg::OnUpdateMenuCheckSelf(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_missionCommand_B1 == 1);
}

void CFWGCSDlgDlg::OnUpdateMenuCheckSurface(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_missionCommand_B3 == 1);
}

void CFWGCSDlgDlg::OnUpdateMenuCheckEngine(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_missionCommand_B4 == 1);
}

// ============================================================
// 发送控制指令函数
// ============================================================
BOOL CFWGCSDlgDlg::SendControlCommand()
{
	if (!m_bUdpConnected)
	{
		TRACE(_T("SendControlCommand: UDP未连接，无法发送控制指令\n"));
		return FALSE;
	}

	// 初始化控制指令数据包
	UdpSendDataPacket_Cmd packet{};
	memset(&packet, 0, sizeof(packet));
	packet.frameHeader = 0xBB11;  // 设置帧头（控制指令包）

	// 填充控制指令数据
	packet.missionCommand_B0 = m_missionCommand_B0;
	packet.missionCommand_B1 = m_missionCommand_B1;
	packet.missionCommand_B2 = m_missionCommand_B2;
	packet.missionCommand_B3 = m_missionCommand_B3;
	packet.missionCommand_B4 = m_missionCommand_B4;
	packet.missionCommand_B5 = m_missionCommand_B5;
	packet.controlMode_B0 = m_controlMode_B0;

	// 计算整个结构体的校验和
	packet.checksum = 0;
	size_t checksumSize = sizeof(packet) - sizeof(packet.checksum);
	packet.checksum = calculateChecksum(&packet, checksumSize);

	// 调试输出
	TRACE(_T("开始发送控制指令: frameHeader=0x%04X, missionCommand_B0~B5=%u,%u,%u,%u,%u,%u, controlMode_B0=%u, checksum=0x%02X\n"),
		packet.frameHeader,
		packet.missionCommand_B0, packet.missionCommand_B1, packet.missionCommand_B2,
		packet.missionCommand_B3, packet.missionCommand_B4, packet.missionCommand_B5,
		packet.controlMode_B0, packet.checksum);

	// 发送数据（连续发送2-3次以应对丢包）
	const int nSendCount = 1;  //当前不启用多次发送
	const int nSendIntervalMs = 10;
	int nSuccessCount = 0;

	for (int i = 0; i < nSendCount; i++)
	{
		BOOL bResult = SendUdpData(&packet, sizeof(packet));
		if (bResult)
		{
			nSuccessCount++;
		}
		if (i < nSendCount - 1)
		{
			Sleep(nSendIntervalMs);
		}
	}

	TRACE(_T("控制指令发送完成：成功 %d/%d 次\n"), nSuccessCount, nSendCount);
	return (nSuccessCount > 0);
}

// 从注册表加载UDP配置（未实现，当前使用宏默认值）
void CFWGCSDlgDlg::LoadUdpConfig()
{
	// 忽略注册表，每次启动都使用宏默认值
	m_strUdpLocalIP = CString(UDP_LOCAL_IP);
	m_nUdpLocalPort = UDP_LOCAL_PORT;
	m_strUdpRemoteIP = CString(UDP_REMOTE_IP);
	m_nUdpRemotePort = UDP_REMOTE_PORT;
	TRACE(_T("LoadUdpConfig: 忽略注册表，使用宏默认值\n"));
}

// 保存UDP配置到注册表
void CFWGCSDlgDlg::SaveUdpConfig()
{
	// 忽略注册表，不保存配置
	TRACE(_T("SaveUdpConfig: 忽略注册表，不保存配置\n"));
}
