#include "stdafx.h"
#include "global.h"
#include "AutoTestTool.h"
#include "CSnapResTestPropertyPage.h"
#include "AutoTestToolDlg.h"

CSnapResTestPropertyPage::CSnapResTestPropertyPage()
	: CPropertyPage(IDD_PROPERTY_SNAP_RES_TEST)
	, m_bStarting(false), m_totalCount(0), m_count(0), m_hThread(nullptr)
{
}

int CSnapResTestPropertyPage::GetTotalCount() const
{
	return m_totalCount;
}

bool CSnapResTestPropertyPage::IsStarting() const
{
	return m_bStarting;
}

void CSnapResTestPropertyPage::UpdateHint()
{
	CString str;
	str.Format(_T("%d/%d"), m_count, m_totalCount);
	SetDlgItemText(IDC_STATIC_HINT, str);
}

void CSnapResTestPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

LRESULT CSnapResTestPropertyPage::OnSnapCountUpdate(WPARAM wp, LPARAM lp)
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
		AfxMessageBox(_T("Snap and resolution test completed."));
		SetDlgItemText(IDC_BUTTON_START, _T("Start"));
		GetDlgItem(IDC_EDIT_SNAP_COUNT)->EnableWindow(TRUE);
		m_count = 0;
		UpdateHint();
	}

	return 0;
}

BEGIN_MESSAGE_MAP(CSnapResTestPropertyPage, CPropertyPage)
	ON_EN_CHANGE(IDC_EDIT_SNAP_COUNT, &CSnapResTestPropertyPage::OnEnChangeEditSnapCount)
	ON_BN_CLICKED(IDC_BUTTON_START, &CSnapResTestPropertyPage::OnBnClickedButtonStart)
	ON_MESSAGE(WM_USER_SNAP_COUNT, &CSnapResTestPropertyPage::OnSnapCountUpdate)
END_MESSAGE_MAP()

void CSnapResTestPropertyPage::OnEnChangeEditSnapCount()
{
	m_totalCount = GetDlgItemInt(IDC_EDIT_SNAP_COUNT);
	UpdateHint();
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(m_totalCount > 0);
}

static unsigned __stdcall SnapResTestThreadProc(LPVOID lpParam)
{
	CSnapResTestPropertyPage* pN = (CSnapResTestPropertyPage*)lpParam;
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
		}

		pN->PostMessage(WM_USER_SNAP_COUNT);
	}

	return 0;
}

void CSnapResTestPropertyPage::OnBnClickedButtonStart()
{
	if (m_bStarting)
	{
		g_bSnapTesting = m_bStarting = false;
		SetDlgItemText(IDC_BUTTON_START, _T("Start"));
		GetDlgItem(IDC_EDIT_SNAP_COUNT)->EnableWindow(TRUE); 
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		m_count = 0;
		UpdateHint();
	}
	else
	{
		g_snapDir = GetAppTimeDir(_T("SnapResTest"));
		if (!PathIsDirectory(g_snapDir))
			SHCreateDirectory(m_hWnd, (LPCTSTR)g_snapDir);

		g_bSnapTesting = m_bStarting = true;
		g_bEnableCheckBlack = false;
		SetDlgItemText(IDC_BUTTON_START, _T("Stop"));
		GetDlgItem(IDC_EDIT_SNAP_COUNT)->EnableWindow(FALSE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_DISABLED);
		m_hThread = (HANDLE)_beginthreadex(nullptr, 0, SnapResTestThreadProc, this, 0, nullptr);
	}
}

BOOL CSnapResTestPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	UpdateHint();
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);

	return TRUE;  
}
