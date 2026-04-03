#pragma once

#include <afxdialogex.h>

class CFWGCSDlgDlg;

// 舵面检查（IDD_SURFACE_CHECK）：测试模式单选与主窗口 CmdSendPacket_s.surface_check_mode 联动
class CSurfaceCheckDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSurfaceCheckDlg)

public:
	explicit CSurfaceCheckDlg(CFWGCSDlgDlg* pMainDlg);

	void SyncRadiosFromMain();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SURFACE_CHECK };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();

	afx_msg void OnBnClickedCheckModeAuto();
	afx_msg void OnBnClickedCheckModeManual();
	afx_msg void OnBnClickedAutoSurfaceCheck();   // 自动偏舵
	afx_msg void OnBnClickedElevatorCheck();      // 手动-升降舵偏舵
	afx_msg void OnBnClickedAileronCheck();       // 手动-倾斜舵偏舵
	afx_msg void OnBnClickedSurfaceReset();       // 舵面复位

	DECLARE_MESSAGE_MAP()

private:
	CFWGCSDlgDlg* m_pMainDlg;
	CEdit m_editElevator;         // IDC_Display1：升降舵指令值显示
	CEdit m_editAileron;          // IDC_Display2：倾斜舵指令值显示

	int m_elevatorIndex = 0;      // 升降舵当前序列索引
	int m_aileronIndex = 0;       // 倾斜舵当前序列索引

	void StepCommand(bool isElevator);
	void UpdateDisplay(CEdit& editCtrl, int value);
};