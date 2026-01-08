// Page1Dlg.cpp: 第一页子对话框实现文件
//

#include "pch.h"
#include "framework.h"
#include "FW_GCS_Dlg.h"
#include "Page1Dlg.h"
#include "UdpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPage1Dlg 对话框

IMPLEMENT_DYNAMIC(CPage1Dlg, CDialogEx)

CPage1Dlg::CPage1Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PAGE1_DIALOG, pParent)
{
}

CPage1Dlg::~CPage1Dlg()
{
}

void CPage1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// 可选绑定：如果控件存在则绑定，不存在也不报错
	// 注意：DoDataExchange 在对话框创建时调用，此时控件可能还未创建
	// 所以这里先不绑定，在 OnInitDialog 中再尝试绑定
	// 如果控件不存在，UpdateDisplay 函数中已经做了检查，不会出错
}

BEGIN_MESSAGE_MAP(CPage1Dlg, CDialogEx)
END_MESSAGE_MAP()

// CPage1Dlg 消息处理程序

BOOL CPage1Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	// 尝试绑定控件（如果控件存在）
	CWnd* pWnd = GetDlgItem(IDC_Display0);
	if (pWnd != NULL)
		m_editData1.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display1);
	if (pWnd != NULL)
		m_editData2.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display2);
	if (pWnd != NULL)
		m_editData3.SubclassWindow(pWnd->GetSafeHwnd());
	
	// 初始化显示控件（如果控件存在）
	if (m_editData1.GetSafeHwnd() != NULL)
		m_editData1.SetWindowText(_T("0.00"));
	if (m_editData2.GetSafeHwnd() != NULL)
		m_editData2.SetWindowText(_T("0.00"));
	if (m_editData3.GetSafeHwnd() != NULL)
		m_editData3.SetWindowText(_T("0.00"));
	
	return TRUE;
}

// 更新数据显示
void CPage1Dlg::UpdateDisplay(const UdpRecvDataPacket* pPacket)
{
	if (pPacket == NULL)
		return;
	
	CString strData1, strData2, strData3;
	strData1.Format(_T("%.2f"), pPacket->pitchAngle / 1.0f);
	strData2.Format(_T("%.2f"), pPacket->rollAngle / 1.0f);
	strData3.Format(_T("%.2f"), pPacket->yawAngle / 1.0f);
	
	if (m_editData1.GetSafeHwnd() != NULL)
		m_editData1.SetWindowText(strData1);
	if (m_editData2.GetSafeHwnd() != NULL)
		m_editData2.SetWindowText(strData2);
	if (m_editData3.GetSafeHwnd() != NULL)
		m_editData3.SetWindowText(strData3);
}

