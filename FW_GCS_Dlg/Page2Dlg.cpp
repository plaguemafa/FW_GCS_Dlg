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
	
	// 绑定发送数据输入控件（IDC_Display_EditData0 到 IDC_Display_EditData28）
	CWnd* pWnd = GetDlgItem(IDC_Display_EditData0);
	if (pWnd != NULL) m_editSendData0.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData1);
	if (pWnd != NULL) m_editSendData1.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData2);
	if (pWnd != NULL) m_editSendData2.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData3);
	if (pWnd != NULL) m_editSendData3.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData4);
	if (pWnd != NULL) m_editSendData4.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData5);
	if (pWnd != NULL) m_editSendData5.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData6);
	if (pWnd != NULL) m_editSendData6.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData7);
	if (pWnd != NULL) m_editSendData7.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData8);
	if (pWnd != NULL) m_editSendData8.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData9);
	if (pWnd != NULL) m_editSendData9.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData10);
	if (pWnd != NULL) m_editSendData10.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData11);
	if (pWnd != NULL) m_editSendData11.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData12);
	if (pWnd != NULL) m_editSendData12.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData13);
	if (pWnd != NULL) m_editSendData13.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData14);
	if (pWnd != NULL) m_editSendData14.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData15);
	if (pWnd != NULL) m_editSendData15.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData16);
	if (pWnd != NULL) m_editSendData16.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData17);
	if (pWnd != NULL) m_editSendData17.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData18);
	if (pWnd != NULL) m_editSendData18.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData19);
	if (pWnd != NULL) m_editSendData19.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData20);
	if (pWnd != NULL) m_editSendData20.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData21);
	if (pWnd != NULL) m_editSendData21.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData22);
	if (pWnd != NULL) m_editSendData22.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData23);
	if (pWnd != NULL) m_editSendData23.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData24);
	if (pWnd != NULL) m_editSendData24.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData25);
	if (pWnd != NULL) m_editSendData25.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData26);
	if (pWnd != NULL) m_editSendData26.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData27);
	if (pWnd != NULL) m_editSendData27.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_Display_EditData28);
	if (pWnd != NULL) m_editSendData28.SubclassWindow(pWnd->GetSafeHwnd());
	
	// 初始化发送数据输入控件（根据数据类型设置默认值）
	if (m_editSendData0.GetSafeHwnd() != NULL) m_editSendData0.SetWindowText(_T("0"));      // uint8_t
	if (m_editSendData1.GetSafeHwnd() != NULL) m_editSendData1.SetWindowText(_T("0"));      // uint8_t
	if (m_editSendData2.GetSafeHwnd() != NULL) m_editSendData2.SetWindowText(_T("0"));      // int32_t
	if (m_editSendData3.GetSafeHwnd() != NULL) m_editSendData3.SetWindowText(_T("0"));      // int32_t
	if (m_editSendData4.GetSafeHwnd() != NULL) m_editSendData4.SetWindowText(_T("0"));      // int16_t
	if (m_editSendData5.GetSafeHwnd() != NULL) m_editSendData5.SetWindowText(_T("0"));      // int16_t
	if (m_editSendData6.GetSafeHwnd() != NULL) m_editSendData6.SetWindowText(_T("0"));      // int16_t
	if (m_editSendData7.GetSafeHwnd() != NULL) m_editSendData7.SetWindowText(_T("0"));      // int16_t
	if (m_editSendData8.GetSafeHwnd() != NULL) m_editSendData8.SetWindowText(_T("0"));      // int16_t
	if (m_editSendData9.GetSafeHwnd() != NULL) m_editSendData9.SetWindowText(_T("0"));      // int16_t
	if (m_editSendData10.GetSafeHwnd() != NULL) m_editSendData10.SetWindowText(_T("0"));    // int16_t
	if (m_editSendData11.GetSafeHwnd() != NULL) m_editSendData11.SetWindowText(_T("0"));    // int16_t
	if (m_editSendData12.GetSafeHwnd() != NULL) m_editSendData12.SetWindowText(_T("0"));    // int16_t
	if (m_editSendData13.GetSafeHwnd() != NULL) m_editSendData13.SetWindowText(_T("0"));    // int16_t
	if (m_editSendData14.GetSafeHwnd() != NULL) m_editSendData14.SetWindowText(_T("0"));    // int16_t
	if (m_editSendData15.GetSafeHwnd() != NULL) m_editSendData15.SetWindowText(_T("0"));    // int16_t
	if (m_editSendData16.GetSafeHwnd() != NULL) m_editSendData16.SetWindowText(_T("0"));    // int16_t
	if (m_editSendData17.GetSafeHwnd() != NULL) m_editSendData17.SetWindowText(_T("0"));    // int32_t
	if (m_editSendData18.GetSafeHwnd() != NULL) m_editSendData18.SetWindowText(_T("0"));    // int32_t
	if (m_editSendData19.GetSafeHwnd() != NULL) m_editSendData19.SetWindowText(_T("0"));    // int16_t
	if (m_editSendData20.GetSafeHwnd() != NULL) m_editSendData20.SetWindowText(_T("0"));    // int32_t
	if (m_editSendData21.GetSafeHwnd() != NULL) m_editSendData21.SetWindowText(_T("0"));    // int32_t
	if (m_editSendData22.GetSafeHwnd() != NULL) m_editSendData22.SetWindowText(_T("0"));    // int16_t
	if (m_editSendData23.GetSafeHwnd() != NULL) m_editSendData23.SetWindowText(_T("0"));    // int32_t
	if (m_editSendData24.GetSafeHwnd() != NULL) m_editSendData24.SetWindowText(_T("0"));    // int32_t
	if (m_editSendData25.GetSafeHwnd() != NULL) m_editSendData25.SetWindowText(_T("0"));    // int16_t
	if (m_editSendData26.GetSafeHwnd() != NULL) m_editSendData26.SetWindowText(_T("0"));    // int8_t
	if (m_editSendData27.GetSafeHwnd() != NULL) m_editSendData27.SetWindowText(_T("0"));    // int8_t
	if (m_editSendData28.GetSafeHwnd() != NULL) m_editSendData28.SetWindowText(_T("0"));    // uint8_t
	
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

	// 获取所有控件数据
	CString strData[29];
	if (m_editSendData0.GetSafeHwnd() != NULL) m_editSendData0.GetWindowText(strData[0]);
	if (m_editSendData1.GetSafeHwnd() != NULL) m_editSendData1.GetWindowText(strData[1]);
	if (m_editSendData2.GetSafeHwnd() != NULL) m_editSendData2.GetWindowText(strData[2]);
	if (m_editSendData3.GetSafeHwnd() != NULL) m_editSendData3.GetWindowText(strData[3]);
	if (m_editSendData4.GetSafeHwnd() != NULL) m_editSendData4.GetWindowText(strData[4]);
	if (m_editSendData5.GetSafeHwnd() != NULL) m_editSendData5.GetWindowText(strData[5]);
	if (m_editSendData6.GetSafeHwnd() != NULL) m_editSendData6.GetWindowText(strData[6]);
	if (m_editSendData7.GetSafeHwnd() != NULL) m_editSendData7.GetWindowText(strData[7]);
	if (m_editSendData8.GetSafeHwnd() != NULL) m_editSendData8.GetWindowText(strData[8]);
	if (m_editSendData9.GetSafeHwnd() != NULL) m_editSendData9.GetWindowText(strData[9]);
	if (m_editSendData10.GetSafeHwnd() != NULL) m_editSendData10.GetWindowText(strData[10]);
	if (m_editSendData11.GetSafeHwnd() != NULL) m_editSendData11.GetWindowText(strData[11]);
	if (m_editSendData12.GetSafeHwnd() != NULL) m_editSendData12.GetWindowText(strData[12]);
	if (m_editSendData13.GetSafeHwnd() != NULL) m_editSendData13.GetWindowText(strData[13]);
	if (m_editSendData14.GetSafeHwnd() != NULL) m_editSendData14.GetWindowText(strData[14]);
	if (m_editSendData15.GetSafeHwnd() != NULL) m_editSendData15.GetWindowText(strData[15]);
	if (m_editSendData16.GetSafeHwnd() != NULL) m_editSendData16.GetWindowText(strData[16]);
	if (m_editSendData17.GetSafeHwnd() != NULL) m_editSendData17.GetWindowText(strData[17]);
	if (m_editSendData18.GetSafeHwnd() != NULL) m_editSendData18.GetWindowText(strData[18]);
	if (m_editSendData19.GetSafeHwnd() != NULL) m_editSendData19.GetWindowText(strData[19]);
	if (m_editSendData20.GetSafeHwnd() != NULL) m_editSendData20.GetWindowText(strData[20]);
	if (m_editSendData21.GetSafeHwnd() != NULL) m_editSendData21.GetWindowText(strData[21]);
	if (m_editSendData22.GetSafeHwnd() != NULL) m_editSendData22.GetWindowText(strData[22]);
	if (m_editSendData23.GetSafeHwnd() != NULL) m_editSendData23.GetWindowText(strData[23]);
	if (m_editSendData24.GetSafeHwnd() != NULL) m_editSendData24.GetWindowText(strData[24]);
	if (m_editSendData25.GetSafeHwnd() != NULL) m_editSendData25.GetWindowText(strData[25]);
	if (m_editSendData26.GetSafeHwnd() != NULL) m_editSendData26.GetWindowText(strData[26]);
	if (m_editSendData27.GetSafeHwnd() != NULL) m_editSendData27.GetWindowText(strData[27]);
	if (m_editSendData28.GetSafeHwnd() != NULL) m_editSendData28.GetWindowText(strData[28]);

	// 初始化数据包结构体
	UdpSendDataPacket packet{};
	memset(&packet, 0, sizeof(packet));  // 清零，包括waypoints数组
	
	// 填充数据（根据数据类型转换）
	packet.missionCommand = static_cast<uint8_t>(_ttoi(strData[0]));           // uint8_t
	packet.controlMode = static_cast<uint8_t>(_ttoi(strData[1]));               // uint8_t
	packet.launchLongitude = static_cast<int32_t>(_ttoi(strData[2]));           // int32_t
	packet.launchLatitude = static_cast<int32_t>(_ttoi(strData[3]));            // int32_t
	packet.launchAltitude = static_cast<int16_t>(_ttoi(strData[4]));            // int16_t
	packet.initPitch = static_cast<int16_t>(_ttoi(strData[5]));                 // int16_t
	packet.initYaw = static_cast<int16_t>(_ttoi(strData[6]));                   // int16_t
	packet.initRoll = static_cast<int16_t>(_ttoi(strData[7]));                  // int16_t
	packet.initPitchRate = static_cast<int16_t>(_ttoi(strData[8]));             // int16_t
	packet.initYawRate = static_cast<int16_t>(_ttoi(strData[9]));               // int16_t
	packet.initRollRate = static_cast<int16_t>(_ttoi(strData[10]));             // int16_t
	packet.initNorthVelocity = static_cast<int16_t>(_ttoi(strData[11]));        // int16_t
	packet.initEastVelocity = static_cast<int16_t>(_ttoi(strData[12]));         // int16_t
	packet.initVerticalVelocity = static_cast<int16_t>(_ttoi(strData[13]));      // int16_t
	packet.initNorthAccel = static_cast<int16_t>(_ttoi(strData[14]));            // int16_t
	packet.initEastAccel = static_cast<int16_t>(_ttoi(strData[15]));             // int16_t
	packet.initVerticalAccel = static_cast<int16_t>(_ttoi(strData[16]));         // int16_t
	// waypoints[100] 数组保持为0（没有对应的IDC控件）
	packet.targetLongitude = static_cast<int32_t>(_ttoi(strData[17]));          // int32_t
	packet.targetLatitude = static_cast<int32_t>(_ttoi(strData[18]));            // int32_t
	packet.targetAltitude = static_cast<int16_t>(_ttoi(strData[19]));            // int16_t
	packet.launchLongitude2 = static_cast<int32_t>(_ttoi(strData[20]));          // int32_t
	packet.launchLatitude2 = static_cast<int32_t>(_ttoi(strData[21]));           // int32_t
	packet.launchAltitude2 = static_cast<int16_t>(_ttoi(strData[22]));           // int16_t
	packet.parachuteLongitude = static_cast<int32_t>(_ttoi(strData[23]));       // int32_t
	packet.parachuteLatitude = static_cast<int32_t>(_ttoi(strData[24]));          // int32_t
	packet.parachuteAltitude = static_cast<int16_t>(_ttoi(strData[25]));         // int16_t
	packet.elevatorCmd = static_cast<int8_t>(_ttoi(strData[26]));                // int8_t
	packet.aileronCmd = static_cast<int8_t>(_ttoi(strData[27]));                  // int8_t
	packet.airspeedSet = static_cast<uint8_t>(_ttoi(strData[28]));               // uint8_t

	// 调试输出：检查发送的数据和结构体大小
	TRACE(_T("UDP发送: missionCommand=%u, controlMode=%u, launchLongitude=%d, launchLatitude=%d, launchAltitude=%d\n"), 
		packet.missionCommand, packet.controlMode, packet.launchLongitude, packet.launchLatitude, packet.launchAltitude);
	TRACE(_T("UDP发送: 结构体大小=%d字节\n"), sizeof(UdpSendDataPacket));
	
	// 调试输出：显示原始字节（用于诊断，只显示前32字节）
	BYTE* pBytes = (BYTE*)&packet;
	TRACE(_T("UDP发送原始字节[前32字节]: "));
	int nBytesToShow = (sizeof(UdpSendDataPacket) < 32) ? sizeof(UdpSendDataPacket) : 32;
	for (int i = 0; i < nBytesToShow; i++)
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
