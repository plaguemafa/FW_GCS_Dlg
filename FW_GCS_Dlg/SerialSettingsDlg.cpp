// SerialSettingsDlg.cpp: 串口设置对话框实现文件（占位）
//

#include "pch.h"
#include "framework.h"
#include "FW_GCS_Dlg.h"
#include "SerialSettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CSerialSettingsDlg 对话框

IMPLEMENT_DYNAMIC(CSerialSettingsDlg, CDialogEx)

CSerialSettingsDlg::CSerialSettingsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SERIAL_SETTINGS_DIALOG, pParent)
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
END_MESSAGE_MAP()

// CSerialSettingsDlg 消息处理程序

BOOL CSerialSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 占位对话框，显示提示信息
	MessageBox(_T("串口通信设置功能尚未实现，敬请期待！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
	
	return TRUE;
}
