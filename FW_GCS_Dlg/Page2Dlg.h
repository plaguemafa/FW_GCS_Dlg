// Page2Dlg.h: 第二页子对话框头文件
//

#pragma once

#include <afxdialogex.h>
#include "UdpData.h"

// 前向声明
class CFWGCSDlgDlg;

// CPage2Dlg 对话框
class CPage2Dlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPage2Dlg)

public:
	CPage2Dlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CPage2Dlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PAGE2_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog() override;
	// 发送数据输入控件
	CEdit m_editSendData0; // IDC_Display_EditData0
	CEdit m_editSendData1; // IDC_Display_EditData1
	CEdit m_editSendData2; // IDC_Display_EditData2
	CEdit m_editSendData3; // IDC_Display_EditData3
	CEdit m_editSendData4; // IDC_Display_EditData4
	
	// 设置主对话框指针（避免每次使用dynamic_cast）
	void SetMainDlg(CFWGCSDlgDlg* pMainDlg) { m_pMainDlg = pMainDlg; }
	
	// 更新数据显示（Page2当前不显示接收数据，保留接口以兼容主对话框调用）
	void UpdateDisplay(const UdpRecvDataPacket* pPacket);
	afx_msg void OnBnClickedButtonSendData();
	
private:
	CFWGCSDlgDlg* m_pMainDlg;  // 主对话框指针
};

