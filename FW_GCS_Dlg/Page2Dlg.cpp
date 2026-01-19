// Page2Dlg.cpp: 第二页子对话框实现文件
//

#include "pch.h"
#include "framework.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <msxml6.h>
#pragma comment(lib, "msxml6.lib")
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
	, m_nEditingItem(-1)
	, m_nEditingSubItem(-1)
	, m_nCurrentWaypointCount(0)
{
	memset(m_currentWaypoints, 0, sizeof(m_currentWaypoints));
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
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_Waypoints, &CPage2Dlg::OnNMDblclkListWaypoints)
	ON_NOTIFY(NM_CLICK, IDC_LIST_Waypoints, &CPage2Dlg::OnNMClickListWaypoints)
	ON_EN_KILLFOCUS(1001, &CPage2Dlg::OnEnKillfocusEditInline)
END_MESSAGE_MAP()

// CPage2Dlg 消息处理程序

BOOL CPage2Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	// 绑定发送数据输入控件（IDC_Display_EditData2 到 IDC_Display_EditData28）
	// 注意：IDC_Display_EditData0 和 IDC_Display_EditData1 已弃用，改用 Radio Button 控件
	CWnd* pWnd = GetDlgItem(IDC_Display_EditData2);
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

	// 绑定扩展协议指令/模式 Radio 控件（仅当资源存在）
	pWnd = GetDlgItem(IDC_RADIO_Flag22);
	if (pWnd != NULL) m_radioMissionCmd0.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_RADIO_Flag23);
	if (pWnd != NULL) m_radioMissionCmd1.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_RADIO_Flag24);
	if (pWnd != NULL) m_radioMissionCmd2.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_RADIO_Flag25);
	if (pWnd != NULL) m_radioMissionCmd3.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_RADIO_Flag26);
	if (pWnd != NULL) m_radioMissionCmd4.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_RADIO_Flag27);
	if (pWnd != NULL) m_radioMissionCmd5.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_RADIO_Flag28);
	if (pWnd != NULL) m_radioMissionCmd6.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_RADIO_Flag29);
	if (pWnd != NULL) m_radioCtrlMode0.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_RADIO_Flag30);
	if (pWnd != NULL) m_radioCtrlMode1.SubclassWindow(pWnd->GetSafeHwnd());
	pWnd = GetDlgItem(IDC_RADIO_Flag31);
	if (pWnd != NULL) m_radioCtrlMode2.SubclassWindow(pWnd->GetSafeHwnd());
	
	// 初始化发送数据输入控件（根据数据类型设置默认值）
	// 注意：m_editSendData0 和 m_editSendData1 已弃用（missionCommand 和 controlMode 改用 Radio Button）
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

	// 默认指令/模式 Radio 状态
	if (m_radioMissionCmd0.GetSafeHwnd() != NULL) m_radioMissionCmd0.SetCheck(BST_CHECKED); // 默认地面测试流程
	if (m_radioMissionCmd1.GetSafeHwnd() != NULL) m_radioMissionCmd1.SetCheck(BST_UNCHECKED);
	if (m_radioMissionCmd2.GetSafeHwnd() != NULL) m_radioMissionCmd2.SetCheck(BST_UNCHECKED);
	if (m_radioMissionCmd3.GetSafeHwnd() != NULL) m_radioMissionCmd3.SetCheck(BST_UNCHECKED);
	if (m_radioMissionCmd4.GetSafeHwnd() != NULL) m_radioMissionCmd4.SetCheck(BST_UNCHECKED);
	if (m_radioMissionCmd5.GetSafeHwnd() != NULL) m_radioMissionCmd5.SetCheck(BST_UNCHECKED);
	if (m_radioMissionCmd6.GetSafeHwnd() != NULL) m_radioMissionCmd6.SetCheck(BST_UNCHECKED);
	if (m_radioCtrlMode0.GetSafeHwnd() != NULL) m_radioCtrlMode0.SetCheck(BST_CHECKED);     // 默认手动
	if (m_radioCtrlMode1.GetSafeHwnd() != NULL) m_radioCtrlMode1.SetCheck(BST_UNCHECKED);
	if (m_radioCtrlMode2.GetSafeHwnd() != NULL) m_radioCtrlMode2.SetCheck(BST_UNCHECKED);
	
	// 绑定航路点相关控件
	pWnd = GetDlgItem(IDC_CHECK_LoadWaypoints);
	if (pWnd != NULL) m_chkLoadWaypoints.SubclassWindow(pWnd->GetSafeHwnd());
	
	// 初始化航路点相关控件
	// 注意：复选框默认勾选，因为列表会在初始化时自动加载XML数据
	m_chkLoadWaypoints.SetCheck(BST_CHECKED);  // 默认勾选（因为列表已自动加载）
	
	// 初始化航路点列表控件
	pWnd = GetDlgItem(IDC_LIST_Waypoints);
	if (pWnd != NULL)
	{
		m_listWaypoints.SubclassWindow(pWnd->GetSafeHwnd());
		
		// 设置列表控件为报告视图（表格模式）- 必须设置，否则无法显示列
		DWORD dwStyle = m_listWaypoints.GetStyle();
		dwStyle &= ~(LVS_TYPEMASK);  // 清除现有视图类型
		dwStyle |= LVS_REPORT;       // 设置为报告视图
		m_listWaypoints.ModifyStyle(0, dwStyle);
		
		// 设置扩展样式
		m_listWaypoints.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
		
		// 添加列标题
		m_listWaypoints.InsertColumn(0, _T("编号"), LVCFMT_LEFT, 60);
		m_listWaypoints.InsertColumn(1, _T("经度(度)"), LVCFMT_LEFT, 120);
		m_listWaypoints.InsertColumn(2, _T("纬度(度)"), LVCFMT_LEFT, 120);
		m_listWaypoints.InsertColumn(3, _T("高度(米)"), LVCFMT_LEFT, 100);
		
		TRACE(_T("航路点列表控件初始化完成\n"));
		
		// ============================================================
		// 对话框初始化时自动加载XML文件并显示到列表
		// ============================================================
		Waypoint waypoints[100];
		int nLoadedCount = 0;
		if (LoadWaypointsFromXml(waypoints, nLoadedCount))
		{
			TRACE(_T("对话框初始化：成功加载 %d 个航路点\n"), nLoadedCount);
			DisplayWaypoints(waypoints, nLoadedCount);
		}
		else
		{
			TRACE(_T("对话框初始化：航路点文件加载失败或文件不存在，列表为空\n"));
			// 不显示错误消息框，因为文件可能不存在是正常情况
			m_listWaypoints.DeleteAllItems();
			m_nCurrentWaypointCount = 0;
		}
	}
	else
	{
		TRACE(_T("警告：未找到IDC_LIST_Waypoints控件\n"));
	}
	
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
	// 注意：strData[0] 和 strData[1] 已弃用（missionCommand 和 controlMode 改用 Radio Button）
	CString strData[29];
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
	// missionCommand: 由 Radio Flag22~28 决定
	packet.missionCommand_B0 = (m_radioMissionCmd1.GetSafeHwnd() != NULL && m_radioMissionCmd1.GetCheck() == BST_CHECKED) ? 1 : 0;
	packet.missionCommand_B1 = (m_radioMissionCmd2.GetSafeHwnd() != NULL && m_radioMissionCmd2.GetCheck() == BST_CHECKED) ? 1 : 0;
	packet.missionCommand_B2 = (m_radioMissionCmd3.GetSafeHwnd() != NULL && m_radioMissionCmd3.GetCheck() == BST_CHECKED) ? 1 : 0;
	packet.missionCommand_B3 = (m_radioMissionCmd4.GetSafeHwnd() != NULL && m_radioMissionCmd4.GetCheck() == BST_CHECKED) ? 1 : 0;
	packet.missionCommand_B4 = (m_radioMissionCmd5.GetSafeHwnd() != NULL && m_radioMissionCmd5.GetCheck() == BST_CHECKED) ? 1 : 0;
	packet.missionCommand_B5 = (m_radioMissionCmd6.GetSafeHwnd() != NULL && m_radioMissionCmd6.GetCheck() == BST_CHECKED) ? 1 : 0;

	// controlMode: Flag29=0 手动，Flag30=1 半自主，Flag31=2 全自主
	if (m_radioCtrlMode1.GetSafeHwnd() != NULL && m_radioCtrlMode1.GetCheck() == BST_CHECKED)
		packet.controlMode_B0 = 1;
	else if (m_radioCtrlMode2.GetSafeHwnd() != NULL && m_radioCtrlMode2.GetCheck() == BST_CHECKED)
		packet.controlMode_B0 = 2;
	else
		packet.controlMode_B0 = 0; // 默认手动
	// 注意：controlMode 只有 B0 字段，没有 B1 和 B2
	packet.launchLongitude = static_cast<int32_t>(_ttoi(strData[2]));            // int32_t
	packet.launchLatitude = static_cast<int32_t>(_ttoi(strData[3]));             // int32_t
	packet.launchAltitude = static_cast<int16_t>(_ttoi(strData[4]));             // int16_t
	packet.initPitch = static_cast<int16_t>(_ttoi(strData[5]));                  // int16_t
	packet.initYaw = static_cast<int16_t>(_ttoi(strData[6]));                    // int16_t
	packet.initRoll = static_cast<int16_t>(_ttoi(strData[7]));                   // int16_t
	packet.initPitchRate = static_cast<int16_t>(_ttoi(strData[8]));              // int16_t
	packet.initYawRate = static_cast<int16_t>(_ttoi(strData[9]));                // int16_t
	packet.initRollRate = static_cast<int16_t>(_ttoi(strData[10]));              // int16_t
	packet.initNorthVelocity = static_cast<int16_t>(_ttoi(strData[11]));         // int16_t
	packet.initEastVelocity = static_cast<int16_t>(_ttoi(strData[12]));          // int16_t
	packet.initVerticalVelocity = static_cast<int16_t>(_ttoi(strData[13]));      // int16_t
	packet.initNorthAccel = static_cast<int16_t>(_ttoi(strData[14]));            // int16_t
	packet.initEastAccel = static_cast<int16_t>(_ttoi(strData[15]));             // int16_t
	packet.initVerticalAccel = static_cast<int16_t>(_ttoi(strData[16]));         // int16_t
	
	// ============================================================
	// 根据复选框状态决定是否在发送数据包中包含航路点数据
	// ============================================================
	if (m_chkLoadWaypoints.GetCheck() == BST_CHECKED)
	{
		// 勾选：使用列表控件中的航路点数据（可能是编辑后的）
		if (m_nCurrentWaypointCount > 0)
		{
			TRACE(_T("勾选加载航路点：使用列表控件中的数据（%d个）\n"), m_nCurrentWaypointCount);
			GetWaypointsFromList(packet.waypoints, m_nCurrentWaypointCount);
		}
		else
		{
			// 列表为空，发送时航路点数组保持为0
			memset(packet.waypoints, 0, sizeof(packet.waypoints));
			TRACE(_T("勾选加载航路点：但列表为空，waypoints数组保持为0\n"));
		}
	}
	else
	{
		// 未勾选：发送时航路点数组保持为0（不发送航路点数据）
		memset(packet.waypoints, 0, sizeof(packet.waypoints));
		TRACE(_T("未勾选加载航路点：waypoints数组已清零（不发送航路点数据）\n"));
	}
	
	packet.targetLongitude = static_cast<int32_t>(_ttoi(strData[17]));          // int32_t
	packet.targetLatitude = static_cast<int32_t>(_ttoi(strData[18]));           // int32_t
	packet.targetAltitude = static_cast<int16_t>(_ttoi(strData[19]));           // int16_t
	packet.launchLongitude2 = static_cast<int32_t>(_ttoi(strData[20]));         // int32_t
	packet.launchLatitude2 = static_cast<int32_t>(_ttoi(strData[21]));          // int32_t
	packet.launchAltitude2 = static_cast<int16_t>(_ttoi(strData[22]));          // int16_t
	packet.parachuteLongitude = static_cast<int32_t>(_ttoi(strData[23]));       // int32_t
	packet.parachuteLatitude = static_cast<int32_t>(_ttoi(strData[24]));        // int32_t
	packet.parachuteAltitude = static_cast<int16_t>(_ttoi(strData[25]));        // int16_t
	packet.elevatorCmd = static_cast<int8_t>(_ttoi(strData[26]));               // int8_t
	packet.aileronCmd = static_cast<int8_t>(_ttoi(strData[27]));                // int8_t
	packet.airspeedSet = static_cast<uint8_t>(_ttoi(strData[28]));              // uint8_t

	// 调试输出：检查发送的数据和结构体大小
	TRACE(_T("UDP发送: missionCommand_B0~B5=%u,%u,%u,%u,%u,%u; controlMode_B0=%u; launchLon=%d, launchLat=%d, launchAlt=%d\n"),
		packet.missionCommand_B0, packet.missionCommand_B1, packet.missionCommand_B2,
		packet.missionCommand_B3, packet.missionCommand_B4, packet.missionCommand_B5,
		packet.controlMode_B0,
		packet.launchLongitude, packet.launchLatitude, packet.launchAltitude);
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

