
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

// UDP配置参数（写死在代码中）
#define UDP_REMOTE_IP      "127.0.0.1"  // 远程IP地址
#define UDP_REMOTE_PORT    14551             // 远程端口
#define UDP_LOCAL_PORT     14550             // 本地端口

// 自定义消息：UDP数据接收
#define WM_UDP_DATA_RECEIVED  (WM_USER + 200)


// CFWGCSDlgDlg 对话框



CFWGCSDlgDlg::CFWGCSDlgDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FW_GCS_DLG_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
	// UDP初始化
	m_udpSocket = INVALID_SOCKET;
	m_bUdpConnected = FALSE;
	m_pUdpRecvThread = NULL;
	m_bUdpThreadRunning = FALSE;
	memset(&m_udpRemoteAddr, 0, sizeof(m_udpRemoteAddr));
}

CFWGCSDlgDlg::~CFWGCSDlgDlg()
{
	// 确保断开UDP连接
	DisconnectUdp();
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

BEGIN_MESSAGE_MAP(CFWGCSDlgDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_UDPlink, &CFWGCSDlgDlg::OnBnClickedUdplink)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_UDP_DATA_RECEIVED, &CFWGCSDlgDlg::OnUdpDataReceivedMsg)
	ON_BN_CLICKED(IDC_SerialLink, &CFWGCSDlgDlg::OnBnClickedSeriallink)
END_MESSAGE_MAP()


// CFWGCSDlgDlg 消息处理程序

BOOL CFWGCSDlgDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// 初始化Winsock库
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		MessageBox(_T("Winsock初始化失败！"), _T("错误"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	// 初始化UDP Socket（但不连接）
	InitUdpSocket();

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
				UDP_LOCAL_PORT, _T(UDP_REMOTE_IP), UDP_REMOTE_PORT);
			MessageBox(strMsg, _T("UDP回报窗口"), MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			int nError = WSAGetLastError();
			CString strError;
			strError.Format(_T("UDP连接失败！\n\n错误代码: %d\n\n请检查：\n1. 端口%d是否被占用\n2. Winsock是否正常初始化\n3. 查看调试输出获取详细信息"), 
				nError, UDP_LOCAL_PORT);
			MessageBox(strError, _T("错误"), MB_OK | MB_ICONERROR);
		}
	}
	else
	{
		// 当前已连接，执行断开操作
		DisconnectUdp();
		MessageBox(_T("UDP已断开！"), _T("UDP回报窗口"), MB_OK | MB_ICONINFORMATION);
	}
}

void CFWGCSDlgDlg::OnDestroy()
{
	// 断开UDP连接
	DisconnectUdp();
	
	// 清理Winsock库
	WSACleanup();
	
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

	// 绑定本地端口（用于接收数据）
	sockaddr_in localAddr;
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = INADDR_ANY;
	localAddr.sin_port = htons(UDP_LOCAL_PORT);
	
	if (bind(m_udpSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
	{
		int nError = WSAGetLastError();
		TRACE(_T("UDP端口绑定失败 (端口%d)，错误代码: %d\n"), UDP_LOCAL_PORT, nError);
		// 如果端口被占用，尝试不绑定（UDP可以发送但不一定能接收）
		// 这里不返回错误，继续执行，但接收可能失败
	}
	else
	{
		TRACE(_T("UDP Socket初始化成功，本地端口: %d\n"), UDP_LOCAL_PORT);
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

	// 设置远程地址
	memset(&m_udpRemoteAddr, 0, sizeof(m_udpRemoteAddr));
	m_udpRemoteAddr.sin_family = AF_INET;
	m_udpRemoteAddr.sin_port = htons(UDP_REMOTE_PORT);
	
	// 使用 inet_pton() 替代已弃用的 inet_addr()
	if (inet_pton(AF_INET, UDP_REMOTE_IP, &m_udpRemoteAddr.sin_addr) != 1)
	{
		// IP地址转换失败
		TRACE(_T("UDP连接失败: IP地址转换失败 (%s)\n"), UDP_REMOTE_IP);
		return FALSE;
	}

	// 启动接收线程（先启动线程，再发送握手）
	m_bUdpThreadRunning = TRUE;
	m_pUdpRecvThread = AfxBeginThread(UdpRecvThread, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (m_pUdpRecvThread == NULL)
	{
		m_bUdpThreadRunning = FALSE;
		TRACE(_T("UDP连接失败: 接收线程创建失败\n"));
		return FALSE;
	}
	m_pUdpRecvThread->ResumeThread();

	// 设置连接标志（在发送握手之前设置，因为SendUdpData需要）
	m_bUdpConnected = TRUE;

	// 发送握手数据包（可选，UDP是无连接的，发送失败不影响接收）
	SendHandshake();  // 不检查返回值，因为UDP发送可能失败但不影响接收

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



void CFWGCSDlgDlg::OnBnClickedSeriallink() //422串口
{
	// TODO: 在此添加控件通知处理程序代码
}
