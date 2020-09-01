#include "stdafx.h"
#include "global.h"
#include "AutoTestTool.h"
#include "CFrameRatePropertyPage.h"

CFrameRatePropertyPage::CFrameRatePropertyPage()
	: CPropertyPage(IDD_PROPERTY_FRAME_RATE)
{
}

void CFrameRatePropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER_FRAME_RATE, m_FrameRateSlider);
}

BEGIN_MESSAGE_MAP(CFrameRatePropertyPage, CPropertyPage)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

BOOL CFrameRatePropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	if (g_hCam)
	{
		int maxSpeed = Toupcam_get_MaxSpeed(g_hCam);
		m_FrameRateSlider.SetRange(0, maxSpeed);
		USHORT speed = 0;
		Toupcam_get_Speed(g_hCam, &speed);
		m_FrameRateSlider.SetPos(speed);
	}

	return TRUE; 
}

void CFrameRatePropertyPage::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == GetDlgItem(IDC_SLIDER_FRAME_RATE))
	{
		USHORT curSpeed = 0;
		Toupcam_get_Speed(g_hCam, &curSpeed);
		USHORT speed = m_FrameRateSlider.GetPos();
		if (speed != curSpeed)
			Toupcam_put_Speed(g_hCam, speed);
	}

	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}
