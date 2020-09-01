#include "stdafx.h"
#include "global.h"
#include "AutoTestTool.h"
#include "CTriggerTestPropertyPage.h"
#include "AutoTestToolDlg.h"

CTriggerTestPropertyPage::CTriggerTestPropertyPage()
	: CPropertyPage(IDD_PROPERTY_TRIGGER_TEST)
	, m_bStarting(false), m_totalCount(0), m_count(0), m_interval(0), m_hThread(nullptr)
{

}

int CTriggerTestPropertyPage::GetTotalCount() const
{
	return m_totalCount;
}

int CTriggerTestPropertyPage::GetInterval() const
{
	return m_interval;
}

bool CTriggerTestPropertyPage::IsStarting() const
{
	return m_bStarting;
}

void CTriggerTestPropertyPage::UpdateHint()
{
	CString str;
	str.Format(_T("%d/%d"), m_count, m_totalCount);
	SetDlgItemText(IDC_STATIC_TRIGGER_TEST_HINT, str);
}

void CTriggerTestPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

LRESULT CTriggerTestPropertyPage::OnTriggerTestCountUpdate(WPARAM wp, LPARAM lp)
{
	++m_count;
	UpdateHint();
	if (m_count == m_totalCount)
	{
		m_bStarting = false;
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = nullptr;
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		AfxMessageBox(_T("Trigger test completed."));
		SetDlgItemText(IDC_BUTTON_TRIGGER_TEST_START, _T("Start"));
		GetDlgItem(IDC_EDIT_TRIGGER_TEST_TIMES)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT_TRIGGER_TEST_INTERVAL)->EnableWindow(TRUE);
		m_count = 0;
		UpdateHint();
		g_bTriggerTesting = false;
		Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_TRIGGER, 0);
	}

	return 0;
}

BEGIN_MESSAGE_MAP(CTriggerTestPropertyPage, CPropertyPage)
	ON_EN_CHANGE(IDC_EDIT_TRIGGER_TEST_TIMES, &CTriggerTestPropertyPage::OnEnChangeEditTriggerTestTimes)
	ON_EN_CHANGE(IDC_EDIT_TRIGGER_TEST_INTERVAL, &CTriggerTestPropertyPage::OnEnChangeEditTriggerTestInterval)
	ON_BN_CLICKED(IDC_BUTTON_TRIGGER_TEST_START, &CTriggerTestPropertyPage::OnBnClickedButtonTriggerTestStart)
	ON_MESSAGE(WM_USER_TRIGGER_TEST_COUNT, &CTriggerTestPropertyPage::OnTriggerTestCountUpdate)
END_MESSAGE_MAP()

BOOL CTriggerTestPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	UpdateHint();
	GetDlgItem(IDC_BUTTON_TRIGGER_TEST_START)->EnableWindow(FALSE);

	return TRUE;
}

void CTriggerTestPropertyPage::OnEnChangeEditTriggerTestTimes()
{
	m_totalCount = GetDlgItemInt(IDC_EDIT_TRIGGER_TEST_TIMES);
	UpdateHint();
	GetDlgItem(IDC_BUTTON_TRIGGER_TEST_START)->EnableWindow(m_totalCount > 0 && m_interval > 0);
}

void CTriggerTestPropertyPage::OnEnChangeEditTriggerTestInterval()
{
	m_interval = GetDlgItemInt(IDC_EDIT_TRIGGER_TEST_INTERVAL);
	GetDlgItem(IDC_BUTTON_TRIGGER_TEST_START)->EnableWindow(m_totalCount > 0 && m_interval > 0);
}

static unsigned __stdcall TriggerTestThreadProc(LPVOID lpParam)
{
	CTriggerTestPropertyPage* pN = (CTriggerTestPropertyPage*)lpParam;
	int totalCount = pN->GetTotalCount();
	for (int i = 0; i < totalCount; ++i)
	{
		if (!pN->IsStarting())
			break;

		Toupcam_Trigger(g_hCam, 1);
		Sleep(pN->GetInterval());

		pN->PostMessage(WM_USER_TRIGGER_TEST_COUNT);
	}

	return 0;
}

void CTriggerTestPropertyPage::OnBnClickedButtonTriggerTestStart()
{
	if (m_bStarting)
	{
		m_bStarting = g_bTriggerTesting = false;
		SetDlgItemText(IDC_BUTTON_TRIGGER_TEST_START, _T("Start"));
		GetDlgItem(IDC_EDIT_TRIGGER_TEST_TIMES)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT_TRIGGER_TEST_INTERVAL)->EnableWindow(TRUE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		m_count = 0;
		UpdateHint();
		Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_TRIGGER, 0);
	}
	else
	{
		g_snapDir = GetAppTimeDir(_T("TriggerTest"));
		if (!PathIsDirectory(g_snapDir))
			SHCreateDirectory(m_hWnd, (LPCTSTR)g_snapDir);

		Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_TRIGGER, 1);
		m_bStarting = g_bTriggerTesting = true;
		SetDlgItemText(IDC_BUTTON_TRIGGER_TEST_START, _T("Stop"));
		GetDlgItem(IDC_EDIT_TRIGGER_TEST_TIMES)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_TRIGGER_TEST_INTERVAL)->EnableWindow(FALSE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_DISABLED);
		m_hThread = (HANDLE)_beginthreadex(nullptr, 0, TriggerTestThreadProc, this, 0, nullptr);
	}
}