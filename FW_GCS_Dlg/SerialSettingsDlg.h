// SerialSettingsDlg.h: 串口设置对话框头文件（占位）
//

#pragma once

#include <afxdialogex.h>

// CSerialSettingsDlg 对话框
class CSerialSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSerialSettingsDlg)

public:
	CSerialSettingsDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSerialSettingsDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERIAL_SETTINGS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
};
