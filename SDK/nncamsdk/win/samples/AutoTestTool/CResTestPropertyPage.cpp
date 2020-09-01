#include "stdafx.h"
#include "global.h"
#include "AutoTestTool.h"
#include "CResTestPropertyPage.h"
#include "AutoTestToolDlg.h"

CResTestPropertyPage::CResTestPropertyPage()
	: CPropertyPage(IDD_PROPERTY_RESOLUTION_TEST)
	, m_bStarting(false), m_totalCount(0), m_count(0), m_hThread(nullptr)
{
}

int CResTestPropertyPage::GetTotalCount() const
{
	return m_totalCount;
}

bool CResTestPropertyPage::IsStarting() const
{
	return m_bStarting;
}

void CResTestPropertyPage::UpdateHint()
{
	CString str;
	str.Format(_T("%d/%d"), m_count, m_totalCount);
	SetDlgItemText(IDC_STATIC_RESOLUTION_TEST_HINT, str);
}

void CResTestPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

LRESULT CResTestPropertyPage::OnResTestCountUpdate(WPARAM wp, LPARAM lp)
{
	++m_count;
	UpdateHint();
	if (m_count == m_totalCount || g_bBlack)
	{
		m_bStarting = false;
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = nullptr;
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		AfxMessageBox(_T("Resolution test completed."));
		SetDlgItemText(IDC_BUTTON_RESOLUTION_TEST_START, _T("Start"));
		GetDlgItem(IDC_EDIT_RESOLUTION_TEST_COUNT)->EnableWindow(TRUE);
		m_count = 0;
		UpdateHint();
	}

	return 0;
}

BEGIN_MESSAGE_MAP(CResTestPropertyPage, CPropertyPage)
	ON_EN_CHANGE(IDC_EDIT_RESOLUTION_TEST_COUNT, &CResTestPropertyPage::OnEnChangeEditResolutionTestCount)
	ON_BN_CLICKED(IDC_BUTTON_RESOLUTION_TEST_START, &CResTestPropertyPage::OnBnClickedButtonResolutionTestStart)
	ON_MESSAGE(WM_USER_RES_TEST_COUNT, &CResTestPropertyPage::OnResTestCountUpdate)
END_MESSAGE_MAP()

BOOL CResTestPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	UpdateHint();
	GetDlgItem(IDC_BUTTON_RESOLUTION_TEST_START)->EnableWindow(FALSE);

	return TRUE;  
}

void CResTestPropertyPage::OnEnChangeEditResolutionTestCount()
{
	m_totalCount = GetDlgItemInt(IDC_EDIT_RESOLUTION_TEST_COUNT);
	UpdateHint();
	GetDlgItem(IDC_BUTTON_RESOLUTION_TEST_START)->EnableWindow(m_totalCount > 0);
}

static unsigned __stdcall ResTestThreadProc(LPVOID lpParam)
{
	CResTestPropertyPage* pN = (CResTestPropertyPage*)lpParam;
	int totalCount = pN->GetTotalCount();
	for (int i = 0; i < totalCount; ++i)
	{
		if (!pN->IsStarting())
			break;

		int resCount = Toupcam_get_ResolutionNumber(g_hCam);
		g_snapCount = i;
		for (int j = 0; j < resCount; ++j)
		{
			g_bResChangedFinished = false;
			g_pMainDlg->PostMessage(WM_USER_PREVIEW_CHANGE, j);
			while (!g_bResChangedFinished)
				Sleep(50);
			g_bImageShoot = true;
			while (g_bImageShoot)
				Sleep(50);

			if (g_bBlack)
				break;
			Sleep(1000);
		}

		pN->PostMessage(WM_USER_RES_TEST_COUNT);

		if (g_bBlack)
			break;
	}

	return 0;
}

void CResTestPropertyPage::OnBnClickedButtonResolutionTestStart()
{
	if (m_bStarting)
	{
		m_bStarting = false;
		SetDlgItemText(IDC_BUTTON_RESOLUTION_TEST_START, _T("Start"));
		GetDlgItem(IDC_EDIT_RESOLUTION_TEST_COUNT)->EnableWindow(TRUE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		m_count = 0;
		UpdateHint();
	}
	else
	{
		g_snapDir = GetAppTimeDir(_T("ResTest"));
		if (!PathIsDirectory(g_snapDir))
			SHCreateDirectory(m_hWnd, (LPCTSTR)g_snapDir);

		m_bStarting = g_bEnableCheckBlack = true;
		g_bBlack = false;
		SetDlgItemText(IDC_BUTTON_RESOLUTION_TEST_START, _T("Stop"));
		GetDlgItem(IDC_EDIT_RESOLUTION_TEST_COUNT)->EnableWindow(FALSE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_DISABLED);
		m_hThread = (HANDLE)_beginthreadex(nullptr, 0, ResTestThreadProc, this, 0, nullptr);
	}
}
