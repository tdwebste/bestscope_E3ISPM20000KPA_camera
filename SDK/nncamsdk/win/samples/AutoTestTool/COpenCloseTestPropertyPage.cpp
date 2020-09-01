#include "stdafx.h"
#include "global.h"
#include "AutoTestTool.h"
#include "COpenCloseTestPropertyPage.h"
#include "AutoTestToolDlg.h"

COpenCloseTestPropertyPage::COpenCloseTestPropertyPage()
	: CPropertyPage(IDD_PROPERTY_OPEN_CLOSE_TEST)
	, m_bStarting(false), m_totalCount(0)
	, m_count(0), m_hThread(nullptr)
{
}

int COpenCloseTestPropertyPage::GetTotalCount() const
{
	return m_totalCount;
}

bool COpenCloseTestPropertyPage::IsStarting() const
{
	return m_bStarting;
}

void COpenCloseTestPropertyPage::UpdateHint()
{
	CString str;
	str.Format(_T("%d/%d"), m_count, m_totalCount);
	SetDlgItemText(IDC_STATIC_OPEN_CLOSE_TEST_HINT, str);
}

void COpenCloseTestPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

LRESULT COpenCloseTestPropertyPage::OnOpenCloseTestCountUpdate(WPARAM wp, LPARAM lp)
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
		AfxMessageBox(_T("Open/close test completed."));
		SetDlgItemText(IDC_BUTTON_OPEN_CLOSE_TEST_START, _T("Start"));
		GetDlgItem(IDC_EDIT_OPEN_CLOSE_CNT)->EnableWindow(TRUE);
		m_count = 0;
		UpdateHint();
	}

	return 0;
}

BEGIN_MESSAGE_MAP(COpenCloseTestPropertyPage, CPropertyPage)
	ON_EN_CHANGE(IDC_EDIT_OPEN_CLOSE_CNT, &COpenCloseTestPropertyPage::OnEnChangeEditOpenCloseCnt)
	ON_BN_CLICKED(IDC_BUTTON_OPEN_CLOSE_TEST_START, &COpenCloseTestPropertyPage::OnBnClickedButtonOpenCloseTestStart)
	ON_MESSAGE(WM_USER_OPEN_CLOSE_TEST_COUNT, &COpenCloseTestPropertyPage::OnOpenCloseTestCountUpdate)
END_MESSAGE_MAP()

BOOL COpenCloseTestPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	UpdateHint();
	GetDlgItem(IDC_BUTTON_OPEN_CLOSE_TEST_START)->EnableWindow(FALSE);

	return TRUE;  
}

void COpenCloseTestPropertyPage::OnEnChangeEditOpenCloseCnt()
{
	m_totalCount = GetDlgItemInt(IDC_EDIT_OPEN_CLOSE_CNT);
	UpdateHint();
	GetDlgItem(IDC_BUTTON_OPEN_CLOSE_TEST_START)->EnableWindow(m_totalCount > 0);
}

static unsigned __stdcall OpenCloseTestThreadProc(LPVOID lpParam)
{
	COpenCloseTestPropertyPage* pN = (COpenCloseTestPropertyPage*)lpParam;
	int totalCount = pN->GetTotalCount();
	for (int i = 0; i < totalCount; ++i)
	{
		if (!pN->IsStarting())
			break;

		g_bOpenCloseFinished = false;
		g_pMainDlg->PostMessage(WM_USER_OPEN_CLOSE);
		while (!g_bOpenCloseFinished)
			Sleep(50);

		Sleep(500);

		g_bOpenCloseFinished = false;
		g_pMainDlg->PostMessage(WM_USER_OPEN_CLOSE);
		while (!g_bOpenCloseFinished)
			Sleep(50);

		g_snapCount = i;
		g_bImageShoot = true;
		while (g_bImageShoot)
			Sleep(50);

		Sleep(500);

		pN->PostMessage(WM_USER_OPEN_CLOSE_TEST_COUNT);

		if (g_bBlack)
			break;
	}

	return 0;
}

void COpenCloseTestPropertyPage::OnBnClickedButtonOpenCloseTestStart()
{
	if (m_bStarting)
	{
		m_bStarting = false;
		SetDlgItemText(IDC_BUTTON_OPEN_CLOSE_TEST_START, _T("Start"));
		GetDlgItem(IDC_EDIT_OPEN_CLOSE_CNT)->EnableWindow(TRUE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		m_count = 0;
		UpdateHint();
	}
	else
	{
		g_snapDir = GetAppTimeDir(_T("OpenCloseTest"));
		if (!PathIsDirectory(g_snapDir))
			SHCreateDirectory(m_hWnd, (LPCTSTR)g_snapDir);

		m_bStarting = g_bEnableCheckBlack = true;
		g_bBlack = false;
		SetDlgItemText(IDC_BUTTON_OPEN_CLOSE_TEST_START, _T("Stop"));
		GetDlgItem(IDC_EDIT_OPEN_CLOSE_CNT)->EnableWindow(FALSE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_DISABLED);
		m_hThread = (HANDLE)_beginthreadex(nullptr, 0, OpenCloseTestThreadProc, this, 0, nullptr);
	}
}
