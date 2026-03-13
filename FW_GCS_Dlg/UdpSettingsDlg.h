// UdpSettingsDlg.h: UDP设置对话框头文件
//

#pragma once

#include <afxdialogex.h>
#include <afxcmn.h>  // 用于CIPAddressCtrl（IP地址控件是Common Controls的一部分）
#include "UdpConfig.h"

// CUdpSettingsDlg 对话框
class CUdpSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CUdpSettingsDlg)

public:
	CUdpSettingsDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CUdpSettingsDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_UDP_SETTINGS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();

	// 获取设置值

	CString GetLocalIP() const { return m_strLocalIP; }
	int GetLocalPort() const { return m_nLocalPort; }
	CString GetRemoteIP() const { return m_strRemoteIP; }
	int GetRemotePort() const { return m_nRemotePort; }

	// 设置初始值
	void SetLocalIP(const CString& ip) { m_strLocalIP = ip; }
	void SetLocalPort(int port) { m_nLocalPort = port; }
	void SetRemoteIP(const CString& ip) { m_strRemoteIP = ip; }
	void SetRemotePort(int port) { m_nRemotePort = port; }

private:
	CString m_strLocalIP;      // 本机IP
	int m_nLocalPort;          // 本机端口
	CString m_strRemoteIP;     // 远程IP
	int m_nRemotePort;         // 远程端口
	
	// 控件变量
	CIPAddressCtrl m_ipLocal;      // 本机IP地址控件 (IDC_IPADDRESS1)
	CIPAddressCtrl m_ipRemote;     // 远程IP地址控件 (IDC_IPADDRESS2)
	// 注意：端口编辑框不需要控件变量，直接使用DDX_Text绑定值
};
