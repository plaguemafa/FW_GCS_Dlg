// Page2Dlg.cpp: 第二页子对话框实现文件
//

#include "pch.h"
#include "framework.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include "FW_GCS_Dlg.h"
#include "FW_GCS_DlgDlg.h"
#include "Page2Dlg.h"
#include "UdpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPage2Dlg 对话框

IMPLEMENT_DYNAMIC(CPage2Dlg, CDialogEx)

CPage2Dlg::CPage2Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PAGE2_DIALOG, pParent)
	, m_pMainDlg(NULL)
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
	ON_BN_CLICKED(IDC_BUTTON_SendData, &CPage2Dlg::OnBnClickedButtonSendData)
END_MESSAGE_MAP()

// CPage2Dlg 消息处理程序

BOOL CPage2Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	// 绑定发送数据输入控件
	CWnd* pWnd = GetDlgItem(IDC_Display_EditData0);
	if (pWnd != NULL)
		m_editSendData0.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData1);
	if (pWnd != NULL)
		m_editSendData1.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData2);
	if (pWnd != NULL)
		m_editSendData2.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData3);
	if (pWnd != NULL)
		m_editSendData3.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData4);
	if (pWnd != NULL)
		m_editSendData4.SubclassWindow(pWnd->GetSafeHwnd());
	
	// 初始化发送数据输入控件
	if (m_editSendData0.GetSafeHwnd() != NULL)
		m_editSendData0.SetWindowText(_T("0.00"));
	if (m_editSendData1.GetSafeHwnd() != NULL)
		m_editSendData1.SetWindowText(_T("0.00"));
	if (m_editSendData2.GetSafeHwnd() != NULL)
		m_editSendData2.SetWindowText(_T("0.00"));
	if (m_editSendData3.GetSafeHwnd() != NULL)
		m_editSendData3.SetWindowText(_T("0.00"));
	if (m_editSendData4.GetSafeHwnd() != NULL)
		m_editSendData4.SetWindowText(_T("0.00"));
	
	return TRUE;
}

// 更新数据显示（Page2当前不显示接收数据，保留接口以兼容主对话框调用）
void CPage2Dlg::UpdateDisplay(const UdpRecvDataPacket* pPacket)
{
	// Page2对话框当前不显示接收数据，此函数保留为空实现
	// 如果将来需要在Page2显示接收数据，可以在此添加相应控件和更新逻辑
	(void)pPacket;  // 避免未使用参数警告
}

//UDP发送
void CPage2Dlg::OnBnClickedButtonSendData()
{
	TRACE(_T("OnBnClickedButtonSendData: 函数被调用\n"));
	
	// 防止重复点击：禁用按钮
	TRACE(_T("OnBnClickedButtonSendData: 开始获取按钮控件\n"));
	CButton* pBtn = (CButton*)GetDlgItem(IDC_BUTTON_SendData);
	TRACE(_T("OnBnClickedButtonSendData: GetDlgItem返回 %p\n"), pBtn);
	
	if (pBtn != NULL)
	{
		TRACE(_T("OnBnClickedButtonSendData: 禁用按钮\n"));
		pBtn->EnableWindow(FALSE);
		TRACE(_T("OnBnClickedButtonSendData: 按钮已禁用\n"));
	}
	else
	{
		TRACE(_T("OnBnClickedButtonSendData: 警告：按钮控件未找到\n"));
	}

	// 使用RAII模式确保按钮状态总是被恢复
	struct ButtonEnabler
	{
		CButton* m_pBtn;
		ButtonEnabler(CButton* pBtn) : m_pBtn(pBtn) {}
		~ButtonEnabler() { if (m_pBtn != NULL) m_pBtn->EnableWindow(TRUE); }
	} btnEnabler(pBtn);

	CString strData0, strData1, strData2, strData3, strData4;
	if (m_editSendData0.GetSafeHwnd() != NULL)
		m_editSendData0.GetWindowText(strData0);
	if (m_editSendData1.GetSafeHwnd() != NULL)
		m_editSendData1.GetWindowText(strData1);
	if (m_editSendData2.GetSafeHwnd() != NULL)
		m_editSendData2.GetWindowText(strData2);
	if (m_editSendData3.GetSafeHwnd() != NULL)
		m_editSendData3.GetWindowText(strData3);
	if (m_editSendData4.GetSafeHwnd() != NULL)
		m_editSendData4.GetWindowText(strData4);

	UdpSendDataPacket packet{};
	packet.data1 = static_cast<float>(_tstof(strData0));
	packet.data2 = static_cast<float>(_tstof(strData1));
	packet.data3 = static_cast<float>(_tstof(strData2));
	packet.data4 = static_cast<float>(_tstof(strData3));
	packet.data5 = static_cast<float>(_tstof(strData4));

	// 调试输出：检查发送的数据和结构体大小
	TRACE(_T("UDP发送: data1=%.2f, data2=%.2f, data3=%.2f, data4=%.2f, data5=%.2f, 结构体大小=%d字节\n"), 
		packet.data1, packet.data2, packet.data3, packet.data4, packet.data5, sizeof(UdpSendDataPacket));
	
	// 调试输出：显示原始字节（用于诊断）
	BYTE* pBytes = (BYTE*)&packet;
	TRACE(_T("UDP发送原始字节[%d字节]: "), sizeof(UdpSendDataPacket));
	for (int i = 0; i < sizeof(UdpSendDataPacket); i++)
	{
		TRACE(_T("%02X "), pBytes[i]);
	}
	TRACE(_T("\n"));

	TRACE(_T("OnBnClickedButtonSendData: 使用存储的主对话框指针\n"));
	
	// 使用存储的主对话框指针（避免dynamic_cast）
	if (m_pMainDlg == NULL)
	{
		TRACE(_T("错误：主对话框指针未设置，UDP发送失败。\n"));
		MessageBox(_T("无法获取主对话框，UDP发送失败。"), _T("错误"), MB_OK | MB_ICONERROR | MB_TOPMOST);
		return;  // btnEnabler析构函数会自动恢复按钮状态
	}

	TRACE(_T("OnBnClickedButtonSendData: 开始调用SendUdpDataPublic\n"));
	
	// 发送数据（非阻塞）
	BOOL bResult = m_pMainDlg->SendUdpDataPublic(&packet, sizeof(packet));
	
	TRACE(_T("OnBnClickedButtonSendData: SendUdpDataPublic返回 %d\n"), bResult);
	
	if (!bResult)
	{
		TRACE(_T("错误：UDP未连接或发送失败，请检查连接后重试。\n"));
		MessageBox(_T("UDP未连接或发送失败，请检查连接后重试。"), _T("发送失败"), MB_OK | MB_ICONERROR | MB_TOPMOST);
		return;  // btnEnabler析构函数会自动恢复按钮状态
	}

	// 发送成功
	TRACE(_T("UDP数据发送成功。\n"));
	MessageBox(_T("UDP数据发送成功。"), _T("发送成功"), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
	// btnEnabler析构函数会自动恢复按钮状态
}
