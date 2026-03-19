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
#include "UAV_DataLink.h"
#include "DemReader.h"

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
	, m_nNextWaypointIndexForMapPick(0)
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
	ON_BN_CLICKED(IDC_BUTTON_LoadData2GUI, &CPage2Dlg::OnBnClickedButtonLoadData2GUI)
	ON_BN_CLICKED(IDC_BUTTON_SaveData2xml, &CPage2Dlg::OnBnClickedButtonSaveData2xml)
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

	// 初始化发送数据输入控件（根据数据类型设置默认值）
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
		// 从 waypoints.xml 的 sendData 节点加载装订参数到 EditData2~28（不显示在航路点列表中）
		LoadSendDataFromXml();
	}
	else
	{
		TRACE(_T("警告：未找到IDC_LIST_Waypoints控件\n"));
	}
	
	return TRUE;
}

// 重置航路点装订计数器（重新从1号点开始）
void CPage2Dlg::ResetWaypointPickFromMap()
{
	m_nNextWaypointIndexForMapPick = 0;
}

// 清空航路点列表（经度纬度高度全0）并重置选点计数器
void CPage2Dlg::ClearAllWaypointsAndResetPick()
{
	if (m_listWaypoints.GetSafeHwnd() == NULL)
		return;
	if (m_nEditingItem >= 0)
		EndEditCell(FALSE);
	m_listWaypoints.DeleteAllItems();
	memset(m_currentWaypoints, 0, sizeof(m_currentWaypoints));
	m_nCurrentWaypointCount = 0;
	m_nNextWaypointIndexForMapPick = 0;
	TRACE(_T("ClearAllWaypointsAndResetPick: 航路点列表已清空，选点从1号开始\n"));
}

// 从地图选点结果写入第 n 个航路点（第1次点击->1号点，第2次->2号点，以此类推）
bool CPage2Dlg::ApplyWaypointFromMap(double lat, double lng)
{
	if (m_listWaypoints.GetSafeHwnd() == NULL)
		return false;

	// 超过100个则忽略后续点击
	if (m_nNextWaypointIndexForMapPick >= 100)
	{
		TRACE(_T("ApplyWaypointFromMap: 已达到100个航路点，忽略后续地图选点\n"));
		return true;
	}

	int idx = m_nNextWaypointIndexForMapPick; // 0-based

	// 如有正在编辑的单元格，先结束编辑，避免覆盖未保存内容
	if (m_nEditingItem >= 0)
	{
		EndEditCell(FALSE);
	}

	// 如果当前航路点数量不足，扩展列表并补齐中间空位
	if (idx >= m_nCurrentWaypointCount)
	{
		int oldCount = m_nCurrentWaypointCount;
		m_nCurrentWaypointCount = idx + 1;
		if (m_nCurrentWaypointCount > 100)
			m_nCurrentWaypointCount = 100;

		for (int i = oldCount; i <= idx && i < 100; ++i)
		{
			CString strIndex, strLon, strLat, strAlt;
			strIndex.Format(_T("%d"), i + 1);
			strLon = _T("0.000000");
			strLat = _T("0.000000");
			strAlt = _T("0");

			int nItem = m_listWaypoints.InsertItem(i, strIndex);
			if (nItem >= 0)
			{
				m_listWaypoints.SetItemText(nItem, 1, strLon);
				m_listWaypoints.SetItemText(nItem, 2, strLat);
				m_listWaypoints.SetItemText(nItem, 3, strAlt);
			}

			m_currentWaypoints[i].longitude = 0;
			m_currentWaypoints[i].latitude = 0;
			m_currentWaypoints[i].altitude = 0;
		}
	}

	// 将当前点击经纬度写入第 idx 个航路点；高度从 DEM 读取，若无则保持原值或 0
	if (idx < 0 || idx >= 100)
		return (m_nNextWaypointIndexForMapPick >= 100);

	double dLongitude = lng;
	double dLatitude = lat;

	// 从 DEM 读取该点高程（米），协议中航路点高度为 int16_t 米×10
	double elevM = 0.0;
	int16_t altX10 = 0;
	if (DemReaderGetElevation(dLongitude, dLatitude, &elevM))
		altX10 = static_cast<int16_t>(elevM * 10.0 + (elevM >= 0 ? 0.5 : -0.5));

	CString strIndex, strLongitude, strLatitude, strAltitude;
	strIndex.Format(_T("%d"), idx + 1);
	strLongitude.Format(_T("%.6f"), dLongitude);
	strLatitude.Format(_T("%.6f"), dLatitude);
	strAltitude.Format(_T("%.1f"), altX10 / 10.0);

	// 更新列表控件显示
	if (m_listWaypoints.GetItemCount() <= idx)
	{
		int nItem = m_listWaypoints.InsertItem(idx, strIndex);
		if (nItem >= 0)
		{
			m_listWaypoints.SetItemText(nItem, 1, strLongitude);
			m_listWaypoints.SetItemText(nItem, 2, strLatitude);
			m_listWaypoints.SetItemText(nItem, 3, strAltitude);
		}
	}
	else
	{
		m_listWaypoints.SetItemText(idx, 0, strIndex);
		m_listWaypoints.SetItemText(idx, 1, strLongitude);
		m_listWaypoints.SetItemText(idx, 2, strLatitude);
		m_listWaypoints.SetItemText(idx, 3, strAltitude);
	}

	// 更新内存中的航路点数据（协议格式：度×1e7，高度米×10）
	m_currentWaypoints[idx].longitude = static_cast<int32_t>(dLongitude * 10000000.0);
	m_currentWaypoints[idx].latitude = static_cast<int32_t>(dLatitude * 10000000.0);
	m_currentWaypoints[idx].altitude = altX10;

	TRACE(_T("ApplyWaypointFromMap: 航路点 %d 设为 lon=%.6f, lat=%.6f, alt=%d(×10)\n"),
		idx + 1, dLongitude, dLatitude, (int)altX10);

	// 准备下一次点击 -> 下一个航路点
	++m_nNextWaypointIndexForMapPick;

	return (m_nNextWaypointIndexForMapPick >= 100);
}

