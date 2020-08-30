#include "stdafx.h"
#include "demomono.h"
#include "demomonoDlg.h"
#include <InitGuid.h>
#include <wincodec.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CdemomonoDlg::CdemomonoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CdemomonoDlg::IDD, pParent)
	, m_hcam(NULL)
	, m_pImageData(NULL)
	, m_pDisplayData(NULL)
	, m_maxBitDepth(0)
	, m_bBitDepth(false)
{
}

void CdemomonoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CdemomonoDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, &CdemomonoDlg::OnBnClickedButton1)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CdemomonoDlg::OnCbnSelchangeCombo1)
	ON_MESSAGE(MSG_CAMEVENT, &CdemomonoDlg::OnMsgCamevent)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON2, &CdemomonoDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_CHECK1, &CdemomonoDlg::OnBnClickedCheck1)
	ON_WM_HSCROLL()
	ON_COMMAND_RANGE(IDM_SNAP_RESOLUTION, IDM_SNAP_RESOLUTION + NNCAM_MAX, &CdemomonoDlg::OnSnapResolution)
END_MESSAGE_MAP()

BOOL CdemomonoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK2)->EnableWindow(TRUE);
	GetDlgItem(IDC_SLIDER1)->EnableWindow(FALSE);
	GetDlgItem(IDC_COMBO1)->EnableWindow(FALSE);

	return TRUE;
}

void CdemomonoDlg::OnBnClickedButton1()
{
	if (m_hcam)
		return;

	m_hcam = Nncam_Open(NULL);
	if (NULL == m_hcam)
	{
		AfxMessageBox(_T("No Device"));
		return;
	}
	GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);

	CComboBox* pCombox = (CComboBox*)GetDlgItem(IDC_COMBO1);
	pCombox->ResetContent();
	int n = (int)Nncam_get_ResolutionNumber(m_hcam);
	if (n > 0)
	{
		TCHAR txt[128];
		int nWidth, nHeight;
		for (int i = 0; i < n; ++i)
		{
			Nncam_get_Resolution(m_hcam, i, &nWidth, &nHeight);
			_stprintf(txt, _T("%d * %d"), nWidth, nHeight);
			pCombox->AddString(txt);
		}

		unsigned nCur = 0;
		Nncam_get_eSize(m_hcam, &nCur);
		pCombox->SetCurSel(nCur);
	}

	GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
	m_maxBitDepth = Nncam_get_MaxBitDepth(m_hcam);

	if ((8 == m_maxBitDepth) || IsDlgButtonChecked(IDC_CHECK2))
	{
		m_bBitDepth = false;
		CheckDlgButton(IDC_CHECK2, 0);
		Nncam_put_Option(m_hcam, NNCAM_OPTION_RGB, 3);
		Nncam_put_Option(m_hcam, NNCAM_OPTION_BITDEPTH, 0);
	}
	else
	{
		m_bBitDepth = true;
		Nncam_put_Option(m_hcam, NNCAM_OPTION_RGB, 4);
		Nncam_put_Option(m_hcam, NNCAM_OPTION_BITDEPTH, 1);
	}
	
	StartDevice();
}

