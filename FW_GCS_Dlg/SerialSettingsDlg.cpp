// SerialSettingsDlg.cpp: 串口设置对话框实现文件（占位）
//

#include "pch.h"
#include "framework.h"
#include "FW_GCS_Dlg.h"
#include "SerialSettingsDlg.h"
#include "SerialConfig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CSerialSettingsDlg 对话框

IMPLEMENT_DYNAMIC(CSerialSettingsDlg, CDialogEx)

CSerialSettingsDlg::CSerialSettingsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SERIAL_SETTINGS_DIALOG, pParent)
	, m_strPortName(_T(SERIAL_PORT_NAME))
	, m_nBaudRate(SERIAL_BAUD_RATE)
{
}

CSerialSettingsDlg::~CSerialSettingsDlg()
{
}

void CSerialSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSerialSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSerialSettingsDlg::OnBnClickedOk)
END_MESSAGE_MAP()

// CSerialSettingsDlg 消息处理程序

BOOL CSerialSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 初始化本机串口编辑框
	if (GetDlgItem(IDC_Display_localCOM) != NULL)
	{
		SetDlgItemText(IDC_Display_localCOM, m_strPortName);
	}

	// 初始化波特率编辑框（如果资源中已添加该控件）
	if (GetDlgItem(IDC_Display_BAUD_RATE) != NULL)
	{
		SetDlgItemInt(IDC_Display_BAUD_RATE, static_cast<UINT>(m_nBaudRate), FALSE);
	}

	return TRUE;
}

void CSerialSettingsDlg::OnBnClickedOk()
{
	// 读取本机串口名
	CString strPort;
	if (GetDlgItem(IDC_Display_localCOM) != NULL)
	{
		GetDlgItemText(IDC_Display_localCOM, strPort);
		strPort.Trim();
	}

	if (strPort.IsEmpty())
	{
		MessageBox(_T("本机端口不能为空！"), _T("输入错误"), MB_OK | MB_ICONERROR);
		if (GetDlgItem(IDC_Display_localCOM) != NULL)
			GetDlgItem(IDC_Display_localCOM)->SetFocus();
		return;
	}

	m_strPortName = strPort;

	// 读取波特率（如果控件存在）
	if (GetDlgItem(IDC_Display_BAUD_RATE) != NULL)
	{
		BOOL bTranslated = FALSE;
		UINT nBaud = GetDlgItemInt(IDC_Display_BAUD_RATE, &bTranslated, FALSE);
		if (!bTranslated || nBaud == 0)
		{
			MessageBox(_T("波特率必须是大于0的整数！"), _T("输入错误"), MB_OK | MB_ICONERROR);
			GetDlgItem(IDC_Display_BAUD_RATE)->SetFocus();
			return;
		}

		m_nBaudRate = static_cast<int>(nBaud);
	}

	CDialogEx::OnOK();
}
