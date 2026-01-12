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
	
	// 视窗组9相关控件（自检结果）
	CEdit m_editDisplay66;   // IMU自检结果（IDC_Display66）
	CEdit m_editDisplay67;   // 气压计自检结果（IDC_Display67）
	CEdit m_editDisplay68;   // 空速计自检结果（IDC_Display68）
	CEdit m_editDisplay69;   // 温度计自检结果（IDC_Display69）
	CEdit m_editDisplay70;   // GPS自检结果（IDC_Display70）
	CEdit m_editDisplay71;   // 电压自检结果（IDC_Display71）
	CEdit m_editDisplay72;   // PWM自检结果（IDC_Display72）
	CEdit m_editDisplay73;   // SBUS自检结果（IDC_Display73）
	
	// 更新数据显示
	void UpdateDisplay(const UdpRecvDataPacket* pPacket);
};