void CdemomonoDlg::StartDevice()
{
	int nWidth = 0, nHeight = 0;
	HRESULT hr = Nncam_get_Size(m_hcam, &nWidth, &nHeight);
	if (FAILED(hr))
		return;

	m_pDisplayData = (PBYTE)realloc(m_pDisplayData, TDIBWIDTHBYTES(nWidth * 24) * nHeight);
	if (m_bBitDepth)
		m_pImageData = (PBYTE)realloc(m_pImageData, TDIBWIDTHBYTES(nWidth * 16) * nHeight);
	else
		m_pImageData = (PBYTE)realloc(m_pImageData, TDIBWIDTHBYTES(nWidth * 8) * nHeight);

	Nncam_StartPullModeWithWndMsg(m_hcam, m_hWnd, MSG_CAMEVENT);

	BOOL bEnableAutoExpo = TRUE;
	Nncam_get_AutoExpoEnable(m_hcam, &bEnableAutoExpo);
	CheckDlgButton(IDC_CHECK1, bEnableAutoExpo ? 1 : 0);
	GetDlgItem(IDC_SLIDER1)->EnableWindow(!bEnableAutoExpo);

	unsigned nMinExpTime, nMaxExpTime, nDefExpTime;
	Nncam_get_ExpTimeRange(m_hcam, &nMinExpTime, &nMaxExpTime, &nDefExpTime);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetRange(nMinExpTime / 1000, nMaxExpTime / 1000);

	OnEventExpo();

	GetDlgItem(IDC_CHECK1)->EnableWindow(TRUE);
	GetDlgItem(IDC_COMBO1)->EnableWindow(TRUE);
}

void CdemomonoDlg::OnCbnSelchangeCombo1()
{
	if (NULL == m_hcam)
		return;

	CComboBox* pCombox = (CComboBox*)GetDlgItem(IDC_COMBO1);
	int nSel = pCombox->GetCurSel();
	if (nSel < 0)
		return;

	unsigned nResolutionIndex = 0;
	HRESULT hr = Nncam_get_eSize(m_hcam, &nResolutionIndex);
	if (FAILED(hr))
		return;

	if (nResolutionIndex != nSel)
	{
		hr = Nncam_Stop(m_hcam);
		if (FAILED(hr))
			return;

		Nncam_put_eSize(m_hcam, nSel);

		StartDevice();
	}
}

LRESULT CdemomonoDlg::OnMsgCamevent(WPARAM wp, LPARAM /*lp*/)
{
	switch (wp)
	{
	case NNCAM_EVENT_ERROR:
	case NNCAM_EVENT_TIMEOUT:
		OnEventError();
		break;
	case NNCAM_EVENT_DISCONNECTED:
		OnEventDisconnected();
		break;
	case NNCAM_EVENT_IMAGE:
		OnEventImage();
		break;
	case NNCAM_EVENT_EXPOSURE:
		OnEventExpo();
		break;
	default:
		break;
	}
	return 0;
}

void CdemomonoDlg::OnEventDisconnected()
{
	if (m_hcam)
	{
		Nncam_Close(m_hcam);
		m_hcam = NULL;
	}
	AfxMessageBox(_T("The camera is disconnected, mybe has been pulled out."));
}

void CdemomonoDlg::OnEventError()
{
	if (m_hcam)
	{
		Nncam_Close(m_hcam);
		m_hcam = NULL;
	}
	AfxMessageBox(_T("Error"));
}

void CdemomonoDlg::OnEventExpo()
{
	unsigned nTime = 0;
	Nncam_get_ExpoTime(m_hcam, &nTime);
	SetDlgItemInt(IDC_STATIC1, nTime / 1000, FALSE);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetPos(nTime / 1000);
}

