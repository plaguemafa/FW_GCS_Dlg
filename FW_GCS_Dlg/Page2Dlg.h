// Page2Dlg.h: 第二页子对话框头文件
//

#pragma once

#include <afxdialogex.h>
#include "UAV_DataLink.h"

// 前向声明
class CFWGCSDlgDlg;

// CPage2Dlg 对话框
class CPage2Dlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPage2Dlg)

public:
	CPage2Dlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CPage2Dlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PAGE2_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog() override;
	// 发送数据输入控件（对应UdpSendDataPacket协议字段）
	CEdit m_editSendData2;  // IDC_Display_EditData2 - launchLongitude (int32_t)
	CEdit m_editSendData3;  // IDC_Display_EditData3 - launchLatitude (int32_t)
	CEdit m_editSendData4;  // IDC_Display_EditData4 - launchAltitude (int16_t)
	CEdit m_editSendData5;  // IDC_Display_EditData5 - initPitch (int16_t)
	CEdit m_editSendData6;  // IDC_Display_EditData6 - initYaw (int16_t)
	CEdit m_editSendData7;  // IDC_Display_EditData7 - initRoll (int16_t)
	CEdit m_editSendData8;  // IDC_Display_EditData8 - initPitchRate (int16_t)
	CEdit m_editSendData9;  // IDC_Display_EditData9 - initYawRate (int16_t)
	CEdit m_editSendData10; // IDC_Display_EditData10 - initRollRate (int16_t)
	CEdit m_editSendData11; // IDC_Display_EditData11 - initNorthVelocity (int16_t)
	CEdit m_editSendData12; // IDC_Display_EditData12 - initEastVelocity (int16_t)
	CEdit m_editSendData13; // IDC_Display_EditData13 - initVerticalVelocity (int16_t)
	CEdit m_editSendData14; // IDC_Display_EditData14 - initNorthAccel (int16_t)
	CEdit m_editSendData15; // IDC_Display_EditData15 - initEastAccel (int16_t)
	CEdit m_editSendData16; // IDC_Display_EditData16 - initVerticalAccel (int16_t)
	CEdit m_editSendData17; // IDC_Display_EditData17 - targetLongitude (int32_t)
	CEdit m_editSendData18; // IDC_Display_EditData18 - targetLatitude (int32_t)
	CEdit m_editSendData19; // IDC_Display_EditData19 - targetAltitude (int16_t)
	CEdit m_editSendData20; // IDC_Display_EditData20 - launchLongitude2 (int32_t)
	CEdit m_editSendData21; // IDC_Display_EditData21 - launchLatitude2 (int32_t)
	CEdit m_editSendData22; // IDC_Display_EditData22 - launchAltitude2 (int16_t)
	CEdit m_editSendData23; // IDC_Display_EditData23 - parachuteLongitude (int32_t)
	CEdit m_editSendData24; // IDC_Display_EditData24 - parachuteLatitude (int32_t)
	CEdit m_editSendData25; // IDC_Display_EditData25 - parachuteAltitude (int16_t)
	CEdit m_editSendData26; // IDC_Display_EditData26 - elevatorCmd (int8_t)
	CEdit m_editSendData27; // IDC_Display_EditData27 - aileronCmd (int8_t)
	CEdit m_editSendData28; // IDC_Display_EditData28 - airspeedSet (uint8_t)

	// 航路点相关控件
	CButton m_chkLoadWaypoints;  // 复选框：是否加载航路点数据
	CListCtrl m_listWaypoints;  // 列表控件：显示航路点数据

	// 计算校验和函数
	uint8_t calculateChecksum(const void* data, size_t len);
	// 设置主对话框指针（避免每次使用dynamic_cast）
	void SetMainDlg(CFWGCSDlgDlg* pMainDlg) { m_pMainDlg = pMainDlg; }
	
	// 更新数据显示（Page2当前不显示接收数据，保留接口以兼容主对话框调用）
	void UpdateDisplay(const DataLinkRecvDataPacket_s* pPacket);
	// 将所有装订数据编辑框清空为 0（UDP 断开时由主对话框调用）
	void ClearAllEditDataToZero();
	// 发送装订参数数据（保留函数，但不再由按钮触发）
	void OnBnClickedButtonSendData();
	
	// 加载航路点XML文件（从可执行文件目录下的waypoints.xml），仅填充航路点数组，供列表显示
	BOOL LoadWaypointsFromXml(Waypoint waypoints[100], int& nLoadedCount);
	// 从 waypoints.xml 的 sendData 节点加载装订参数到 EditData2~28 控件（不涉及航路点列表）
	BOOL LoadSendDataFromXml();
	// 将本页编辑框与航路点列表保存到可执行文件目录下的 waypoints.xml
	BOOL SavePage2DataToXml();

	// 显示航路点数据到列表控件
	void DisplayWaypoints(const Waypoint waypoints[100], int nCount);
	
	// 获取当前显示的航路点数据（用于编辑后保存）
	void GetWaypointsFromList(Waypoint waypoints[100], int& nCount);
	// 从主对话框的地图选点结果写入第 n 个航路点（第1次点击->1号点，以此类推）
	// 返回值：true 表示已装订到第100个航路点，本轮航点装订可自动结束
	bool ApplyWaypointFromMap(double lat, double lng);
	// 重置航路点装订计数器（重新从1号点开始装订）
	void ResetWaypointPickFromMap();
	// 当前航路点数量（用于判断是否有历史选点数据）
	int GetWaypointCount() const { return m_nCurrentWaypointCount; }
	// 清空航路点列表（经度纬度高度全0）并重置选点计数器，用于“是：全部清除重新选点”
	void ClearAllWaypointsAndResetPick();
	
	// 列表控件编辑相关
	afx_msg void OnNMDblclkListWaypoints(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickListWaypoints(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnKillfocusEditInline();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
	// 将 PAGE2 编辑框/航路点列表数据按当前绘图逻辑加载到地图（由“加载当前数据至主GUI”按钮调用）
	void LoadPage2DataToMap();
	afx_msg void OnBnClickedButtonLoadData2GUI();  // 加载当前数据至主GUI：将本页数据加载到地图绘图
	afx_msg void OnBnClickedButtonSaveData2xml();  // 保存当前数据至 xml 文件

private:
	CFWGCSDlgDlg* m_pMainDlg;  // 主对话框指针
	CEdit m_editInline;         // 内联编辑控件
	int m_nEditingItem;         // 当前编辑的行索引
	int m_nEditingSubItem;     // 当前编辑的列索引
	Waypoint m_currentWaypoints[100];  // 当前显示的航路点数据
	int m_nCurrentWaypointCount;        // 当前航路点数量
	int m_nNextWaypointIndexForMapPick; // 地图选点装订时，下一个要写入的航路点索引（0-based，对应1~100号）
	
	// 开始编辑单元格
	void StartEditCell(int nItem, int nSubItem);
	// 结束编辑单元格
	void EndEditCell(BOOL bCancel = FALSE);
};

