// Page2Dlg.h: 第二页子对话框头文件
//

#pragma once

#include <afxdialogex.h>
#include "UdpData.h"

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
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	// 数据显示控件
	CEdit m_editData4;   // Display3 (AoA)
	CEdit m_editData5;   // Display4 (AoS)
	
	// 更新数据显示
	void UpdateDisplay(const UdpRecvDataPacket* pPacket);
	afx_msg void OnBnClickedButton1();
};