// 更新数据显示（Page2当前不显示接收数据，保留接口以兼容主对话框调用）
void CPage2Dlg::ClearAllEditDataToZero()
{
	if (m_editSendData2.GetSafeHwnd() != NULL)  m_editSendData2.SetWindowText(_T("0"));
	if (m_editSendData3.GetSafeHwnd() != NULL)  m_editSendData3.SetWindowText(_T("0"));
	if (m_editSendData4.GetSafeHwnd() != NULL)  m_editSendData4.SetWindowText(_T("0"));
	if (m_editSendData5.GetSafeHwnd() != NULL)  m_editSendData5.SetWindowText(_T("0"));
	if (m_editSendData6.GetSafeHwnd() != NULL)  m_editSendData6.SetWindowText(_T("0"));
	if (m_editSendData7.GetSafeHwnd() != NULL)  m_editSendData7.SetWindowText(_T("0"));
	if (m_editSendData8.GetSafeHwnd() != NULL)  m_editSendData8.SetWindowText(_T("0"));
	if (m_editSendData9.GetSafeHwnd() != NULL)  m_editSendData9.SetWindowText(_T("0"));
	if (m_editSendData10.GetSafeHwnd() != NULL) m_editSendData10.SetWindowText(_T("0"));
	if (m_editSendData11.GetSafeHwnd() != NULL) m_editSendData11.SetWindowText(_T("0"));
	if (m_editSendData12.GetSafeHwnd() != NULL) m_editSendData12.SetWindowText(_T("0"));
	if (m_editSendData13.GetSafeHwnd() != NULL) m_editSendData13.SetWindowText(_T("0"));
	if (m_editSendData14.GetSafeHwnd() != NULL) m_editSendData14.SetWindowText(_T("0"));
	if (m_editSendData15.GetSafeHwnd() != NULL) m_editSendData15.SetWindowText(_T("0"));
	if (m_editSendData16.GetSafeHwnd() != NULL) m_editSendData16.SetWindowText(_T("0"));
	if (m_editSendData17.GetSafeHwnd() != NULL) m_editSendData17.SetWindowText(_T("0"));
	if (m_editSendData18.GetSafeHwnd() != NULL) m_editSendData18.SetWindowText(_T("0"));
	if (m_editSendData19.GetSafeHwnd() != NULL) m_editSendData19.SetWindowText(_T("0"));
	if (m_editSendData20.GetSafeHwnd() != NULL) m_editSendData20.SetWindowText(_T("0"));
	if (m_editSendData21.GetSafeHwnd() != NULL) m_editSendData21.SetWindowText(_T("0"));
	if (m_editSendData22.GetSafeHwnd() != NULL) m_editSendData22.SetWindowText(_T("0"));
	if (m_editSendData23.GetSafeHwnd() != NULL) m_editSendData23.SetWindowText(_T("0"));
	if (m_editSendData24.GetSafeHwnd() != NULL) m_editSendData24.SetWindowText(_T("0"));
	if (m_editSendData25.GetSafeHwnd() != NULL) m_editSendData25.SetWindowText(_T("0"));
	if (m_editSendData26.GetSafeHwnd() != NULL) m_editSendData26.SetWindowText(_T("0"));
	if (m_editSendData27.GetSafeHwnd() != NULL) m_editSendData27.SetWindowText(_T("0"));
	if (m_editSendData28.GetSafeHwnd() != NULL) m_editSendData28.SetWindowText(_T("0"));
}

void CPage2Dlg::UpdateDisplay(const DataLinkRecvDataPacket_s* pPacket)
{
	// Page2对话框当前不显示接收数据，此函数保留为空实现
	// 如果将来需要在Page2显示接收数据，可以在此添加相应控件和更新逻辑
	(void)pPacket;  // 避免未使用参数警告
}

// 辅助：判断经纬高三个值是否均为 0（空或 "0" 视为 0）
static bool IsGroupAllZero(double lon, double lat, double alt)
{
	return (lon == 0.0 && lat == 0.0 && alt == 0.0);
}