void CdemomonoDlg::OnEventImage()
{
	NncamFrameInfoV2 info;
	/* bits: 24 (RGB24), 32 (RGB32), 8 (Gray) or 16 (Gray). In RAW mode, this parameter is ignored. */
	HRESULT hr = Nncam_PullImageV2(m_hcam, m_pImageData, m_bBitDepth ? 16 : 8, &info);
	if (SUCCEEDED(hr))
	{
		BITMAPINFOHEADER header = { sizeof(header) };
		header.biPlanes = 1;
		header.biBitCount = 24;
		header.biWidth = info.width;
		header.biHeight = info.height;
		header.biSizeImage = TDIBWIDTHBYTES(header.biWidth * 24) * header.biHeight;
		int pitchDst = TDIBWIDTHBYTES(header.biWidth * 24);
		if (m_bBitDepth)
		{
			int pitchSrc = TDIBWIDTHBYTES(header.biWidth * 16);
			for (int i = 0; i < header.biHeight; ++i)
			{
				for (int j = 0; j < header.biWidth; ++j)
				{
					unsigned short value = (*((PUSHORT)(m_pImageData + i * pitchSrc + j * 2))) >> (m_maxBitDepth - 8);
					*(m_pDisplayData + i * pitchDst + 3 * j)
						= *(m_pDisplayData + i * pitchDst + 3 * j + 1)
						= *(m_pDisplayData + i * pitchDst + 3 * j + 2)
						= value;
				}
			}
		}
		else
		{
			int pitchSrc = TDIBWIDTHBYTES(header.biWidth * 8);
			for (int i = 0; i < header.biHeight; ++i)
			{
				for (int j = 0; j < header.biWidth; ++j)
				{
					*(m_pDisplayData + i * pitchDst + 3 * j)
						= *(m_pDisplayData + i * pitchDst + 3 * j + 1)
						= *(m_pDisplayData + i * pitchDst + 3 * j + 2)
						= *(m_pImageData + i * pitchSrc + j);
				}
			}
		}
		
		CClientDC dc(this);
		CRect rc, rcStartButton;
		GetClientRect(&rc);
		GetDlgItem(IDC_BUTTON1)->GetWindowRect(&rcStartButton);
		ScreenToClient(&rcStartButton);
		rc.left = rcStartButton.right + 4;
		rc.top += 4;
		rc.bottom -= 4;
		rc.right -= 4;

		int m = dc.SetStretchBltMode(COLORONCOLOR);
		StretchDIBits(dc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0, 0,
			header.biWidth, header.biHeight, m_pDisplayData, (BITMAPINFO*)&header, DIB_RGB_COLORS, SRCCOPY);
		dc.SetStretchBltMode(m);
	}
}

void CdemomonoDlg::OnDestroy()
{
	if (m_hcam)
	{
		Nncam_Close(m_hcam);
		m_hcam = NULL;
	}
	if (m_pImageData)
	{
		free(m_pImageData);
		m_pImageData = NULL;
	}
	if (m_pDisplayData)
	{
		free(m_pDisplayData);
		m_pDisplayData = NULL;
	}

	CDialog::OnDestroy();
}

void CdemomonoDlg::OnSnapResolution(UINT nID)
{
	Nncam_Snap(m_hcam, nID - IDM_SNAP_RESOLUTION);
}

void CdemomonoDlg::OnBnClickedButton2()
{
	int n = Nncam_get_StillResolutionNumber(m_hcam);
	if (n <= 0)
	{
		unsigned e = 0;
		Nncam_get_eSize(m_hcam, &e);
		Nncam_Snap(m_hcam, e);
	}
	else
	{
		CPoint pt;
		GetCursorPos(&pt);
		CMenu menu;
		menu.CreatePopupMenu();
		TCHAR text[64];
		int w, h;
		for (int i = 0; i < n; ++i)
		{
			Nncam_get_StillResolution(m_hcam, i, &w, &h);
			_stprintf(text, _T("%d * %d"), w, h);
			menu.AppendMenu(MF_STRING, IDM_SNAP_RESOLUTION + i, text);
		}
		menu.TrackPopupMenu(TPM_RIGHTALIGN, pt.x, pt.y, this);
	}
}

void CdemomonoDlg::OnBnClickedCheck1()
{
	if (m_hcam)
		Nncam_put_AutoExpoEnable(m_hcam, IsDlgButtonChecked(IDC_CHECK1) ? TRUE : FALSE);
	GetDlgItem(IDC_SLIDER1)->EnableWindow(IsDlgButtonChecked(IDC_CHECK1) ? FALSE : TRUE);
}

void CdemomonoDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == GetDlgItem(IDC_SLIDER1))
	{
		int nTime = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->GetPos();
		SetDlgItemInt(IDC_STATIC1, nTime, TRUE);
		Nncam_put_ExpoTime(m_hcam, nTime * 1000);
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}
