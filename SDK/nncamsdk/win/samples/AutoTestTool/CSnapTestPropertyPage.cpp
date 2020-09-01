#include "stdafx.h"
#include "global.h"
#include "AutoTestTool.h"
#include "CSnapTestPropertyPage.h"

CSnapTestPropertyPage::CSnapTestPropertyPage()
	: CPropertyPage(IDD_PROPERTY_SNAP_TEST)
	, m_bStarting(false), m_totalCount(0), m_count(0), m_hThread(nullptr)
{
}

int CSnapTestPropertyPage::GetTotalCount() const
{
	return m_totalCount;
}

bool CSnapTestPropertyPage::IsStarting() const
{
	return m_bStarting;
}

void CSnapTestPropertyPage::UpdateHint()
{
	CString str;
	str.Format(_T("%d/%d"), m_count, m_totalCount);
	SetDlgItemText(IDC_STATIC_INFO, str);
}

void CSnapTestPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

LRESULT CSnapTestPropertyPage::OnSnapCountUpdate(WPARAM wp, LPARAM lp)
{
	++m_count;
	UpdateHint();
	if (m_count == m_totalCount)
	{
		g_bSnapTesting = m_bStarting = false;
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = nullptr;
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		AfxMessageBox(_T("Snap test completed."));
		SetDlgItemText(IDC_BUTTON_SNAP_START, _T("Start"));
		GetDlgItem(IDC_EDIT_SNAP_COUNT)->EnableWindow(TRUE);
		m_count = 0;
		UpdateHint();
	}

	return 0;
}

BEGIN_MESSAGE_MAP(CSnapTestPropertyPage, CPropertyPage)
	ON_EN_CHANGE(IDC_EDIT_SNAP_COUNT, &CSnapTestPropertyPage::OnEnChangeEditSnapCount)
	ON_BN_CLICKED(IDC_BUTTON_SNAP_START, &CSnapTestPropertyPage::OnBnClickedButtonStart)
	ON_MESSAGE(WM_USER_SNAP_COUNT, &CSnapTestPropertyPage::OnSnapCountUpdate)
END_MESSAGE_MAP()

void CSnapTestPropertyPage::OnEnChangeEditSnapCount()
{
	m_totalCount = GetDlgItemInt(IDC_EDIT_SNAP_COUNT);
	UpdateHint();
	GetDlgItem(IDC_BUTTON_SNAP_START)->EnableWindow(m_totalCount > 0);
}

static unsigned __stdcall SnapTestThreadProc(LPVOID lpParam)
{
	CSnapTestPropertyPage* pN = (CSnapTestPropertyPage*)lpParam;
	int totalCount = pN->GetTotalCount();
	for (int i = 0; i < totalCount; ++i)
	{
		if (!pN->IsStarting())
			break;

		g_snapCount = i;

		int stillResCount = Toupcam_get_StillResolutionNumber(g_hCam);
		for (int k = 0; k < stillResCount; ++k)
		{
			g_bSnapFinished = false;
			Toupcam_Snap(g_hCam, k);

			int cnt = 0;
			while (!g_bSnapFinished)
			{
				Sleep(50);
				++cnt;
				if (cnt > 60)
					break;
			}
		}

		pN->PostMessage(WM_USER_SNAP_COUNT);
	}

	return 0;
}

void CSnapTestPropertyPage::OnBnClickedButtonStart()
{
	if (m_bStarting)
	{
		g_bSnapTesting = m_bStarting = false;
		SetDlgItemText(IDC_BUTTON_SNAP_START, _T("Start"));
		GetDlgItem(IDC_EDIT_SNAP_COUNT)->EnableWindow(TRUE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		m_count = 0;
		UpdateHint();
	}
	else
	{
		g_snapDir = GetAppTimeDir(_T("SnapTest"));
		if (!PathIsDirectory(g_snapDir))
			SHCreateDirectory(m_hWnd, (LPCTSTR)g_snapDir);

		g_bSnapTesting = m_bStarting = true;
		g_bEnableCheckBlack = false;
		SetDlgItemText(IDC_BUTTON_SNAP_START, _T("Stop"));
		GetDlgItem(IDC_EDIT_SNAP_COUNT)->EnableWindow(FALSE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_DISABLED);
		m_hThread = (HANDLE)_beginthreadex(nullptr, 0, SnapTestThreadProc, this, 0, nullptr);
	}
}

BOOL CSnapTestPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	UpdateHint();
	GetDlgItem(IDC_BUTTON_SNAP_START)->EnableWindow(FALSE);

	return TRUE;  
}
