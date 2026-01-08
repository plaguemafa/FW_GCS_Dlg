// Page1Dlg.h: 第一页子对话框头文件
//

#pragma once

#include <afxdialogex.h>
#include "UdpData.h"

// CPage1Dlg 对话框
class CPage1Dlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPage1Dlg)

public:
	CPage1Dlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CPage1Dlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PAGE1_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	// 数据显示控件
	CEdit m_editData1;   // Display0 (Pitch)
	CEdit m_editData2;   // Display1 (Roll)
	CEdit m_editData3;   // Display2 (Yaw)
	
	// 更新数据显示
	void UpdateDisplay(const UdpRecvDataPacket* pPacket);
};

