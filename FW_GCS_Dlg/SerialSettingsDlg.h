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
	afx_msg void OnBnClickedOk();

	// 获取设置值
	CString GetPortName() const { return m_strPortName; }
	int     GetBaudRate() const { return m_nBaudRate; }

	// 设置初始值
	void SetPortName(const CString& port) { m_strPortName = port; }
	void SetBaudRate(int baud) { m_nBaudRate = baud; }

private:
	CString m_strPortName;   // 本机串口名称（例如 "COM21"）
	int     m_nBaudRate;     // 串口波特率（例如 115200）
};