// 从XML文件加载航路点（文件路径：可执行文件目录下的waypoints.xml）
BOOL CPage2Dlg::LoadWaypointsFromXml(Waypoint waypoints[100], int& nLoadedCount)
{
	nLoadedCount = 0;
	memset(waypoints, 0, sizeof(Waypoint) * 100);
	
	// 获取可执行文件所在目录
	TCHAR szModulePath[MAX_PATH];
	GetModuleFileName(NULL, szModulePath, MAX_PATH);
	CString strExePath = szModulePath;
	int nLastSlash = strExePath.ReverseFind(_T('\\'));
	if (nLastSlash >= 0)
	{
		strExePath = strExePath.Left(nLastSlash + 1);
	}
	
	// 构建航路点文件完整路径（可执行文件目录下的waypoints.xml）
	CString strFilePath = strExePath + _T("waypoints.xml");
	
	TRACE(_T("尝试加载航路点文件: %s\n"), strFilePath);
	
	// 检查文件是否存在
	CFileStatus status;
	if (!CFile::GetStatus(strFilePath, status))
	{
		TRACE(_T("航路点文件不存在: %s\n"), strFilePath);
		CString strMsg;
		strMsg.Format(_T("航路点文件不存在:\n%s"), (LPCTSTR)strFilePath);
		MessageBox(strMsg, _T("XML文件加载失败"), MB_OK | MB_ICONWARNING);
		return FALSE;
	}
	
	// 初始化COM（如果尚未初始化）
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	BOOL bNeedUninit = SUCCEEDED(hr);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
	{
		TRACE(_T("COM初始化失败，错误代码: 0x%08X\n"), hr);
		return FALSE;
	}
	
	// 使用作用域块确保所有 COM 对象在 CoUninitialize() 之前析构
	{
		// 创建DOM文档对象
		CComPtr<IXMLDOMDocument> spXMLDoc;
		hr = spXMLDoc.CoCreateInstance(__uuidof(DOMDocument60));
		if (FAILED(hr))
		{
			TRACE(_T("创建XML文档对象失败，错误代码: 0x%08X\n"), hr);
			if (bNeedUninit) CoUninitialize();
			return FALSE;
		}
		
		// 设置异步加载为FALSE
		VARIANT_BOOL vbSuccess;
		spXMLDoc->put_async(VARIANT_FALSE);
		
		// 加载XML文件
		CComVariant varFileName(strFilePath);
		hr = spXMLDoc->load(varFileName, &vbSuccess);
		
		if (FAILED(hr) || vbSuccess != VARIANT_TRUE)
		{
			TRACE(_T("加载XML文件失败: %s\n"), strFilePath);
			if (bNeedUninit) CoUninitialize();
			CString strMsg;
			strMsg.Format(_T("加载XML文件失败\n"));
			MessageBox(strMsg, _T("xml文件加载失败"), MB_OK | MB_ICONINFORMATION);
			return FALSE;
		}
		
		TRACE(_T("XML文件加载成功\n"));
		
		// 获取根节点（根节点就是 <waypoints>）
		CComPtr<IXMLDOMElement> spRoot;
		hr = spXMLDoc->get_documentElement(&spRoot);
		if (FAILED(hr) || spRoot == NULL)
		{
			TRACE(_T("获取XML根节点失败\n"));
			if (bNeedUninit) CoUninitialize();
			return FALSE;
		}
		
		// 查找waypoint节点（根节点waypoints，直接查询其子节点waypoint）
		CComPtr<IXMLDOMNodeList> spNodeList;
		hr = spRoot->selectNodes(CComBSTR(_T("waypoint")), &spNodeList);
		if (FAILED(hr) || spNodeList == NULL)
		{
			TRACE(_T("查找waypoint节点失败，错误代码: 0x%08X\n"), hr);
			// 尝试使用 getElementsByTagName 方法
			hr = spRoot->getElementsByTagName(CComBSTR(_T("waypoint")), &spNodeList);
			if (FAILED(hr) || spNodeList == NULL)
			{
				TRACE(_T("使用getElementsByTagName查找waypoint节点也失败，错误代码: 0x%08X\n"), hr);
				if (bNeedUninit) CoUninitialize();
				return FALSE;
			}
			TRACE(_T("使用getElementsByTagName成功找到waypoint节点\n"));
		}
		
		// 获取节点数量
		long nNodeCount = 0;
		hr = spNodeList->get_length(&nNodeCount);
		if (FAILED(hr))
		{
			TRACE(_T("获取节点数量失败\n"));
			if (bNeedUninit) CoUninitialize();
			return FALSE;
		}
		
		// 限制最多100个航路点
		if (nNodeCount > 100)
		{
			TRACE(_T("警告：XML文件包含 %d 个航路点，只加载前100个\n"), nNodeCount);
			nNodeCount = 100;
		}
		
		// 遍历所有waypoint节点
		for (long i = 0; i < nNodeCount; i++)
		{
			CComPtr<IXMLDOMNode> spNode;
			hr = spNodeList->get_item(i, &spNode);
			if (FAILED(hr) || spNode == NULL)
				continue;
			
			// 获取longitude子节点（XML中为浮点数，转换为int32_t：度 * 1000000）
			CComPtr<IXMLDOMNode> spLongitudeNode;
			hr = spNode->selectSingleNode(CComBSTR(_T("longitude")), &spLongitudeNode);
			if (SUCCEEDED(hr) && spLongitudeNode != NULL)
			{
				CComBSTR bstrText;
				spLongitudeNode->get_text(&bstrText);
				if (bstrText.Length() > 0)
				{
					// 读取浮点数并转换为int32_t（度 * 1000000）
					double dLongitude = _tstof(CString(bstrText));
					waypoints[i].longitude = static_cast<int32_t>(dLongitude * 1000000.0);
					TRACE(_T("航路点 %d: longitude=%.6f度 -> %d\n"), i + 1, dLongitude, waypoints[i].longitude);
				}
			}
			
			// 获取latitude子节点（XML中为浮点数，转换为int32_t：度 * 1000000）
			CComPtr<IXMLDOMNode> spLatitudeNode;
			hr = spNode->selectSingleNode(CComBSTR(_T("latitude")), &spLatitudeNode);
			if (SUCCEEDED(hr) && spLatitudeNode != NULL)
			{
				CComBSTR bstrText;
				spLatitudeNode->get_text(&bstrText);
				if (bstrText.Length() > 0)
				{
					// 读取浮点数并转换为int32_t（度 * 1000000）
					double dLatitude = _tstof(CString(bstrText));
					waypoints[i].latitude = static_cast<int32_t>(dLatitude * 1000000.0);
					TRACE(_T("航路点 %d: latitude=%.6f度 -> %d\n"), i + 1, dLatitude, waypoints[i].latitude);
				}
			}
			
			// 获取altitude子节点（XML中为浮点数，转换为int16_t：米）
			CComPtr<IXMLDOMNode> spAltitudeNode;
			hr = spNode->selectSingleNode(CComBSTR(_T("altitude")), &spAltitudeNode);
			if (SUCCEEDED(hr) && spAltitudeNode != NULL)
			{
				CComBSTR bstrText;
				spAltitudeNode->get_text(&bstrText);
				if (bstrText.Length() > 0)
				{
					// 读取浮点数并转换为int16_t（米）
					double dAltitude = _tstof(CString(bstrText));
					waypoints[i].altitude = static_cast<int16_t>(dAltitude);
					TRACE(_T("航路点 %d: altitude=%.2f米 -> %d\n"), i + 1, dAltitude, waypoints[i].altitude);
				}
			}
			
			nLoadedCount++;
		}
		
		// 作用域块结束，所有 CComPtr 对象会自动析构并释放 COM 对象
		// 此时 COM 环境仍然有效，可以安全地释放对象
	}
	
	// 所有 COM 对象都已释放，安全卸载 COM
	if (bNeedUninit) CoUninitialize();
	
	TRACE(_T("成功加载 %d 个航路点\n"), nLoadedCount);
	return (nLoadedCount > 0);
}

