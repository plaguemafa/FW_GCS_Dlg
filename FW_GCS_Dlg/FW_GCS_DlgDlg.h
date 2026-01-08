
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
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedSeriallink();
	afx_msg void OnBnClickedButton1();
};