// 将 PAGE2 编辑框/航路点列表数据按当前绘图逻辑加载到地图；经纬高全 0 的组跳过，已有绘制的也跳过
void CPage2Dlg::LoadPage2DataToMap()
{
	if (!m_pMainDlg) return;

	CString strData[26];
	if (m_editSendData2.GetSafeHwnd())  m_editSendData2.GetWindowText(strData[2]);
	if (m_editSendData3.GetSafeHwnd())  m_editSendData3.GetWindowText(strData[3]);
	if (m_editSendData4.GetSafeHwnd())  m_editSendData4.GetWindowText(strData[4]);
	if (m_editSendData17.GetSafeHwnd()) m_editSendData17.GetWindowText(strData[17]);
	if (m_editSendData18.GetSafeHwnd()) m_editSendData18.GetWindowText(strData[18]);
	if (m_editSendData19.GetSafeHwnd()) m_editSendData19.GetWindowText(strData[19]);
	if (m_editSendData23.GetSafeHwnd()) m_editSendData23.GetWindowText(strData[23]);
	if (m_editSendData24.GetSafeHwnd()) m_editSendData24.GetWindowText(strData[24]);
	if (m_editSendData25.GetSafeHwnd()) m_editSendData25.GetWindowText(strData[25]);

	double launchLon = _ttof(strData[2]),  launchLat = _ttof(strData[3]),  launchAlt = _ttof(strData[4]);
	double targetLon = _ttof(strData[17]), targetLat = _ttof(strData[18]), targetAlt = _ttof(strData[19]);
	double paraLon   = _ttof(strData[23]), paraLat   = _ttof(strData[24]), paraAlt   = _ttof(strData[25]);

	CStringA jsonA;
	jsonA = "{\"command\":\"loadPage2PointsToMap\"";

	if (!IsGroupAllZero(launchLon, launchLat, launchAlt))
	{
		CStringA part;
		part.Format(",\"launch\":{\"lat\":%.6f,\"lng\":%.6f}", launchLat, launchLon);
		jsonA += part;
	}
	if (!IsGroupAllZero(targetLon, targetLat, targetAlt))
	{
		CStringA part;
		part.Format(",\"target\":{\"lat\":%.6f,\"lng\":%.6f}", targetLat, targetLon);
		jsonA += part;
	}
	if (!IsGroupAllZero(paraLon, paraLat, paraAlt))
	{
		CStringA part;
		part.Format(",\"parachute\":{\"lat\":%.6f,\"lng\":%.6f}", paraLat, paraLon);
		jsonA += part;
	}

	// 航路点：从 m_currentWaypoints 取，经纬高全 0 的跳过
	CStringA waypointsJson;
	for (int i = 0; i < m_nCurrentWaypointCount && i < 100; i++)
	{
		const Waypoint& w = m_currentWaypoints[i];
		double lon = w.longitude / 10000000.0;
		double lat = w.latitude / 10000000.0;
		double alt = static_cast<double>(w.altitude);
		if (IsGroupAllZero(lon, lat, alt)) continue;
		if (!waypointsJson.IsEmpty()) waypointsJson += ",";
		CStringA item;
		item.Format("{\"lat\":%.6f,\"lng\":%.6f}", lat, lon);
		waypointsJson += item;
	}
	if (!waypointsJson.IsEmpty())
		jsonA += ",\"waypoints\":[" + waypointsJson + "]";

	jsonA += "}";
	m_pMainDlg->PostMapMessageFromPage2(jsonA);
}

// 加载当前数据至主GUI：将本页数据加载到地图绘图
void CPage2Dlg::OnBnClickedButtonLoadData2GUI()
{
	// 若列表正在内联编辑，先结束编辑并写回数据，避免直接点加载时未提交导致绘制错误
	if (m_nEditingItem >= 0)
		EndEditCell(FALSE);
	LoadPage2DataToMap();
}

// 保存当前数据至 xml 文件：将本页编辑框与航路点列表写入可执行文件目录下的 waypoints.xml
void CPage2Dlg::OnBnClickedButtonSaveData2xml()
{
	if (m_nEditingItem >= 0)
		EndEditCell(FALSE);
	if (SavePage2DataToXml())
		MessageBox(_T("已保存到 waypoints.xml"), _T("保存成功"), MB_OK | MB_ICONINFORMATION);
	else
		MessageBox(_T("保存失败，请检查文件路径或权限。"), _T("保存失败"), MB_OK | MB_ICONWARNING);
}