// 显示航路点数据到列表控件
void CPage2Dlg::DisplayWaypoints(const Waypoint waypoints[100], int nCount)
{
	if (m_listWaypoints.GetSafeHwnd() == NULL)
	{
		TRACE(_T("错误：列表控件句柄无效，无法显示航路点\n"));
		return;
	}
	
	TRACE(_T("开始显示航路点数据，数量: %d\n"), nCount);
	
	// 清空现有数据
	m_listWaypoints.DeleteAllItems();
	
	// 限制显示数量（最多100个）
	if (nCount > 100)
		nCount = 100;
	
	if (nCount <= 0)
	{
		TRACE(_T("警告：航路点数量为0，不显示数据\n"));
		return;
	}
	
	// 添加航路点数据到列表
	for (int i = 0; i < nCount; i++)
	{
		// 将协议格式转换为显示格式
		// 经度/纬度：从 int32_t（度 * 1000000）转换为浮点数（度）
		double dLongitude = waypoints[i].longitude / 1000000.0;
		double dLatitude = waypoints[i].latitude / 1000000.0;
		// 高度：int16_t（米）直接显示
		int nAltitude = waypoints[i].altitude;
		
		// 格式化字符串
		CString strIndex, strLongitude, strLatitude, strAltitude;
		strIndex.Format(_T("%d"), i + 1);
		strLongitude.Format(_T("%.6f"), dLongitude);
		strLatitude.Format(_T("%.6f"), dLatitude);
		strAltitude.Format(_T("%d"), nAltitude);
		
		// 插入行
		int nItem = m_listWaypoints.InsertItem(i, strIndex);
		if (nItem >= 0)
		{
			m_listWaypoints.SetItemText(nItem, 1, strLongitude);
			m_listWaypoints.SetItemText(nItem, 2, strLatitude);
			m_listWaypoints.SetItemText(nItem, 3, strAltitude);
			
			// 调试输出前几个航路点
			if (i < 3)
			{
				TRACE(_T("航路点 %d: 编号=%s, 经度=%s, 纬度=%s, 高度=%s\n"), 
					i + 1, strIndex, strLongitude, strLatitude, strAltitude);
			}
		}
		else
		{
			TRACE(_T("错误：插入航路点 %d 失败\n"), i + 1);
		}
	}
	
	TRACE(_T("已显示 %d 个航路点到列表控件\n"), nCount);
	
	// 保存当前显示的航路点数据（用于编辑）
	memcpy(m_currentWaypoints, waypoints, sizeof(Waypoint) * nCount);
	m_nCurrentWaypointCount = nCount;
}

