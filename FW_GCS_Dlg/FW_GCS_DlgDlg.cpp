
// FW_GCS_DlgDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include <winsock2.h>
#include <ws2tcpip.h>  // 用于 inet_pton()
#include "FW_GCS_Dlg.h"
#include "FW_GCS_DlgDlg.h"
#include "afxdialogex.h"
#include "UdpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// UDP配置参数宏
#define UDP_REMOTE_IP      "127.0.0.1"  // 远程IP地址
#define UDP_REMOTE_PORT    14551             // 远程端口
#define UDP_LOCAL_PORT     14550             // 本地端口

// 串口配置参数宏
#define SERIAL_PORT_NAME   "COM21"        // 目标串口名称（RS422串口，格式：COM1-COM256）
#define SERIAL_BAUD_RATE   115200        // 波特率（常用值：9600, 19200, 38400, 57600, 115200）

// 自定义消息：UDP数据接收
#define WM_UDP_DATA_RECEIVED  (WM_USER + 200)
// 自定义消息：串口数据接收（由串口接收线程发送到主线程）
#define WM_SERIAL_DATA_RECEIVED  (WM_USER + 201)


// CFWGCSDlgDlg 对话框

CFWGCSDlgDlg::CFWGCSDlgDlg(CWnd* pParent /*=nullptr*/):CDialogEx(IDD_FW_GCS_DLG_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
	// UDP初始化
	m_udpSocket = INVALID_SOCKET;
	m_bUdpConnected = FALSE;
	m_pUdpRecvThread = NULL;
	m_bUdpThreadRunning = FALSE;
	memset(&m_udpRemoteAddr, 0, sizeof(m_udpRemoteAddr));
	
	// 串口初始化
	m_hSerialPort = INVALID_HANDLE_VALUE;      // 串口句柄初始化为无效值
	m_bSerialConnected = FALSE;                 // 串口连接状态标志：未连接
	m_pSerialRecvThread = NULL;                 // 串口接收线程指针：未创建
	m_bSerialThreadRunning = FALSE;             // 串口线程运行标志：未运行
	m_nSerialBufferSize = 0;                    // 串口接收缓冲区大小：空
	memset(m_serialBuffer, 0, sizeof(m_serialBuffer));  // 清空接收缓冲区
}

CFWGCSDlgDlg::~CFWGCSDlgDlg()
{
	DisconnectUdp();	// 断开UDP连接
	CloseSerialPort();	// 关闭串口
}

void CFWGCSDlgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_Display, m_editData1);    // 绑定data1显示控件
	DDX_Control(pDX, IDC_Display1, m_editData2);  // 绑定data2显示控件
	DDX_Control(pDX, IDC_Display2, m_editData3);  // 绑定data3显示控件
	DDX_Control(pDX, IDC_Display3, m_editData4);  // 绑定data4显示控件
	DDX_Control(pDX, IDC_Display4, m_editData5);  // 绑定data5显示控件
}

BEGIN_MESSAGE_MAP(CFWGCSDlgDlg, CDialogEx) // 消息映射
	ON_WM_PAINT() // 绘制消息处理
	ON_WM_QUERYDRAGICON() // 查询拖动图标消息处理
	ON_BN_CLICKED(IDC_UDPlink, &CFWGCSDlgDlg::OnBnClickedUdplink) // UDP连接按钮事件处理
	ON_WM_DESTROY() // 销毁消息处理
	ON_MESSAGE(WM_UDP_DATA_RECEIVED, &CFWGCSDlgDlg::OnUdpDataReceivedMsg) // UDP数据接收消息处理
	ON_MESSAGE(WM_SERIAL_DATA_RECEIVED, &CFWGCSDlgDlg::OnSerialDataReceivedMsg) // 串口数据接收消息处理
	ON_BN_CLICKED(IDC_SerialLink, &CFWGCSDlgDlg::OnBnClickedSeriallink) // 串口连接按钮事件处理
END_MESSAGE_MAP()


// CFWGCSDlgDlg 消息处理程序