//UDP发送装订参数数据
void CPage2Dlg::OnBnClickedButtonSendData()
{
	TRACE(_T("OnBnClickedButtonSendData: 发送装订参数数据\n"));
	
	// 获取所有控件数据
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
	DataSendPacket_s packet{};
	memset(&packet, 0, sizeof(packet));  // 清零，包括waypoints数组
	packet.frameHeader = 0xF00F;  // 设置帧头（装订参数包）
	
	// 装订数据类型转换
	packet.launchLongitude = static_cast<int32_t>(_ttof(strData[2]) * 10000000.0); //发射点经纬度
	packet.launchLatitude = static_cast<int32_t>(_ttof(strData[3]) * 10000000.0);
	packet.launchAltitude = static_cast<int16_t>(_ttoi(strData[4]) * 10.0);
	packet.initPitch = static_cast<int16_t>(_ttoi(strData[5]) * 10.0);
	packet.initYaw = static_cast<int16_t>(_ttoi(strData[6]) * 10.0);
	packet.initRoll = static_cast<int16_t>(_ttoi(strData[7]) * 10.0);
	packet.initPitchRate = static_cast<int16_t>(_ttoi(strData[8]) * 10.0);
	packet.initYawRate = static_cast<int16_t>(_ttoi(strData[9]) * 10.0);
	packet.initRollRate = static_cast<int16_t>(_ttoi(strData[10]) * 10.0);
	packet.initNorthVelocity = static_cast<int16_t>(_ttoi(strData[11]) * 10.0);
	packet.initEastVelocity = static_cast<int16_t>(_ttoi(strData[12]) * 10.0);
	packet.initVerticalVelocity = static_cast<int16_t>(_ttoi(strData[13]) * 10.0);
	packet.initNorthAccel = static_cast<int16_t>(_ttoi(strData[14]) * 10.0);
	packet.initEastAccel = static_cast<int16_t>(_ttoi(strData[15]) * 10.0);
	packet.initVerticalAccel = static_cast<int16_t>(_ttoi(strData[16]) * 10.0);
	
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
	
	// 目标点经纬度
	packet.targetLongitude = static_cast<int32_t>(_ttof(strData[17]) * 10000000.0);
	packet.targetLatitude = static_cast<int32_t>(_ttof(strData[18]) * 10000000.0);
	packet.targetAltitude = static_cast<int16_t>(_ttoi(strData[19]) * 10.0);           // int16_t
	// UAV_DataLink.h 的 DataSendPacket_s 不包含 launchLongitude2/launchLatitude2/launchAltitude2（弃用）
	// 开伞点经纬度
	packet.parachuteLongitude = static_cast<int32_t>(_ttof(strData[23]) * 10000000.0);
	packet.parachuteLatitude = static_cast<int32_t>(_ttof(strData[24]) * 10000000.0);
	packet.parachuteAltitude = static_cast<int16_t>(_ttoi(strData[25]) * 10.0);        // int16_t
	// UAV_DataLink.h 的 DataSendPacket_s 不包含 elevatorCmd/aileronCmd（弃用）
	packet.airspeedSet = static_cast<uint8_t>(_ttoi(strData[28]));              // uint8_t

	// 计算整个结构体的校验和
	packet.checksum = 0;
	size_t checksumSize = sizeof(packet) - sizeof(packet.checksum);
	packet.checksum = calculateChecksum(&packet, checksumSize);

	// 调试输出：检查发送的数据和结构体大小
	TRACE(_T("UDP发送装订参数: frameHeader=0x%04X, launchLon=%d, launchLat=%d, launchAlt=%d\n"),
		packet.frameHeader, packet.launchLongitude, packet.launchLatitude, packet.launchAltitude);
	TRACE(_T("UDP发送: 结构体大小=%d字节, 校验和=0x%02X\n"), sizeof(DataSendPacket_s), packet.checksum);
	
	// 调试输出：显示原始字节（用于诊断，只显示前32字节）
	BYTE* pBytes = (BYTE*)&packet;
	TRACE(_T("UDP发送原始字节[前32字节]: "));
	int nBytesToShow = (sizeof(DataSendPacket_s) < 32) ? sizeof(DataSendPacket_s) : 32;
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
		return;
	}

	TRACE(_T("OnBnClickedButtonSendData: 开始调用SendUdpDataPublic（连续发送3次以应对丢包）\n"));
	
	// ============================================================
	// 丢包保护措施：连续发送3次相同的数据包
	// 接收端通过判断连续两个包数据相同时，才采纳数据
	// ============================================================
	const int nSendCount = 3;  // 连续发送次数
	const int nSendIntervalMs = 10;  // 每次发送之间的间隔（毫秒），避免网络拥塞
	int nSuccessCount = 0;
	int nFailCount = 0;
	
	for (int i = 0; i < nSendCount; i++)
	{
		BOOL bResult = m_pMainDlg->SendUdpDataPublic(&packet, sizeof(packet));		// 发送数据（非阻塞）
		
		if (bResult)
		{
			nSuccessCount++;
			TRACE(_T("UDP发送第 %d/%d 次：成功\n"), i + 1, nSendCount);
		}
		else
		{
			nFailCount++;
			TRACE(_T("UDP发送第 %d/%d 次：失败\n"), i + 1, nSendCount);
		}
		
		// 如果不是最后一次发送，等待一段时间再发送下一次
		if (i < nSendCount - 1)
		{
			Sleep(nSendIntervalMs);
		}
	}
	
	TRACE(_T("UDP发送完成：成功 %d 次，失败 %d 次（共 %d 次）\n"), nSuccessCount, nFailCount, nSendCount);
	
	// 如果所有发送都失败，显示错误消息
	if (nSuccessCount == 0)
	{
		TRACE(_T("错误：UDP未连接或所有发送都失败，请检查连接后重试。\n"));
		MessageBox(_T("UDP未连接或所有发送都失败，请检查连接后重试。"), _T("发送失败"), MB_OK | MB_ICONERROR | MB_TOPMOST);
		return;
	}
	
	// 至少有一次发送成功
	if (nFailCount > 0)
	{
		// 部分成功，显示警告
		CString strMsg;
		strMsg.Format(_T("UDP数据发送完成（成功 %d 次，失败 %d 次）。\n\n建议：如果频繁失败，请检查网络连接。"), 
			nSuccessCount, nFailCount);
		MessageBox(strMsg, _T("发送部分成功"), MB_OK | MB_ICONWARNING | MB_TOPMOST);
	}
	else
	{
		// 全部成功
		TRACE(_T("UDP数据发送成功（全部 %d 次都成功）。\n"), nSuccessCount);
		MessageBox(_T("UDP数据发送成功\n（已发送3包次，间隔10ms）。"), _T("发送成功"), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
	}
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
					// 读取浮点数并转换为int32_t（度 * 10000000）
					double dLongitude = _tstof(CString(bstrText));
					waypoints[i].longitude = static_cast<int32_t>(dLongitude * 10000000.0);
					// TRACE(_T("航路点 %d: longitude=%.6f度 -> %d\n"), i + 1, dLongitude, waypoints[i].longitude);
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
					// 读取浮点数并转换为int32_t（度 * 10000000）
					double dLatitude = _tstof(CString(bstrText));
					waypoints[i].latitude = static_cast<int32_t>(dLatitude * 10000000.0);
					// TRACE(_T("航路点 %d: latitude=%.6f度 -> %d\n"), i + 1, dLatitude, waypoints[i].latitude);
				}
			}
			
			// 获取altitude子节点（XML中为米，协议存储为米×10）
			CComPtr<IXMLDOMNode> spAltitudeNode;
			hr = spNode->selectSingleNode(CComBSTR(_T("altitude")), &spAltitudeNode);
			if (SUCCEEDED(hr) && spAltitudeNode != NULL)
			{
				CComBSTR bstrText;
				spAltitudeNode->get_text(&bstrText);
				if (bstrText.Length() > 0)
				{
					double dAltitude = _tstof(CString(bstrText));
					waypoints[i].altitude = static_cast<int16_t>(dAltitude * 10.0 + (dAltitude >= 0 ? 0.5 : -0.5));
					// TRACE(_T("航路点 %d: altitude=%.2f米 -> %d\n"), i + 1, dAltitude, waypoints[i].altitude);
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

// 从 waypoints.xml 的 sendData 节点加载装订参数到 EditData2~28，不涉及航路点列表
BOOL CPage2Dlg::LoadSendDataFromXml()
{
	TCHAR szModulePath[MAX_PATH];
	GetModuleFileName(NULL, szModulePath, MAX_PATH);
	CString strExePath = szModulePath;
	int nLastSlash = strExePath.ReverseFind(_T('\\'));
	if (nLastSlash >= 0)
		strExePath = strExePath.Left(nLastSlash + 1);
	CString strFilePath = strExePath + _T("waypoints.xml");

	CFileStatus status;
	if (!CFile::GetStatus(strFilePath, status))
		return TRUE;  // 文件不存在时视为成功，仅不加载 sendData

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	BOOL bNeedUninit = SUCCEEDED(hr);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
		return FALSE;

	BOOL bOk = FALSE;
	{
		CComPtr<IXMLDOMDocument> spXMLDoc;
		hr = spXMLDoc.CoCreateInstance(__uuidof(DOMDocument60));
		if (FAILED(hr))
		{
			if (bNeedUninit) CoUninitialize();
			return FALSE;
		}
		spXMLDoc->put_async(VARIANT_FALSE);
		CComVariant varFileName(strFilePath);
		VARIANT_BOOL vbSuccess;
		hr = spXMLDoc->load(varFileName, &vbSuccess);
		if (FAILED(hr) || vbSuccess != VARIANT_TRUE)
		{
			if (bNeedUninit) CoUninitialize();
			return TRUE;  // 文件加载失败时不影响程序，仅不加载 sendData
		}

		CComPtr<IXMLDOMElement> spRoot;
		hr = spXMLDoc->get_documentElement(&spRoot);
		if (FAILED(hr) || spRoot == NULL)
		{
			if (bNeedUninit) CoUninitialize();
			return TRUE;
		}

		CComPtr<IXMLDOMNode> spSendDataNode;
		hr = spRoot->selectSingleNode(CComBSTR(_T("sendData")), &spSendDataNode);
		if (FAILED(hr) || spSendDataNode == NULL)
		{
			TRACE(_T("waypoints.xml 中无 sendData 节点，跳过装订参数加载\n"));
			bOk = TRUE;
			if (bNeedUninit) CoUninitialize();
			return bOk;
		}

		CComPtr<IXMLDOMNodeList> spChildList;
		hr = spSendDataNode->get_childNodes(&spChildList);
		if (FAILED(hr) || spChildList == NULL)
		{
			bOk = TRUE;
			if (bNeedUninit) CoUninitialize();
			return bOk;
		}

		long nLen = 0;
		spChildList->get_length(&nLen);
		for (long i = 0; i < nLen; i++)
		{
			CComPtr<IXMLDOMNode> spNode;
			hr = spChildList->get_item(i, &spNode);
			if (FAILED(hr) || spNode == NULL) continue;
			DOMNodeType nodeType;
			spNode->get_nodeType(&nodeType);
			if (nodeType != NODE_ELEMENT) continue;

			CComBSTR bstrName;
			spNode->get_nodeName(&bstrName);
			CComBSTR bstrText;
			spNode->get_text(&bstrText);
			CString strName(bstrName);
			CString strValue(bstrText);
			strName.Trim(); strValue.Trim();

			CEdit* pEdit = NULL;
			if (strName == _T("launchLongitude"))       pEdit = &m_editSendData2;
			else if (strName == _T("launchLatitude"))   pEdit = &m_editSendData3;
			else if (strName == _T("launchAltitude"))   pEdit = &m_editSendData4;
			else if (strName == _T("initPitch"))        pEdit = &m_editSendData5;
			else if (strName == _T("initYaw"))          pEdit = &m_editSendData6;
			else if (strName == _T("initRoll"))        pEdit = &m_editSendData7;
			else if (strName == _T("initPitchRate"))    pEdit = &m_editSendData8;
			else if (strName == _T("initYawRate"))      pEdit = &m_editSendData9;
			else if (strName == _T("initRollRate"))     pEdit = &m_editSendData10;
			else if (strName == _T("initNorthVelocity")) pEdit = &m_editSendData11;
			else if (strName == _T("initEastVelocity"))  pEdit = &m_editSendData12;
			else if (strName == _T("initVerticalVelocity")) pEdit = &m_editSendData13;
			else if (strName == _T("initNorthAccel"))   pEdit = &m_editSendData14;
			else if (strName == _T("initEastAccel"))    pEdit = &m_editSendData15;
			else if (strName == _T("initVerticalAccel")) pEdit = &m_editSendData16;
			else if (strName == _T("targetLongitude"))   pEdit = &m_editSendData17;
			else if (strName == _T("targetLatitude"))   pEdit = &m_editSendData18;
			else if (strName == _T("targetAltitude"))   pEdit = &m_editSendData19;
			else if (strName == _T("launchLongitude2"))  pEdit = &m_editSendData20;
			else if (strName == _T("launchLatitude2"))   pEdit = &m_editSendData21;
			else if (strName == _T("launchAltitude2"))   pEdit = &m_editSendData22;
			else if (strName == _T("parachuteLongitude")) pEdit = &m_editSendData23;
			else if (strName == _T("parachuteLatitude"))  pEdit = &m_editSendData24;
			else if (strName == _T("parachuteAltitude"))  pEdit = &m_editSendData25;
			else if (strName == _T("elevatorCmd"))       pEdit = &m_editSendData26;
			else if (strName == _T("aileronCmd"))        pEdit = &m_editSendData27;
			else if (strName == _T("airspeedSet"))       pEdit = &m_editSendData28;
			if (pEdit != NULL && pEdit->GetSafeHwnd() != NULL)
				pEdit->SetWindowText(strValue);
		}
		bOk = TRUE;
	}
	if (bNeedUninit) CoUninitialize();
	TRACE(_T("LoadSendDataFromXml: 装订参数已从 waypoints.xml 的 sendData 加载到界面\n"));
	return bOk;
}

// 将本页编辑框与航路点列表保存到可执行文件目录下的 waypoints.xml（格式与 Load 一致）
BOOL CPage2Dlg::SavePage2DataToXml()
{
	TCHAR szModulePath[MAX_PATH];
	GetModuleFileName(NULL, szModulePath, MAX_PATH);
	CString strExePath = szModulePath;
	int nLastSlash = strExePath.ReverseFind(_T('\\'));
	if (nLastSlash >= 0)
		strExePath = strExePath.Left(nLastSlash + 1);
	CString strFilePath = strExePath + _T("waypoints.xml");

	CString strData[29];
	if (m_editSendData2.GetSafeHwnd())  m_editSendData2.GetWindowText(strData[2]);
	if (m_editSendData3.GetSafeHwnd())  m_editSendData3.GetWindowText(strData[3]);
	if (m_editSendData4.GetSafeHwnd())  m_editSendData4.GetWindowText(strData[4]);
	if (m_editSendData5.GetSafeHwnd())  m_editSendData5.GetWindowText(strData[5]);
	if (m_editSendData6.GetSafeHwnd())  m_editSendData6.GetWindowText(strData[6]);
	if (m_editSendData7.GetSafeHwnd())  m_editSendData7.GetWindowText(strData[7]);
	if (m_editSendData8.GetSafeHwnd())  m_editSendData8.GetWindowText(strData[8]);
	if (m_editSendData9.GetSafeHwnd())  m_editSendData9.GetWindowText(strData[9]);
	if (m_editSendData10.GetSafeHwnd()) m_editSendData10.GetWindowText(strData[10]);
	if (m_editSendData11.GetSafeHwnd()) m_editSendData11.GetWindowText(strData[11]);
	if (m_editSendData12.GetSafeHwnd()) m_editSendData12.GetWindowText(strData[12]);
	if (m_editSendData13.GetSafeHwnd()) m_editSendData13.GetWindowText(strData[13]);
	if (m_editSendData14.GetSafeHwnd()) m_editSendData14.GetWindowText(strData[14]);
	if (m_editSendData15.GetSafeHwnd()) m_editSendData15.GetWindowText(strData[15]);
	if (m_editSendData16.GetSafeHwnd()) m_editSendData16.GetWindowText(strData[16]);
	if (m_editSendData17.GetSafeHwnd()) m_editSendData17.GetWindowText(strData[17]);
	if (m_editSendData18.GetSafeHwnd()) m_editSendData18.GetWindowText(strData[18]);
	if (m_editSendData19.GetSafeHwnd()) m_editSendData19.GetWindowText(strData[19]);
	if (m_editSendData20.GetSafeHwnd()) m_editSendData20.GetWindowText(strData[20]);
	if (m_editSendData21.GetSafeHwnd()) m_editSendData21.GetWindowText(strData[21]);
	if (m_editSendData22.GetSafeHwnd()) m_editSendData22.GetWindowText(strData[22]);
	if (m_editSendData23.GetSafeHwnd()) m_editSendData23.GetWindowText(strData[23]);
	if (m_editSendData24.GetSafeHwnd()) m_editSendData24.GetWindowText(strData[24]);
	if (m_editSendData25.GetSafeHwnd()) m_editSendData25.GetWindowText(strData[25]);
	if (m_editSendData26.GetSafeHwnd()) m_editSendData26.GetWindowText(strData[26]);
	if (m_editSendData27.GetSafeHwnd()) m_editSendData27.GetWindowText(strData[27]);
	if (m_editSendData28.GetSafeHwnd()) m_editSendData28.GetWindowText(strData[28]);

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	BOOL bNeedUninit = SUCCEEDED(hr);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
		return FALSE;

	BOOL bOk = FALSE;
	{
		CComPtr<IXMLDOMDocument> spDoc;
		hr = spDoc.CoCreateInstance(__uuidof(DOMDocument60));
		if (FAILED(hr))
		{
			if (bNeedUninit) CoUninitialize();
			return FALSE;
		}
		spDoc->put_async(VARIANT_FALSE);

		CComPtr<IXMLDOMElement> spRoot;
		hr = spDoc->createElement(CComBSTR(L"waypoints"), &spRoot);
		if (FAILED(hr) || spRoot == NULL)
		{
			if (bNeedUninit) CoUninitialize();
			return FALSE;
		}
		CComPtr<IXMLDOMNode> spRootNode;
		hr = spDoc->appendChild(spRoot, &spRootNode);
		if (FAILED(hr))
		{
			if (bNeedUninit) CoUninitialize();
			return FALSE;
		}

		// 航路点节点：与 LoadWaypointsFromXml 格式一致（longitude/latitude 度浮点，altitude 米）
		for (int i = 0; i < m_nCurrentWaypointCount && i < 100; i++)
		{
			CComPtr<IXMLDOMElement> spWp;
			hr = spDoc->createElement(CComBSTR(L"waypoint"), &spWp);
			if (FAILED(hr) || spWp == NULL) continue;
			double lon = m_currentWaypoints[i].longitude / 10000000.0;
			double lat = m_currentWaypoints[i].latitude / 10000000.0;
			CString strLon, strLat, strAlt;
			strLon.Format(_T("%.7f"), lon);
			strLat.Format(_T("%.7f"), lat);
			strAlt.Format(_T("%.1f"), m_currentWaypoints[i].altitude / 10.0);
			CComPtr<IXMLDOMText> spText;
			CComPtr<IXMLDOMElement> spChild;
			hr = spDoc->createElement(CComBSTR(L"longitude"), &spChild);
			if (SUCCEEDED(hr) && spChild)
			{
				hr = spDoc->createTextNode(CComBSTR(strLon), &spText);
				if (SUCCEEDED(hr) && spText) spChild->appendChild(spText, NULL);
				spWp->appendChild(spChild, NULL);
			}
			spChild.Release();
			spText.Release();
			hr = spDoc->createElement(CComBSTR(L"latitude"), &spChild);
			if (SUCCEEDED(hr) && spChild)
			{
				hr = spDoc->createTextNode(CComBSTR(strLat), &spText);
				if (SUCCEEDED(hr) && spText) spChild->appendChild(spText, NULL);
				spWp->appendChild(spChild, NULL);
			}
			spChild.Release();
			spText.Release();
			hr = spDoc->createElement(CComBSTR(L"altitude"), &spChild);
			if (SUCCEEDED(hr) && spChild)
			{
				hr = spDoc->createTextNode(CComBSTR(strAlt), &spText);
				if (SUCCEEDED(hr) && spText) spChild->appendChild(spText, NULL);
				spWp->appendChild(spChild, NULL);
			}
			spRoot->appendChild(spWp, NULL);
		}

		// sendData 节点：与 LoadSendDataFromXml 子节点名一致
		struct _SendDataItem { LPCTSTR name; CString& value; };
		_SendDataItem sendItems[] = {
			{ _T("launchLongitude"),    strData[2] },  { _T("launchLatitude"),    strData[3] },  { _T("launchAltitude"),    strData[4] },
			{ _T("initPitch"),          strData[5] },  { _T("initYaw"),            strData[6] },  { _T("initRoll"),           strData[7] },
			{ _T("initPitchRate"),      strData[8] },  { _T("initYawRate"),        strData[9] },  { _T("initRollRate"),       strData[10] },
			{ _T("initNorthVelocity"),  strData[11] }, { _T("initEastVelocity"),   strData[12] }, { _T("initVerticalVelocity"), strData[13] },
			{ _T("initNorthAccel"),     strData[14] }, { _T("initEastAccel"),      strData[15] }, { _T("initVerticalAccel"),  strData[16] },
			{ _T("targetLongitude"),    strData[17] }, { _T("targetLatitude"),     strData[18] }, { _T("targetAltitude"),     strData[19] },
			{ _T("launchLongitude2"),   strData[20] }, { _T("launchLatitude2"),    strData[21] }, { _T("launchAltitude2"),    strData[22] },
			{ _T("parachuteLongitude"), strData[23] }, { _T("parachuteLatitude"),  strData[24] }, { _T("parachuteAltitude"),  strData[25] },
			{ _T("elevatorCmd"),        strData[26] }, { _T("aileronCmd"),         strData[27] }, { _T("airspeedSet"),        strData[28] }
		};
		CComPtr<IXMLDOMElement> spSendData;
		hr = spDoc->createElement(CComBSTR(L"sendData"), &spSendData);
		if (SUCCEEDED(hr) && spSendData)
		{
			for (int k = 0; k < _countof(sendItems); k++)
			{
				CComPtr<IXMLDOMElement> spEl;
				hr = spDoc->createElement(CComBSTR(sendItems[k].name), &spEl);
				if (SUCCEEDED(hr) && spEl)
				{
					CString& v = sendItems[k].value;
					if (v.IsEmpty()) v = _T("0");
					CComPtr<IXMLDOMText> spText;
					hr = spDoc->createTextNode(CComBSTR(v), &spText);
					if (SUCCEEDED(hr) && spText) spEl->appendChild(spText, NULL);
					spSendData->appendChild(spEl, NULL);
				}
			}
			spRoot->appendChild(spSendData, NULL);
		}

		CComVariant varPath(strFilePath);
		hr = spDoc->save(varPath);
		bOk = SUCCEEDED(hr);
	}
	if (bNeedUninit) CoUninitialize();
	TRACE(_T("SavePage2DataToXml: %s\n"), bOk ? _T("保存成功") : _T("保存失败"));
	return bOk;
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
		// 经度/纬度：从 int32_t（度×1e7）转换为浮点数（度）
		double dLongitude = waypoints[i].longitude / 10000000.0;
		double dLatitude = waypoints[i].latitude / 10000000.0;
		// 高度：协议为米×10，显示为米
		double dAltitudeM = waypoints[i].altitude / 10.0;
		
		// 格式化字符串
		CString strIndex, strLongitude, strLatitude, strAltitude;
		strIndex.Format(_T("%d"), i + 1);
		strLongitude.Format(_T("%.7f"), dLongitude);
		strLatitude.Format(_T("%.7f"), dLatitude);
		strAltitude.Format(_T("%.1f"), dAltitudeM);
		
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
	// 保证后续“继承历史继续选点”时，从当前最后一个航点之后开始装订
	m_nNextWaypointIndexForMapPick = m_nCurrentWaypointCount;
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
				m_currentWaypoints[m_nEditingItem].longitude = static_cast<int32_t>(dValue * 10000000.0);
				TRACE(_T("航路点 %d 经度更新为: %.7f度 (%d)\n"), 
					m_nEditingItem + 1, dValue, m_currentWaypoints[m_nEditingItem].longitude);
				break;
			case 2:  // 纬度
				m_currentWaypoints[m_nEditingItem].latitude = static_cast<int32_t>(dValue * 10000000.0);
				TRACE(_T("航路点 %d 纬度更新为: %.7f度 (%d)\n"), 
					m_nEditingItem + 1, dValue, m_currentWaypoints[m_nEditingItem].latitude);
				break;
			case 3:  // 高度（界面为米，协议存储为米×10）
				m_currentWaypoints[m_nEditingItem].altitude = static_cast<int16_t>(dValue * 10.0 + (dValue >= 0 ? 0.5 : -0.5));
				TRACE(_T("航路点 %d 高度更新为: %.1f米 (%d×10)\n"), 
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

// 计算校验和函数 (非必须，仅用于数据完整性校验)
uint8_t CPage2Dlg::calculateChecksum(const void* data, size_t len) {
	const uint8_t* bytes = (const uint8_t*)data;
	uint8_t sum = 0;
	for (size_t i = 0; i < len; i++) {
		sum += bytes[i];
	}
	return sum;
}