// 从列表控件获取航路点数据（用于编辑后保存）
void CPage2Dlg::GetWaypointsFromList(Waypoint waypoints[100], int& nCount)
{
	nCount = m_nCurrentWaypointCount;
	if (nCount > 100) nCount = 100;
	memcpy(waypoints, m_currentWaypoints, sizeof(Waypoint) * nCount);
}

// 双击列表控件单元格，开始编辑
void CPage2Dlg::OnNMDblclkListWaypoints(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	// 如果点击的不是有效行，返回
	if (pNMItemActivate->iItem < 0)
	{
		*pResult = 0;
		return;
	}
	
	// 如果点击的是编号列（第0列），不允许编辑
	if (pNMItemActivate->iSubItem == 0)
	{
		*pResult = 0;
		return;
	}
	
	// 开始编辑单元格
	StartEditCell(pNMItemActivate->iItem, pNMItemActivate->iSubItem);
	
	*pResult = 0;
}

// 单击列表控件，结束编辑
void CPage2Dlg::OnNMClickListWaypoints(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	// 如果当前正在编辑，结束编辑
	if (m_nEditingItem >= 0)
	{
		EndEditCell(FALSE);  // 保存编辑
	}
	
	*pResult = 0;
}

// 开始编辑单元格
void CPage2Dlg::StartEditCell(int nItem, int nSubItem)
{
	// 如果正在编辑其他单元格，先结束编辑
	if (m_nEditingItem >= 0 && (m_nEditingItem != nItem || m_nEditingSubItem != nSubItem))
	{
		EndEditCell(FALSE);
	}
	
	// 获取单元格文本
	CString strText = m_listWaypoints.GetItemText(nItem, nSubItem);
	
	// 获取单元格位置
	CRect rect;
	m_listWaypoints.GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rect);
	
	// 创建或显示编辑控件
	if (m_editInline.GetSafeHwnd() == NULL)
	{
		m_editInline.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
			rect, &m_listWaypoints, 1001);
		m_editInline.SetFont(m_listWaypoints.GetFont());
	}
	
	m_editInline.SetWindowText(strText);
	m_editInline.SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(),
		SWP_SHOWWINDOW | SWP_NOZORDER);
	m_editInline.SetSel(0, -1);  // 全选文本
	m_editInline.SetFocus();
	
	m_nEditingItem = nItem;
	m_nEditingSubItem = nSubItem;
}

