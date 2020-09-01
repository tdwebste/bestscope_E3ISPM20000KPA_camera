#include "stdafx.h"
#include "global.h"
#include "AutoTestTool.h"
#include "CROITestPropertyPage.h"

CROITestPropertyPage::CROITestPropertyPage()
	: CPropertyPage(IDD_PROPERTY_ROI_TEST)
	, m_invertal(0), m_hThread(nullptr)
{
}

void CROITestPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

LRESULT CROITestPropertyPage::OnROITestOne(WPARAM wp, LPARAM lp)
{
	int curPos = ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS_ROI_TEST))->GetPos();
	((CProgressCtrl*)GetDlgItem(IDC_PROGRESS_ROI_TEST))->SetPos(curPos + 1);

	return 0;
}

LRESULT CROITestPropertyPage::OnROITestFinished(WPARAM wp, LPARAM lp)
{
	g_bROITesting = g_bROITest_StartSnap = false;
	WaitForSingleObject(m_hThread, INFINITE);
	CloseHandle(m_hThread);
	m_hThread = nullptr;
	CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
	pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
	AfxMessageBox(_T("ROI test completed."));
	GetDlgItem(IDC_EDIT_INTERVAL)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_START)->SetWindowText(_T("Start"));
	((CProgressCtrl*)GetDlgItem(IDC_PROGRESS_ROI_TEST))->SetPos(0);
	Toupcam_put_Roi(g_hCam, 0, 0, 0, 0);

	return 0;
}

int CROITestPropertyPage::GetInvertal() const
{
	return m_invertal;
}

BEGIN_MESSAGE_MAP(CROITestPropertyPage, CPropertyPage)
	ON_BN_CLICKED(IDC_BUTTON_START, &CROITestPropertyPage::OnBnClickedButtonStart)
	ON_EN_CHANGE(IDC_EDIT_INTERVAL, &CROITestPropertyPage::OnEnChangeEditInterval)
	ON_MESSAGE(WM_USER_ROI_TEST_ONE, &CROITestPropertyPage::OnROITestOne)
	ON_MESSAGE(WM_USER_ROI_TEST_FINISHED, &CROITestPropertyPage::OnROITestFinished)
END_MESSAGE_MAP()

static unsigned __stdcall ROITestThreadProc(LPVOID lpParam)
{
	CROITestPropertyPage* pN = (CROITestPropertyPage*)lpParam;

	int resWidth = 0, resHeight = 0;
	Toupcam_get_Size(g_hCam, &resWidth, &resHeight);
	int invertal = pN->GetInvertal();
	for (int i = resWidth; i > 0; i = i - invertal)
	{
		if (!g_bROITesting)
			break;

		for (int j = resHeight; j > 0; j = j - invertal)
		{
			if (!g_bROITesting)
				break;

			g_bROITest_SnapFinished = false;
			Toupcam_put_Roi(g_hCam, 0, 0, i, j);
			Sleep(1000);
			g_bROITest_StartSnap = true;
			while (!g_bROITest_SnapFinished)
				Sleep(50);

			Sleep(1000);
			pN->PostMessage(WM_USER_ROI_TEST_ONE);
		}
	}
	pN->PostMessage(WM_USER_ROI_TEST_FINISHED);

	return 0;
}

void CROITestPropertyPage::OnBnClickedButtonStart()
{
	if (g_bROITesting)
	{
		g_ROITestCount = 0;
		g_bROITesting = g_bROITest_StartSnap = false;
		GetDlgItem(IDC_BUTTON_START)->SetWindowText(_T("Start"));
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED);
		GetDlgItem(IDC_EDIT_INTERVAL)->EnableWindow(TRUE);
		((CProgressCtrl*)GetDlgItem(IDC_PROGRESS_ROI_TEST))->SetPos(0);
		Toupcam_put_Roi(g_hCam, 0, 0, 0, 0);
	}
	else
	{
		g_snapDir = GetAppTimeDir(_T("ROITest"));
		if (!PathIsDirectory(g_snapDir))
			SHCreateDirectory(m_hWnd, (LPCTSTR)g_snapDir);

		g_ROITestCount = 0;
		g_bROITesting = true;
		g_bROITest_StartSnap = false;
		GetDlgItem(IDC_BUTTON_START)->SetWindowText(_T("Stop"));
		CMenu* pMenu = GetParent()->GetSystemMenu(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_DISABLED);
		GetDlgItem(IDC_EDIT_INTERVAL)->EnableWindow(FALSE);
		int resWidth = 0, resHeight = 0;
		Toupcam_get_Size(g_hCam, &resWidth, &resHeight);
		int widthCnt = ceil(resWidth / (double)m_invertal);
		int heightCnt = ceil(resHeight / (double)m_invertal);
		((CProgressCtrl*)GetDlgItem(IDC_PROGRESS_ROI_TEST))->SetRange(0, widthCnt * heightCnt);
		((CProgressCtrl*)GetDlgItem(IDC_PROGRESS_ROI_TEST))->SetPos(0);
		m_hThread = (HANDLE)_beginthreadex(nullptr, 0, ROITestThreadProc, this, 0, nullptr);
	}
}

BOOL CROITestPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	return TRUE; 
}

void CROITestPropertyPage::OnEnChangeEditInterval()
{
	m_invertal = GetDlgItemInt(IDC_EDIT_INTERVAL);
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(m_invertal > 0 && (m_invertal % 2 == 0));
}
