#include "stdafx.h"
#include "global.h"
#include "AutoTestTool.h"
#include "CPreviewSnapPropertyPage.h"
#include "AutoTestToolDlg.h"

CPreviewSnapPropertyPage::CPreviewSnapPropertyPage()
	: CPropertyPage(IDD_PROPERTY_PREVIEW_SNAP)
{
}

void CPreviewSnapPropertyPage::UpdateSnapRes()
{
	if (g_hCam)
	{
		int resCnt = Toupcam_get_StillResolutionNumber(g_hCam);
		CComboBox* resList = (CComboBox*)GetDlgItem(IDC_COMBO_SNAP);
		int width = 0, height = 0;
		resList->ResetContent();
		if (resCnt <= 0)
		{
			Toupcam_get_Size(g_hCam, &width, &height);
			CString str;
			str.Format(_T("%d x %d"), width, height);
			resList->AddString(str);
			resList->EnableWindow(FALSE);
		}
		else
		{
			for (int i = 0; i < resCnt; ++i)
			{
				Toupcam_get_StillResolution(g_hCam, i, &width, &height);
				CString str;
				str.Format(_T("%d x %d"), width, height);
				resList->AddString(str);
			}
			resList->EnableWindow(TRUE);
		}
		resList->SetCurSel(0);
	}
}

BEGIN_MESSAGE_MAP(CPreviewSnapPropertyPage, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_COMBO_PREVIEW, &CPreviewSnapPropertyPage::OnCbnSelchangeComboPreview)
	ON_BN_CLICKED(IDC_BUTTON_SNAP, &CPreviewSnapPropertyPage::OnBnClickedButtonSnap)
END_MESSAGE_MAP()

BOOL CPreviewSnapPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	if (g_hCam)
	{
		int resCnt = Toupcam_get_ResolutionNumber(g_hCam);
		CComboBox* previewResList = (CComboBox*)GetDlgItem(IDC_COMBO_PREVIEW);
		int width = 0, height = 0;
		for (int i = 0; i < resCnt; ++i)
		{
			Toupcam_get_Resolution(g_hCam, i, &width, &height);
			CString str;
			str.Format(_T("%d x %d"), width, height);
			previewResList->AddString(str);
		}
		unsigned resIndex = 0;
		Toupcam_get_eSize(g_hCam, &resIndex);
		previewResList->SetCurSel(resIndex);

		UpdateSnapRes();
	}

	return TRUE; 
}

void CPreviewSnapPropertyPage::OnCbnSelchangeComboPreview()
{
	CComboBox* previewResList = (CComboBox*)GetDlgItem(IDC_COMBO_PREVIEW);
	if (previewResList)
		g_pMainDlg->PostMessage(WM_USER_PREVIEW_CHANGE, previewResList->GetCurSel());
	UpdateSnapRes();
}

void CPreviewSnapPropertyPage::OnBnClickedButtonSnap()
{
	int resCnt = Toupcam_get_StillResolutionNumber(g_hCam);
	if (resCnt <= 0)
	{
		unsigned res = 0;
		Toupcam_get_eSize(g_hCam, &res);
		Toupcam_Snap(g_hCam, res);
	}
	else
	{
		CComboBox* resList = (CComboBox*)GetDlgItem(IDC_COMBO_SNAP);
		Toupcam_Snap(g_hCam, resList->GetCurSel());
	}
}
