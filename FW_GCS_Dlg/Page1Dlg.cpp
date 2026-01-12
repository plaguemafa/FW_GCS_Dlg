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
	
	// 绑定视窗组9相关控件（自检结果）
	CWnd* pWnd = GetDlgItem(IDC_Display66);
	if (pWnd != NULL)
		m_editDisplay66.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display67);
	if (pWnd != NULL)
		m_editDisplay67.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display68);
	if (pWnd != NULL)
		m_editDisplay68.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display69);
	if (pWnd != NULL)
		m_editDisplay69.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display70);
	if (pWnd != NULL)
		m_editDisplay70.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display71);
	if (pWnd != NULL)
		m_editDisplay71.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display72);
	if (pWnd != NULL)
		m_editDisplay72.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display73);
	if (pWnd != NULL)
		m_editDisplay73.SubclassWindow(pWnd->GetSafeHwnd());
	
	// 初始化视窗组9相关控件（自检结果）
	if (m_editDisplay66.GetSafeHwnd() != NULL)
		m_editDisplay66.SetWindowText(_T("0"));
	if (m_editDisplay67.GetSafeHwnd() != NULL)
		m_editDisplay67.SetWindowText(_T("0"));
	if (m_editDisplay68.GetSafeHwnd() != NULL)
		m_editDisplay68.SetWindowText(_T("0"));
	if (m_editDisplay69.GetSafeHwnd() != NULL)
		m_editDisplay69.SetWindowText(_T("0"));
	if (m_editDisplay70.GetSafeHwnd() != NULL)
		m_editDisplay70.SetWindowText(_T("0"));
	if (m_editDisplay71.GetSafeHwnd() != NULL)
		m_editDisplay71.SetWindowText(_T("0"));
	if (m_editDisplay72.GetSafeHwnd() != NULL)
		m_editDisplay72.SetWindowText(_T("0"));
	if (m_editDisplay73.GetSafeHwnd() != NULL)
		m_editDisplay73.SetWindowText(_T("0"));
	
	return TRUE;
}

// 更新数据显示
void CPage1Dlg::UpdateDisplay(const UdpRecvDataPacket* pPacket)
{
	if (pPacket == NULL)
		return;
	
	// 更新视窗组9相关字段（自检结果）
	CString strData66, strData67, strData68, strData69, strData70, strData71, strData72, strData73;
	strData66.Format(_T("%u"), pPacket->YIS100A_result);
	strData67.Format(_T("%u"), pPacket->HP5804_result);
	strData68.Format(_T("%u"), pPacket->MS4525D_result);
	strData69.Format(_T("%u"), pPacket->M401_result);
	strData70.Format(_T("%u"), pPacket->GPS_result);
	strData71.Format(_T("%u"), pPacket->PAC1931_result1);
	strData72.Format(_T("%u"), pPacket->can_to_pw_result);
	strData73.Format(_T("%u"), pPacket->SBUS_result);
	
	if (m_editDisplay66.GetSafeHwnd() != NULL)
		m_editDisplay66.SetWindowText(strData66);
	if (m_editDisplay67.GetSafeHwnd() != NULL)
		m_editDisplay67.SetWindowText(strData67);
	if (m_editDisplay68.GetSafeHwnd() != NULL)
		m_editDisplay68.SetWindowText(strData68);
	if (m_editDisplay69.GetSafeHwnd() != NULL)
		m_editDisplay69.SetWindowText(strData69);
	if (m_editDisplay70.GetSafeHwnd() != NULL)
		m_editDisplay70.SetWindowText(strData70);
	if (m_editDisplay71.GetSafeHwnd() != NULL)
		m_editDisplay71.SetWindowText(strData71);
	if (m_editDisplay72.GetSafeHwnd() != NULL)
		m_editDisplay72.SetWindowText(strData72);
	if (m_editDisplay73.GetSafeHwnd() != NULL)
		m_editDisplay73.SetWindowText(strData73);
}