BOOL CFWGCSDlgDlg::OnInitDialog() // 对话框初始化
{
	CDialogEx::OnInitDialog(); // 调用基类对话框初始化

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	// 执行此操作
	SetIcon(m_hIcon, TRUE);		// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// 初始化Winsock库
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // 初始化Winsock库
	{
		MessageBox(_T("Winsock初始化失败！"), _T("错误"), MB_OK | MB_ICONERROR); // 显示错误消息
		return FALSE;
	}

	// 初始化UDP Socket（但不连接）
	InitUdpSocket(); // 初始化UDP Socket

	// 初始化显示控件
	m_editData1.SetWindowText(_T("0.00"));  // 初始化data1显示
	m_editData2.SetWindowText(_T("0.00"));  // 初始化data2显示
	m_editData3.SetWindowText(_T("0.00"));  // 初始化data3显示
	m_editData4.SetWindowText(_T("0.00"));  // 初始化data4显示
	m_editData5.SetWindowText(_T("0.00"));  // 初始化data5显示
	
	TRACE(_T("OnInitDialog: 所有数据显示控件已初始化\n"));

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CFWGCSDlgDlg::OnPaint() // 绘制消息处理
{
	if (IsIconic()) // 如果窗口最小化
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0); // 发送图标擦除背景消息

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON); // 获取图标宽度
		int cyIcon = GetSystemMetrics(SM_CYICON); // 获取图标高度
		CRect rect; // 矩形区域
		GetClientRect(&rect); // 获取客户区矩形
		int x = (rect.Width() - cxIcon + 1) / 2; // 计算图标左上角坐标
		int y = (rect.Height() - cyIcon + 1) / 2; // 计算图标左上角坐标

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon); // 绘制图标
	}
	else
	{
		CDialogEx::OnPaint(); // 调用基类绘制
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CFWGCSDlgDlg::OnQueryDragIcon() // 查询拖动图标消息处理
{
	return static_cast<HCURSOR>(m_hIcon); // 返回图标句柄
}

void CFWGCSDlgDlg::OnBnClickedUdplink() // UDP连接按钮事件处理
{
	if (!m_bUdpConnected) // 如果未连接
	{
		// 当前未连接，执行连接操作
		if (ConnectUdp()) // 连接UDP
		{
			CString strMsg; // 消息字符串
			strMsg.Format(_T("UDP连接成功！\n\n本地端口: %d\n远程地址: %s:%d"),  
				UDP_LOCAL_PORT, _T(UDP_REMOTE_IP), UDP_REMOTE_PORT); // 格式化消息字符串
			MessageBox(strMsg, _T("UDP回报窗口"), MB_OK | MB_ICONINFORMATION); // 显示消息
		}
		else
		{
			int nError = WSAGetLastError(); // 获取错误代码
			CString strError; // 错误字符串
			strError.Format(_T("UDP连接失败！\n\n错误代码: %d\n\n请检查：\n1. 端口%d是否被占用\n2. Winsock是否正常初始化\n3. 查看调试输出获取详细信息"), 
				nError, UDP_LOCAL_PORT); // 格式化错误字符串
			MessageBox(strError, _T("错误"), MB_OK | MB_ICONERROR); // 显示错误消息
		}
	}
	else
	{
		// 当前已连接，执行断开操作
		DisconnectUdp(); // 断开UDP连接
		MessageBox(_T("UDP已断开！"), _T("UDP回报窗口"), MB_OK | MB_ICONINFORMATION); // 显示消息
	}
}

void CFWGCSDlgDlg::OnDestroy() // 销毁消息处理
{
	// 断开UDP连接
	DisconnectUdp();
	// 关闭串口
	CloseSerialPort();
	
	// 清理Winsock库
	WSACleanup();
	
	CDialogEx::OnDestroy();
}

// 初始化UDP Socket
BOOL CFWGCSDlgDlg::InitUdpSocket()
{
	if (m_udpSocket != INVALID_SOCKET) // 如果UDP Socket有效
	{
		closesocket(m_udpSocket); // 关闭UDP Socket
		m_udpSocket = INVALID_SOCKET; // 将UDP Socket设置为无效
	}

	// 创建UDP Socket
	m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // 创建UDP Socket
	if (m_udpSocket == INVALID_SOCKET) // 如果UDP Socket创建失败
	{
		int nError = WSAGetLastError(); // 获取错误代码
		TRACE(_T("UDP Socket创建失败，错误代码: %d\n"), nError); // 输出错误信息
		return FALSE; // 返回失败
	}

	// 绑定本地端口（用于接收数据）
	sockaddr_in localAddr; // 本地地址结构
	memset(&localAddr, 0, sizeof(localAddr)); // 清空本地地址结构
	localAddr.sin_family = AF_INET; // 设置地址族为IPv4
	localAddr.sin_addr.s_addr = INADDR_ANY; // 设置地址为任意IP地址
	localAddr.sin_port = htons(UDP_LOCAL_PORT); // 设置端口
	
	if (bind(m_udpSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) // 绑定本地端口
	{
		int nError = WSAGetLastError(); // 获取错误代码
		TRACE(_T("UDP端口绑定失败 (端口%d)，错误代码: %d\n"), UDP_LOCAL_PORT, nError); // 输出错误信息
		// 如果端口被占用，尝试不绑定（UDP可以发送但不一定能接收）
		// 这里不返回错误，继续执行，但接收可能失败
	}
	else
	{
		TRACE(_T("UDP Socket初始化成功，本地端口: %d\n"), UDP_LOCAL_PORT); // 输出成功信息
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
	if (m_udpSocket == INVALID_SOCKET) // 如果UDP Socket无效
	{
		if (!InitUdpSocket()) // 初始化UDP Socket
		{
			TRACE(_T("UDP连接失败: Socket初始化失败\n"));
			return FALSE;
		}
	}

	// 设置远程地址
	memset(&m_udpRemoteAddr, 0, sizeof(m_udpRemoteAddr)); // 清空远程地址结构
	m_udpRemoteAddr.sin_family = AF_INET; // 设置地址族为IPv4
	m_udpRemoteAddr.sin_port = htons(UDP_REMOTE_PORT); // 设置端口
	
	// 使用 inet_pton() 替代已弃用的 inet_addr()
	if (inet_pton(AF_INET, UDP_REMOTE_IP, &m_udpRemoteAddr.sin_addr) != 1) // 转换IP地址
	{
		// IP地址转换失败
		TRACE(_T("UDP连接失败: IP地址转换失败 (%s)\n"), UDP_REMOTE_IP); // 输出错误信息
		return FALSE;
	}

	// 启动接收线程（先启动线程，再发送握手）
	m_bUdpThreadRunning = TRUE; // 设置线程运行标志
	m_pUdpRecvThread = AfxBeginThread(UdpRecvThread, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (m_pUdpRecvThread == NULL) // 如果接收线程创建失败
	{
		m_bUdpThreadRunning = FALSE; // 设置线程运行标志
		TRACE(_T("UDP连接失败: 接收线程创建失败\n")); // 输出错误信息
		return FALSE;
	}
	m_pUdpRecvThread->ResumeThread(); // 启动接收线程

	// 设置连接标志（在发送握手之前设置，因为SendUdpData需要）
	m_bUdpConnected = TRUE;

	// 发送握手数据包（可选，UDP是无连接的，发送失败不影响接收）
	SendHandshake();  // 不检查返回值，因为UDP发送可能失败但不影响接收

	return TRUE;
}

// 断开UDP连接
void CFWGCSDlgDlg::DisconnectUdp() // 断开UDP连接
{
	if (!m_bUdpConnected) // 如果未连接
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
	UdpHandshakePacket handshake;
	memcpy(handshake.magic, "GCS", 3);
	handshake.magic[3] = '\0';
	handshake.version = 1;

	return SendUdpData(&handshake, sizeof(handshake));
}

// 发送UDP数据
BOOL CFWGCSDlgDlg::SendUdpData(const void* pData, int nSize)
{
	if (m_udpSocket == INVALID_SOCKET)
	{
		return FALSE;
	}

	int nSent = sendto(m_udpSocket, (const char*)pData, nSize, 0, 
		(sockaddr*)&m_udpRemoteAddr, sizeof(m_udpRemoteAddr));

	if (nSent == SOCKET_ERROR)
	{
		return FALSE;
	}

	return (nSent == nSize);
}

// UDP接收线程函数
UINT CFWGCSDlgDlg::UdpRecvThread(LPVOID pParam)
{
	CFWGCSDlgDlg* pDlg = (CFWGCSDlgDlg*)pParam;
	char buffer[1024];
	sockaddr_in fromAddr;
	int nFromLen = sizeof(fromAddr);

	while (pDlg->m_bUdpThreadRunning)
	{
		// 阻塞等待接收数据
		int nReceived = recvfrom(pDlg->m_udpSocket, buffer, sizeof(buffer), 0,
			(sockaddr*)&fromAddr, &nFromLen);

		if (nReceived > 0)
		{
			TRACE(_T("UDP接收线程: 收到 %d 字节数据\n"), nReceived);
			
			// 检查数据包大小是否匹配
			if (nReceived == sizeof(UdpRecvDataPacket))
			{
				// 分配内存保存数据包（通过消息传递）
				UdpRecvDataPacket* pPacket = new UdpRecvDataPacket;
				memcpy(pPacket, buffer, sizeof(UdpRecvDataPacket));
				
				// 调试输出：显示接收到的数据
				TRACE(_T("UDP接收: data1=%.2f, data2=%.2f, data3=%.2f, data4=%.2f, data5=%.2f\n"),
					pPacket->data1, pPacket->data2, pPacket->data3, pPacket->data4, pPacket->data5);
				
				// 发送消息到主线程处理
				pDlg->PostMessage(WM_UDP_DATA_RECEIVED, (WPARAM)pPacket, 0);
			}
			else
			{
				TRACE(_T("UDP接收: 数据包大小不匹配！期望 %d 字节，实际收到 %d 字节\n"), 
					sizeof(UdpRecvDataPacket), nReceived);
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
		// 处理接收到的数据包
		ProcessReceivedData(pPacket);
		
		// 释放内存
		delete pPacket;
	}
	return 0;
}

// 处理接收到的数据包
void CFWGCSDlgDlg::ProcessReceivedData(const UdpRecvDataPacket* pPacket)
{
	if (pPacket == NULL)
	{
		TRACE(_T("ProcessReceivedData: 数据包指针为空！\n"));
		return;
	}

	TRACE(_T("ProcessReceivedData: 开始处理数据包\n"));
	TRACE(_T("  data1=%.6f, data2=%.6f, data3=%.6f, data4=%.6f, data5=%.6f\n"),
		pPacket->data1, pPacket->data2, pPacket->data3, pPacket->data4, pPacket->data5);
	
	// 格式化并更新所有数据显示控件
	CString strData1, strData2, strData3, strData4, strData5;
	strData1.Format(_T("%.2f"), pPacket->data1);
	strData2.Format(_T("%.2f"), pPacket->data2);
	strData3.Format(_T("%.2f"), pPacket->data3);
	strData4.Format(_T("%.2f"), pPacket->data4);
	strData5.Format(_T("%.2f"), pPacket->data5);
	
	// 更新data1显示控件（IDC_Display）
	if (m_editData1.GetSafeHwnd() != NULL)
	{
		m_editData1.SetWindowText(strData1);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display);
		if (pWnd != NULL) pWnd->SetWindowText(strData1);
	}
	
	// 更新data2显示控件（IDC_Display1）
	if (m_editData2.GetSafeHwnd() != NULL)
	{
		m_editData2.SetWindowText(strData2);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display1);
		if (pWnd != NULL) pWnd->SetWindowText(strData2);
	}
	
	// 更新data3显示控件（IDC_Display2）
	if (m_editData3.GetSafeHwnd() != NULL)
	{
		m_editData3.SetWindowText(strData3);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display2);
		if (pWnd != NULL) pWnd->SetWindowText(strData3);
	}
	
	// 更新data4显示控件（IDC_Display3）
	if (m_editData4.GetSafeHwnd() != NULL)
	{
		m_editData4.SetWindowText(strData4);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display3);
		if (pWnd != NULL) pWnd->SetWindowText(strData4);
	}
	
	// 更新data5显示控件（IDC_Display4）
	if (m_editData5.GetSafeHwnd() != NULL)
	{
		m_editData5.SetWindowText(strData5);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display4);
		if (pWnd != NULL) pWnd->SetWindowText(strData5);
	}
	
	TRACE(_T("ProcessReceivedData: 已更新所有控件显示\n"));
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
//   1. 循环读取串口数据（ReadFile阻塞等待）
//   2. 将新数据追加到接收缓冲区（处理不完整数据包）
//   3. 从缓冲区中提取完整的数据包
//   4. 只处理最后一个完整数据包（避免UI频繁刷新）
//   5. 通过PostMessage发送到主线程处理
//   6. 保留不完整数据到下次接收时拼接
// 注意事项：
//   - 串口数据是流式的，可能一次接收多个数据包或部分数据包
//   - 需要缓冲区管理，确保数据包边界正确
//   - 只处理最后一个数据包，避免消息队列积压
// ============================================================================
UINT CFWGCSDlgDlg::SerialRecvThread(LPVOID pParam)
{
	CFWGCSDlgDlg* pDlg = (CFWGCSDlgDlg*)pParam;  // 获取对话框指针
	BYTE buffer[1024];                            // 临时接收缓冲区（每次ReadFile的最大读取量）
	DWORD dwBytesRead;                            // 实际读取的字节数
	int nPacketSize = sizeof(UdpRecvDataPacket);  // 数据包大小：20字节（5个float × 4字节）
	
	TRACE(_T("串口接收线程启动，数据包大小: %d 字节\n"), nPacketSize);

	// ============================================================
	// 主循环：持续接收数据直到线程停止标志为FALSE
	// ============================================================
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

					// 调试输出：显示接收到的最后一个数据包内容
					TRACE(_T("串口接收[最后/%d]: data1=%.2f, data2=%.2f, data3=%.2f, data4=%.2f, data5=%.2f\n"),
						nPacketCount,
						pPacket->data1, pPacket->data2, pPacket->data3, pPacket->data4, pPacket->data5);

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
// ============================================================================
void CFWGCSDlgDlg::ProcessSerialReceivedData(const UdpRecvDataPacket* pPacket)
{
	// ============================================================
	// 步骤1：验证数据包指针有效性
	// ============================================================
	if (pPacket == NULL)
	{
		TRACE(_T("ProcessSerialReceivedData: 数据包指针为空！\n"));
		return;
	}

	// ============================================================
	// 步骤2：格式化数据为字符串（保留2位小数）
	// ============================================================
	CString strData1, strData2, strData3, strData4, strData5;
	strData1.Format(_T("%.2f"), pPacket->data1);  // data1：保留2位小数
	strData2.Format(_T("%.2f"), pPacket->data2);  // data2：保留2位小数
	strData3.Format(_T("%.2f"), pPacket->data3);  // data3：保留2位小数
	strData4.Format(_T("%.2f"), pPacket->data4);  // data4：保留2位小数
	strData5.Format(_T("%.2f"), pPacket->data5);  // data5：保留2位小数

	// ============================================================
	// 步骤3：更新所有数据显示控件
	// ============================================================
	// 使用双重检查：先检查控件句柄是否有效，失败则使用GetDlgItem获取控件
	
	// 更新data1显示控件（IDC_Display）
	if (m_editData1.GetSafeHwnd() != NULL)
	{
		m_editData1.SetWindowText(strData1);
	}
	else
	{
		// 控件未绑定，使用GetDlgItem获取控件
		CWnd* pWnd = GetDlgItem(IDC_Display);
		if (pWnd != NULL) pWnd->SetWindowText(strData1);
	}

	// 更新data2显示控件（IDC_Display1）
	if (m_editData2.GetSafeHwnd() != NULL)
	{
		m_editData2.SetWindowText(strData2);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display1);
		if (pWnd != NULL) pWnd->SetWindowText(strData2);
	}

	// 更新data3显示控件（IDC_Display2）
	if (m_editData3.GetSafeHwnd() != NULL)
	{
		m_editData3.SetWindowText(strData3);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display2);
		if (pWnd != NULL) pWnd->SetWindowText(strData3);
	}

	// 更新data4显示控件（IDC_Display3）
	if (m_editData4.GetSafeHwnd() != NULL)
	{
		m_editData4.SetWindowText(strData4);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display3);
		if (pWnd != NULL) pWnd->SetWindowText(strData4);
	}

	// 更新data5显示控件（IDC_Display4）
	if (m_editData5.GetSafeHwnd() != NULL)
	{
		m_editData5.SetWindowText(strData5);
	}
	else
	{
		CWnd* pWnd = GetDlgItem(IDC_Display4);
		if (pWnd != NULL) pWnd->SetWindowText(strData5);
	}
		
	TRACE(_T("ProcessSerialReceivedData: 已更新所有控件显示\n"));
}
