// UdpSettingsDlg.cpp: UDP设置对话框实现文件
//

#include "pch.h"
#include "framework.h"
#include "FW_GCS_Dlg.h"
#include "UdpSettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CUdpSettingsDlg 对话框

IMPLEMENT_DYNAMIC(CUdpSettingsDlg, CDialogEx)

CUdpSettingsDlg::CUdpSettingsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_UDP_SETTINGS_DIALOG, pParent)
	, m_strLocalIP(CString(UDP_LOCAL_IP))
	, m_nLocalPort(50001)
	, m_strRemoteIP(_T("192.168.1.11"))
	, m_nRemotePort(50000)
{
}

CUdpSettingsDlg::~CUdpSettingsDlg()
{
}

void CUdpSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// 注意：不在此处做DDX绑定，避免控件ID不匹配导致断言
}

BEGIN_MESSAGE_MAP(CUdpSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CUdpSettingsDlg::OnBnClickedOk)
END_MESSAGE_MAP()

// CUdpSettingsDlg 消息处理程序

BOOL CUdpSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 手动绑定IP地址控件（避免DDX问题）
	CWnd* pWnd = GetDlgItem(IDC_IPADDRESS1);
	if (pWnd != NULL)
	{
		m_ipLocal.SubclassWindow(pWnd->GetSafeHwnd());
	}
	
	pWnd = GetDlgItem(IDC_IPADDRESS2);
	if (pWnd != NULL)
	{
		m_ipRemote.SubclassWindow(pWnd->GetSafeHwnd());
	}

	// 设置端口值（控件存在时才写入）
	pWnd = GetDlgItem(IDC_Display73); // 本机端口（与资源一致）
	if (pWnd != NULL)
	{
		SetDlgItemInt(IDC_Display73, static_cast<UINT>(m_nLocalPort), FALSE);
	}
	pWnd = GetDlgItem(IDC_Display76); // 远程端口（与资源一致）
	if (pWnd != NULL)
	{
		SetDlgItemInt(IDC_Display76, static_cast<UINT>(m_nRemotePort), FALSE);
	}

	// 然后设置IP地址控件值
	// 解析IP地址字符串为4个字节
	// 本机IP
	BYTE bLocalIP[4] = {0, 0, 0, 0};
	if (!m_strLocalIP.IsEmpty())
	{
		_stscanf_s(m_strLocalIP, _T("%hhu.%hhu.%hhu.%hhu"), 
			&bLocalIP[0], &bLocalIP[1], &bLocalIP[2], &bLocalIP[3]);
	}
	if (m_ipLocal.GetSafeHwnd() != NULL)
	{
		m_ipLocal.SetAddress(bLocalIP[0], bLocalIP[1], bLocalIP[2], bLocalIP[3]);
	}
	
	// 远程IP
	BYTE bRemoteIP[4] = {0, 0, 0, 0};
	if (!m_strRemoteIP.IsEmpty())
	{
		_stscanf_s(m_strRemoteIP, _T("%hhu.%hhu.%hhu.%hhu"), 
			&bRemoteIP[0], &bRemoteIP[1], &bRemoteIP[2], &bRemoteIP[3]);
	}
	if (m_ipRemote.GetSafeHwnd() != NULL)
	{
		m_ipRemote.SetAddress(bRemoteIP[0], bRemoteIP[1], bRemoteIP[2], bRemoteIP[3]);
	}

	return TRUE;
}

void CUdpSettingsDlg::OnBnClickedOk()
{
	// 从IP地址控件获取IP地址
	BYTE bLocalIP[4] = {0, 0, 0, 0};
	BYTE bRemoteIP[4] = {0, 0, 0, 0};
	
	// 获取本机IP（使用GetAddress的重载版本，直接获取4个字节）
	if (m_ipLocal.GetSafeHwnd() == NULL || 
		m_ipLocal.GetAddress(bLocalIP[0], bLocalIP[1], bLocalIP[2], bLocalIP[3]) == 0)
	{
		MessageBox(_T("本机IP地址格式不正确！"), _T("输入错误"), MB_OK | MB_ICONERROR);
		if (m_ipLocal.GetSafeHwnd() != NULL)
			m_ipLocal.SetFocus();
		return;
	}
	m_strLocalIP.Format(_T("%d.%d.%d.%d"), bLocalIP[0], bLocalIP[1], bLocalIP[2], bLocalIP[3]);
	
	// 获取远程IP
	if (m_ipRemote.GetSafeHwnd() == NULL || 
		m_ipRemote.GetAddress(bRemoteIP[0], bRemoteIP[1], bRemoteIP[2], bRemoteIP[3]) == 0)
	{
		MessageBox(_T("远程IP地址格式不正确！"), _T("输入错误"), MB_OK | MB_ICONERROR);
		if (m_ipRemote.GetSafeHwnd() != NULL)
			m_ipRemote.SetFocus();
		return;
	}
	m_strRemoteIP.Format(_T("%d.%d.%d.%d"), bRemoteIP[0], bRemoteIP[1], bRemoteIP[2], bRemoteIP[3]);
	
	// 读取端口值（控件存在时才读取）
	BOOL bTranslated = FALSE;
	if (GetDlgItem(IDC_Display73) != NULL)
	{
		UINT localPort = GetDlgItemInt(IDC_Display73, &bTranslated, FALSE);
		if (bTranslated)
		{
			m_nLocalPort = static_cast<int>(localPort);
		}
	}
	if (GetDlgItem(IDC_Display76) != NULL)
	{
		UINT remotePort = GetDlgItemInt(IDC_Display76, &bTranslated, FALSE);
		if (bTranslated)
		{
			m_nRemotePort = static_cast<int>(remotePort);
		}
	}

	// 验证端口范围
	if (m_nLocalPort < 1 || m_nLocalPort > 65535)
	{
		MessageBox(_T("本机端口必须在1-65535之间！"), _T("输入错误"), MB_OK | MB_ICONERROR);
		if (GetDlgItem(IDC_Display73) != NULL)
		{
			GetDlgItem(IDC_Display73)->SetFocus();
		}
		return;
	}
	
	if (m_nRemotePort < 1 || m_nRemotePort > 65535)
	{
		MessageBox(_T("远程端口必须在1-65535之间！"), _T("输入错误"), MB_OK | MB_ICONERROR);
		if (GetDlgItem(IDC_Display76) != NULL)
		{
			GetDlgItem(IDC_Display76)->SetFocus();
		}
		return;
	}

	CDialogEx::OnOK();
}
