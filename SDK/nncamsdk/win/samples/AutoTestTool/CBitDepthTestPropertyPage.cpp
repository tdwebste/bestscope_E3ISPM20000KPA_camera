#include "stdafx.h"
#include "global.h"
#include "AutoTestTool.h"
#include "CBitDepthTestPropertyPage.h"

CBitDepthTestPropertyPage::CBitDepthTestPropertyPage()
	: CPropertyPage(IDD_PROPERTY_BITDEPTH_TEST)
	, m_bStarting(false), m_totalCount(0)
	, m_count(0), m_hThread(nullptr)
{
}

int CBitDepthTestPropertyPage::GetTotalCount() const
{
	return m_totalCount;
}

BOOL CBitDepthTestPropertyPage::IsStarting() const
{
	return m_bStarting;
}

void CBitDepthTestPropertyPage::UpdateHint()
{
	CString str;
	str.Format(_T("%d/%d"), m_count, m_totalCount);
	SetDlgItemText(IDC_STATIC_BITDEPTH_TEST_HINT, str);
}

void CBitDepthTestPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

LRESULT CBitDepthTestPropertyPage::OnBitDepthTestCountUpdate(WPARAM wp, LPARAM lp)
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
		AfxMessageBox(_T("Bitdepth test completed."));
		SetDlgItemText(IDC_BUTTON_BITDEPTH_TEST_START, _T("Start"));
		GetDlgItem(IDC_EDIT_BITDEPTH_TEST_CNT)->EnableWindow(TRUE);
		m_count = 0;
		UpdateHint();
	}

	return 0;
}

BEGIN_MESSAGE_MAP(CBitDepthTestPropertyPage, CPropertyPage)
	ON_EN_CHANGE(IDC_EDIT_BITDEPTH_TEST_CNT, &CBitDepthTestPropertyPage::OnEnChangeEditBitDepthTestCount)
	ON_BN_CLICKED(IDC_BUTTON_BITDEPTH_TEST_START, &CBitDepthTestPropertyPage::OnBnClickedButtonBitDepthTestStart)
	ON_MESSAGE(WM_USER_BITDEPTH_TEST_COUNT, &CBitDepthTestPropertyPage::OnBitDepthTestCountUpdate)
END_MESSAGE_MAP()

BOOL CBitDepthTestPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	UpdateHint();
	GetDlgItem(IDC_BUTTON_BITDEPTH_TEST_START)->EnableWindow(FALSE);

	return TRUE;
}

void CBitDepthTestPropertyPage::OnEnChangeEditBitDepthTestCount()
{
	m_totalCount = GetDlgItemInt(IDC_EDIT_BITDEPTH_TEST_CNT);
	UpdateHint();
	GetDlgItem(IDC_BUTTON_BITDEPTH_TEST_START)->EnableWindow(m_totalCount > 0);
}

static unsigned __stdcall BitDepthTestThreadProc(LPVOID lpParam)
{
	CBitDepthTestPropertyPage* pN = (CBitDepthTestPropertyPage*)lpParam;
	int totalCount = pN->GetTotalCount();
	for (int i = 0; i < totalCount; ++i)
	{
		if (!pN->IsStarting())
			break;

		int bitDepth = 0;
		Toupcam_get_Option(g_hCam, TOUPCAM_OPTION_BITDEPTH, &bitDepth);
		bitDepth = !bitDepth;
		Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_BITDEPTH, bitDepth);
		Sleep(1000);
		bitDepth = !bitDepth;
		Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_BITDEPTH, bitDepth);
		Sleep(1000);

		pN->PostMessage(WM_USER_BITDEPTH_TEST_COUNT);
	}

	return 0;
}

void CBitDepthTestPropertyPage::OnBnClickedButtonBitDepthTestStart()
{
	if (m_bStarting)
	{
		m_bStarting = false;
		SetDlgItemText(IDC_BUTTON_BITDEPTH_TEST_START, _T("Start"));
		GetDlgItem(IDC_EDIT_BITDEPTH_TEST_CNT)->EnableWindow(TRUE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		m_count = 0;
		UpdateHint();
	}
	else
	{
		m_bStarting = true;
		g_bEnableCheckBlack = false;
		SetDlgItemText(IDC_BUTTON_BITDEPTH_TEST_START, _T("Stop"));
		GetDlgItem(IDC_EDIT_BITDEPTH_TEST_CNT)->EnableWindow(FALSE);
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_DISABLED);
		m_hThread = (HANDLE)_beginthreadex(nullptr, 0, BitDepthTestThreadProc, this, 0, nullptr);
	}
}
