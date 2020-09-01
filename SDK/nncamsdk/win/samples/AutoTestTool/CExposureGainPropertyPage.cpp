#include "stdafx.h"
#include "global.h"
#include "AutoTestTool.h"
#include "CExposureGainPropertyPage.h"

CExposureGainPropertyPage::CExposureGainPropertyPage()
	: CPropertyPage(IDD_PROPERTY_EXPOSURE_GAIN)
{
}

void CExposureGainPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

LRESULT CExposureGainPropertyPage::OnAutoExposure(WPARAM wp, LPARAM lp)
{
	if (GetDlgItem(IDC_SLIDER_EXPOSURE))
	{
		unsigned time = 0;
		Toupcam_get_ExpoTime(g_hCam, &time);
		SetExpTimeValue(time);
	}

	if (GetDlgItem(IDC_SLIDER_GAIN))
	{
		unsigned short gain = 0;
		Toupcam_get_ExpoAGain(g_hCam, &gain);
		SetGainValue(gain);
	}

	return 0;
}

void CExposureGainPropertyPage::UpdateSlidersEnable()
{
	int bAutoExp = 0;
	Toupcam_get_AutoExpoEnable(g_hCam, &bAutoExp);
	GetDlgItem(IDC_SLIDER_TARGET)->EnableWindow(bAutoExp);
	GetDlgItem(IDC_SLIDER_EXPOSURE)->EnableWindow(!bAutoExp);
	GetDlgItem(IDC_SLIDER_GAIN)->EnableWindow(!bAutoExp);

	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_TARGET))->SetRange(TOUPCAM_AETARGET_MIN, TOUPCAM_AETARGET_MAX);
	unsigned short target = 0;
	Toupcam_get_AutoExpoTarget(g_hCam, &target);
	SetTargetValue(target);

	unsigned timeMin = 0, timeMax = 0, timeDef = 0, timeVal = 0;
	Toupcam_get_ExpTimeRange(g_hCam, &timeMin, &timeMax, &timeDef);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_EXPOSURE))->SetRange(timeMin, timeMax);
	Toupcam_get_ExpoTime(g_hCam, &timeVal);
	SetExpTimeValue(timeVal);

	unsigned short gainMin = 0, gainMax = 0, gainDef = 0, gainVal = 0;
	Toupcam_get_ExpoAGainRange(g_hCam, &gainMin, &gainMax, &gainDef);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_GAIN))->SetRange(gainMin, gainMax);
	Toupcam_get_ExpoAGain(g_hCam, &gainVal);
	SetGainValue(gainVal);
}

void CExposureGainPropertyPage::SetTargetValue(int value)
{
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_TARGET))->SetPos(value);
	SetDlgItemInt(IDC_STATIC_TARGET, value);
}

void CExposureGainPropertyPage::SetExpTimeValue(unsigned value)
{
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_EXPOSURE))->SetPos(value);
	CString str;
	str.Format(_T("%d us"), value);
	SetDlgItemText(IDC_STATIC_EXPOSURE, str);
}

void CExposureGainPropertyPage::SetGainValue(int value)
{
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER_GAIN))->SetPos(value);
	SetDlgItemInt(IDC_STATIC_GAIN, value);
}

BEGIN_MESSAGE_MAP(CExposureGainPropertyPage, CPropertyPage)
	ON_BN_CLICKED(IDC_CHECK_AUTO, &CExposureGainPropertyPage::OnBnClickedCheckAuto)
	ON_MESSAGE(WM_USER_AUTO_EXPOSURE, &CExposureGainPropertyPage::OnAutoExposure)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

void CExposureGainPropertyPage::OnBnClickedCheckAuto()
{
	Toupcam_put_AutoExpoEnable(g_hCam, ((CButton*)GetDlgItem(IDC_CHECK_AUTO))->GetCheck());
	UpdateSlidersEnable();
}

BOOL CExposureGainPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	int bAutoExp = 0;
	Toupcam_get_AutoExpoEnable(g_hCam, &bAutoExp);
	((CButton*)GetDlgItem(IDC_CHECK_AUTO))->SetCheck(bAutoExp);
	UpdateSlidersEnable();

	return TRUE;  
}

void CExposureGainPropertyPage::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == GetDlgItem(IDC_SLIDER_TARGET))
	{
		unsigned short curTarget = 0;
		Toupcam_get_AutoExpoTarget(g_hCam, &curTarget);
		unsigned short target = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_TARGET))->GetPos();
		if (target != curTarget)
		{
			Toupcam_put_AutoExpoTarget(g_hCam, target);
			SetDlgItemInt(IDC_STATIC_TARGET, target);
		}
	}
	else if (pScrollBar == GetDlgItem(IDC_SLIDER_EXPOSURE))
	{
		unsigned curTime = 0;
		Toupcam_get_ExpoTime(g_hCam, &curTime);
		unsigned time = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_EXPOSURE))->GetPos();
		if (time != curTime)
		{
			Toupcam_put_ExpoTime(g_hCam, time);
			CString str;
			str.Format(_T("%d us"), time);
			SetDlgItemText(IDC_STATIC_EXPOSURE, str);
		}
	}
	else if (pScrollBar == GetDlgItem(IDC_SLIDER_GAIN))
	{
		unsigned short curGain = 0;
		Toupcam_get_ExpoAGain(g_hCam, &curGain);
		unsigned short gain = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_GAIN))->GetPos();
		if (gain != curGain)
		{
			Toupcam_put_ExpoAGain(g_hCam, gain);
			SetDlgItemInt(IDC_STATIC_GAIN, gain);
		}
	}

	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}
