// Page2Dlg.cpp: 第二页子对话框实现文件
//

#include "pch.h"
#include "framework.h"
#include "FW_GCS_Dlg.h"
#include "Page2Dlg.h"
#include "UdpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPage2Dlg 对话框

IMPLEMENT_DYNAMIC(CPage2Dlg, CDialogEx)

CPage2Dlg::CPage2Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PAGE2_DIALOG, pParent)
{
}

CPage2Dlg::~CPage2Dlg()
{
}

void CPage2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// 可选绑定：如果控件存在则绑定，不存在也不报错
	// 注意：DoDataExchange 在对话框创建时调用，此时控件可能还未创建
	// 所以这里先不绑定，在 OnInitDialog 中再尝试绑定
	// 如果控件不存在，UpdateDisplay 函数中已经做了检查，不会出错
}

BEGIN_MESSAGE_MAP(CPage2Dlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CPage2Dlg::OnBnClickedButton1)
END_MESSAGE_MAP()

// CPage2Dlg 消息处理程序

BOOL CPage2Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	// 尝试绑定控件（如果控件存在）
	CWnd* pWnd = GetDlgItem(IDC_Display3);
	if (pWnd != NULL)
		m_editData4.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display4);
	if (pWnd != NULL)
		m_editData5.SubclassWindow(pWnd->GetSafeHwnd());
	
	// 初始化显示控件（如果控件存在）
	if (m_editData4.GetSafeHwnd() != NULL)
		m_editData4.SetWindowText(_T("0.00"));
	if (m_editData5.GetSafeHwnd() != NULL)
		m_editData5.SetWindowText(_T("0.00"));
	
	return TRUE;
}

// 更新数据显示
void CPage2Dlg::UpdateDisplay(const UdpRecvDataPacket* pPacket)
{
	if (pPacket == NULL)
		return;
	
	CString strData4, strData5;
	strData4.Format(_T("%.2f"), pPacket->attackAngle / 1.0f);
	strData5.Format(_T("%.2f"), pPacket->sideslipAngle / 1.0f);
	
	if (m_editData4.GetSafeHwnd() != NULL)
		m_editData4.SetWindowText(strData4);
	if (m_editData5.GetSafeHwnd() != NULL)
		m_editData5.SetWindowText(strData5);
}

//UDP发送
void CPage2Dlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
}
