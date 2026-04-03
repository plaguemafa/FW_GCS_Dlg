#include "pch.h"
#include "framework.h"
#include "FW_GCS_Dlg.h"
#include "SurfaceCheckDlg.h"
#include "FW_GCS_DlgDlg.h"
#include "resource.h"
#include "UAV_DataLink.h"

IMPLEMENT_DYNAMIC(CSurfaceCheckDlg, CDialogEx)

namespace {
constexpr uint8_t kSurfaceCheckModeAuto = 0x0A;
constexpr uint8_t kSurfaceCheckModeManual = 0x0F;
}

CSurfaceCheckDlg::CSurfaceCheckDlg(CFWGCSDlgDlg* pMainDlg)
	: CDialogEx(IDD_SURFACE_CHECK, pMainDlg)
	, m_pMainDlg(pMainDlg)
{
}

void CSurfaceCheckDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_Display1, m_editElevator);
	DDX_Control(pDX, IDC_Display2, m_editAileron);
}

BEGIN_MESSAGE_MAP(CSurfaceCheckDlg, CDialogEx)
	ON_BN_CLICKED(IDC_Check_Mode_Auto, &CSurfaceCheckDlg::OnBnClickedCheckModeAuto)
	ON_BN_CLICKED(IDC_Check_Mode_Manual, &CSurfaceCheckDlg::OnBnClickedCheckModeManual)
	ON_BN_CLICKED(IDC_Auto_Surface_Check, &CSurfaceCheckDlg::OnBnClickedAutoSurfaceCheck)
	ON_BN_CLICKED(IDC_Elevator_Check, &CSurfaceCheckDlg::OnBnClickedElevatorCheck)
	ON_BN_CLICKED(IDC_Aileron_Check, &CSurfaceCheckDlg::OnBnClickedAileronCheck)
	ON_BN_CLICKED(IDC_Aileron_Check2, &CSurfaceCheckDlg::OnBnClickedSurfaceReset)
END_MESSAGE_MAP()

BOOL CSurfaceCheckDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(_T("舵面检查"));
	SyncRadiosFromMain();

	// 初始化两路舵面显示为 0°
	UpdateDisplay(m_editElevator, 0);
	UpdateDisplay(m_editAileron, 0);

	return TRUE;
}

void CSurfaceCheckDlg::PostNcDestroy()
{
	if (m_pMainDlg)
		m_pMainDlg->NotifySurfaceCheckDlgClosed();
	CDialogEx::PostNcDestroy();
	delete this;
}

void CSurfaceCheckDlg::SyncRadiosFromMain()
{
	if (!m_pMainDlg || GetSafeHwnd() == nullptr)
		return;
	const uint8_t mode = m_pMainDlg->GetSurfaceCheckMode();
	const int id = (mode == kSurfaceCheckModeManual) ? IDC_Check_Mode_Manual : IDC_Check_Mode_Auto;
	CheckRadioButton(IDC_Check_Mode_Auto, IDC_Check_Mode_Manual, id);
}

void CSurfaceCheckDlg::OnBnClickedCheckModeAuto()
{
	if (m_pMainDlg)
		m_pMainDlg->ApplySurfaceCheckMode(kSurfaceCheckModeAuto);
}

void CSurfaceCheckDlg::OnBnClickedCheckModeManual()
{
	if (m_pMainDlg)
		m_pMainDlg->ApplySurfaceCheckMode(kSurfaceCheckModeManual);
}

void CSurfaceCheckDlg::OnBnClickedAutoSurfaceCheck()
{
	if (!m_pMainDlg)
		return;

	// 仅在自动模式下允许触发自动偏舵
	if (m_pMainDlg->GetSurfaceCheckMode() != kSurfaceCheckModeAuto)
	{
		AfxMessageBox(_T("当前为手动模式，请先选择“自动模式”后再进行自动偏舵测试。"), MB_ICONINFORMATION | MB_OK);
		return;
	}

	m_pMainDlg->TriggerSurfaceAutoCheck();
}

void CSurfaceCheckDlg::OnBnClickedElevatorCheck()
{
	StepCommand(true);
}

void CSurfaceCheckDlg::OnBnClickedAileronCheck()
{
	StepCommand(false);
}

void CSurfaceCheckDlg::OnBnClickedSurfaceReset()
{
	m_elevatorIndex = 0;
	m_aileronIndex = 0;

	if (m_pMainDlg)
		m_pMainDlg->ResetSurfaceCommands();

	UpdateDisplay(m_editElevator, 0);
	UpdateDisplay(m_editAileron, 0);
}

void CSurfaceCheckDlg::StepCommand(bool isElevator)
{
	if (!m_pMainDlg)
		return;

	// 仅在手动模式下允许单步偏舵
	if (m_pMainDlg->GetSurfaceCheckMode() != kSurfaceCheckModeManual)
	{
		AfxMessageBox(_T("当前为自动模式，请先选择“手动模式”后再进行单步偏舵测试。"), MB_ICONINFORMATION | MB_OK);
		return;
	}

	// 舵偏角度序列：0, 15, 30, 15, 0, -15, -30, -15, 0 循环
	static const int kCmdSeq[] = { 0, 15, 30, 15, 0, -15, -30, -15, 0 };
	const int kSeqLen = static_cast<int>(sizeof(kCmdSeq) / sizeof(kCmdSeq[0]));

	int& index = isElevator ? m_elevatorIndex : m_aileronIndex;
	index = (index + 1) % kSeqLen;
	const int value = kCmdSeq[index];

	if (isElevator)
	{
		m_pMainDlg->UpdateElevatorCmd(static_cast<int8_t>(value));
		UpdateDisplay(m_editElevator, value);
	}
	else
	{
		m_pMainDlg->UpdateAileronCmd(static_cast<int8_t>(value));
		UpdateDisplay(m_editAileron, value);
	}
}

void CSurfaceCheckDlg::UpdateDisplay(CEdit& editCtrl, int value)
{
	if (editCtrl.GetSafeHwnd() == nullptr)
		return;

	CString text;
	text.Format(_T("%d"), value);
	editCtrl.SetWindowTextW(text);
}