// 结束编辑单元格
void CPage2Dlg::EndEditCell(BOOL bCancel)
{
	if (m_nEditingItem < 0 || m_editInline.GetSafeHwnd() == NULL)
		return;
	
	if (!bCancel)
	{
		// 获取编辑后的文本
		CString strNewText;
		m_editInline.GetWindowText(strNewText);
		
		// 更新列表控件显示
		m_listWaypoints.SetItemText(m_nEditingItem, m_nEditingSubItem, strNewText);
		
		// 更新内存中的航路点数据
		if (m_nEditingItem < m_nCurrentWaypointCount)
		{
			double dValue = _tstof(strNewText);
			
			switch (m_nEditingSubItem)
			{
			case 1:  // 经度
				m_currentWaypoints[m_nEditingItem].longitude = static_cast<int32_t>(dValue * 1000000.0);
				TRACE(_T("航路点 %d 经度更新为: %.6f度 (%d)\n"), 
					m_nEditingItem + 1, dValue, m_currentWaypoints[m_nEditingItem].longitude);
				break;
			case 2:  // 纬度
				m_currentWaypoints[m_nEditingItem].latitude = static_cast<int32_t>(dValue * 1000000.0);
				TRACE(_T("航路点 %d 纬度更新为: %.6f度 (%d)\n"), 
					m_nEditingItem + 1, dValue, m_currentWaypoints[m_nEditingItem].latitude);
				break;
			case 3:  // 高度
				m_currentWaypoints[m_nEditingItem].altitude = static_cast<int16_t>(dValue);
				TRACE(_T("航路点 %d 高度更新为: %.2f米 (%d)\n"), 
					m_nEditingItem + 1, dValue, m_currentWaypoints[m_nEditingItem].altitude);
				break;
			}
		}
	}
	
	// 隐藏编辑控件
	m_editInline.ShowWindow(SW_HIDE);
	m_nEditingItem = -1;
	m_nEditingSubItem = -1;
}

// 编辑控件失去焦点时，结束编辑
void CPage2Dlg::OnEnKillfocusEditInline()
{
	EndEditCell(FALSE);  // 保存编辑
}

// 处理命令消息（用于处理编辑控件的回车键）
BOOL CPage2Dlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// 如果编辑控件存在且正在编辑，处理回车键
	if (m_editInline.GetSafeHwnd() != NULL && m_nEditingItem >= 0)
	{
		if (LOWORD(wParam) == 1001 && HIWORD(wParam) == EN_KILLFOCUS)
		{
			EndEditCell(FALSE);
			return TRUE;
		}
	}
	
	return CDialogEx::OnCommand(wParam, lParam);
}