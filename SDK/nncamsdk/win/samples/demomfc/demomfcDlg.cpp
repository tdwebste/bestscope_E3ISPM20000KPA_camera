#include "stdafx.h"
#include "demomfc.h"
#include "demomfcDlg.h"
#include <InitGuid.h>
#include <wincodec.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static BOOL SaveImageByWIC(const wchar_t* strFilename, const void* pData, const BITMAPINFOHEADER* pHeader)
{
	GUID guidContainerFormat;
	if (PathMatchSpec(strFilename, L"*.bmp"))
		guidContainerFormat = GUID_ContainerFormatBmp;
	else if (PathMatchSpec(strFilename, L"*.jpg"))
		guidContainerFormat = GUID_ContainerFormatJpeg;
	else if (PathMatchSpec(strFilename, L"*.png"))
		guidContainerFormat = GUID_ContainerFormatPng;
	else
		return FALSE;

	CComPtr<IWICImagingFactory> spIWICImagingFactory;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory), (LPVOID*)&spIWICImagingFactory);
	if (FAILED(hr))
		return FALSE;

	CComPtr<IWICBitmapEncoder> spIWICBitmapEncoder;
	hr = spIWICImagingFactory->CreateEncoder(guidContainerFormat, NULL, &spIWICBitmapEncoder);
	if (FAILED(hr))
		return FALSE;

	CComPtr<IWICStream> spIWICStream;
	spIWICImagingFactory->CreateStream(&spIWICStream);
	if (FAILED(hr))
		return FALSE;

	hr = spIWICStream->InitializeFromFilename(strFilename, GENERIC_WRITE);
	if (FAILED(hr))
		return FALSE;

	hr = spIWICBitmapEncoder->Initialize(spIWICStream, WICBitmapEncoderNoCache);
	if (FAILED(hr))
		return FALSE;

	CComPtr<IWICBitmapFrameEncode> spIWICBitmapFrameEncode;
	CComPtr<IPropertyBag2> spIPropertyBag2;
	hr = spIWICBitmapEncoder->CreateNewFrame(&spIWICBitmapFrameEncode, &spIPropertyBag2);
	if (FAILED(hr))
		return FALSE;

	if (GUID_ContainerFormatJpeg == guidContainerFormat)
	{
		PROPBAG2 option = { 0 };
		option.pstrName = L"ImageQuality"; /* jpg quality, you can change this setting */
		CComVariant varValue(0.75f);
		spIPropertyBag2->Write(1, &option, &varValue);
	}
	hr = spIWICBitmapFrameEncode->Initialize(spIPropertyBag2);
	if (FAILED(hr))
		return FALSE;

	hr = spIWICBitmapFrameEncode->SetSize(pHeader->biWidth, pHeader->biHeight);
	if (FAILED(hr))
		return FALSE;

	WICPixelFormatGUID formatGUID = GUID_WICPixelFormat24bppBGR;
	hr = spIWICBitmapFrameEncode->SetPixelFormat(&formatGUID);
	if (FAILED(hr))
		return FALSE;

	LONG nWidthBytes = TDIBWIDTHBYTES(pHeader->biWidth * pHeader->biBitCount);
	for (LONG i = 0; i < pHeader->biHeight; ++i)
	{
		hr = spIWICBitmapFrameEncode->WritePixels(1, nWidthBytes, nWidthBytes, ((BYTE*)pData) + nWidthBytes * (pHeader->biHeight - i - 1));
		if (FAILED(hr))
			return FALSE;
	}

	hr = spIWICBitmapFrameEncode->Commit();
	if (FAILED(hr))
		return FALSE;
	hr = spIWICBitmapEncoder->Commit();
	if (FAILED(hr))
		return FALSE;
	
	return TRUE;
}

CdemomfcDlg::CdemomfcDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CdemomfcDlg::IDD, pParent), m_hcam(NULL), m_pImageData(NULL)
{
	memset(&m_header, 0, sizeof(m_header));
	m_header.biSize = sizeof(m_header);
	m_header.biPlanes = 1;
	m_header.biBitCount = 24;
}

void CdemomfcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CdemomfcDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, &CdemomfcDlg::OnBnClickedButton1)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CdemomfcDlg::OnCbnSelchangeCombo1)
	ON_MESSAGE(MSG_CAMEVENT, &CdemomfcDlg::OnMsgCamevent)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON2, &CdemomfcDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_CHECK1, &CdemomfcDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON3, &CdemomfcDlg::OnBnClickedButton3)
	ON_WM_HSCROLL()
	ON_COMMAND_RANGE(IDM_SNAP_RESOLUTION, IDM_SNAP_RESOLUTION + TOUPCAM_MAX, &CdemomfcDlg::OnSnapResolution)
END_MESSAGE_MAP()

BOOL CdemomfcDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
	GetDlgItem(IDC_SLIDER1)->EnableWindow(FALSE);
	GetDlgItem(IDC_SLIDER2)->EnableWindow(FALSE);
	GetDlgItem(IDC_SLIDER3)->EnableWindow(FALSE);
	GetDlgItem(IDC_COMBO1)->EnableWindow(FALSE);

	return TRUE;
}

void CdemomfcDlg::OnBnClickedButton1()
{
	if (m_hcam)
		return;

	m_hcam = Toupcam_Open(NULL);
	if (NULL == m_hcam)
	{
		AfxMessageBox(_T("No Device"));
		return;
	}

	CComboBox* pCombox = (CComboBox*)GetDlgItem(IDC_COMBO1);
	pCombox->ResetContent();
	int n = (int)Toupcam_get_ResolutionNumber(m_hcam);
	if (n > 0)
	{
		TCHAR txt[128];
		int nWidth, nHeight;
		for (int i = 0; i < n; ++i)
		{
			Toupcam_get_Resolution(m_hcam, i, &nWidth, &nHeight);
			_stprintf(txt, _T("%d * %d"), nWidth, nHeight);
			pCombox->AddString(txt);
		}

		unsigned nCur = 0;
		Toupcam_get_eSize(m_hcam, &nCur);
		pCombox->SetCurSel(nCur);
	}
	
	StartDevice();
}

void CdemomfcDlg::StartDevice()
{
	int nWidth = 0, nHeight = 0;
	HRESULT hr = Toupcam_get_Size(m_hcam, &nWidth, &nHeight);
	if (FAILED(hr))
		return;

	m_header.biWidth = nWidth;
	m_header.biHeight = nHeight;
	m_header.biSizeImage = TDIBWIDTHBYTES(nWidth * 24) * nHeight;
	if (m_pImageData)
	{
		free(m_pImageData);
		m_pImageData = NULL;
	}
	m_pImageData = malloc(m_header.biSizeImage);

	Toupcam_StartPullModeWithWndMsg(m_hcam, m_hWnd, MSG_CAMEVENT);

	BOOL bEnableAutoExpo = TRUE;
	Toupcam_get_AutoExpoEnable(m_hcam, &bEnableAutoExpo);
	CheckDlgButton(IDC_CHECK1, bEnableAutoExpo ? 1 : 0);
	GetDlgItem(IDC_SLIDER1)->EnableWindow(!bEnableAutoExpo);

	unsigned nMinExpTime, nMaxExpTime, nDefExpTime;
	Toupcam_get_ExpTimeRange(m_hcam, &nMinExpTime, &nMaxExpTime, &nDefExpTime);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetRange(nMinExpTime / 1000, nMaxExpTime / 1000);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER2))->SetRange(TOUPCAM_TEMP_MIN, TOUPCAM_TEMP_MAX);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER3))->SetRange(TOUPCAM_TINT_MIN, TOUPCAM_TINT_MAX);

	OnEventExpo();
	OnEventTempTint();

	GetDlgItem(IDC_BUTTON2)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK1)->EnableWindow(TRUE);
	GetDlgItem(IDC_SLIDER2)->EnableWindow(TRUE);
	GetDlgItem(IDC_SLIDER3)->EnableWindow(TRUE);
	GetDlgItem(IDC_COMBO1)->EnableWindow(TRUE);
}

void CdemomfcDlg::OnCbnSelchangeCombo1()
{
	if (NULL == m_hcam)
		return;

	CComboBox* pCombox = (CComboBox*)GetDlgItem(IDC_COMBO1);
	int nSel = pCombox->GetCurSel();
	if (nSel < 0)
		return;

	unsigned nResolutionIndex = 0;
	HRESULT hr = Toupcam_get_eSize(m_hcam, &nResolutionIndex);
	if (FAILED(hr))
		return;

	if (nResolutionIndex != nSel)
	{
		hr = Toupcam_Stop(m_hcam);
		if (FAILED(hr))
			return;

		Toupcam_put_eSize(m_hcam, nSel);

		StartDevice();
	}
}

LRESULT CdemomfcDlg::OnMsgCamevent(WPARAM wp, LPARAM /*lp*/)
{
	switch (wp)
	{
	case TOUPCAM_EVENT_ERROR:
	case TOUPCAM_EVENT_NOFRAMETIMEOUT:
		OnEventError();
		break;
	case TOUPCAM_EVENT_DISCONNECTED:
		OnEventDisconnected();
		break;
	case TOUPCAM_EVENT_IMAGE:
		OnEventImage();
		break;
	case TOUPCAM_EVENT_EXPOSURE:
		OnEventExpo();
		break;
	case TOUPCAM_EVENT_TEMPTINT:
		OnEventTempTint();
		break;
	case TOUPCAM_EVENT_STILLIMAGE:
		OnEventStillImage();
		break;
	default:
		break;
	}
	return 0;
}

void CdemomfcDlg::OnEventDisconnected()
{
	if (m_hcam)
	{
		Toupcam_Close(m_hcam);
		m_hcam = NULL;
	}
	AfxMessageBox(_T("The camera is disconnected, mybe has been pulled out."));
}

void CdemomfcDlg::OnEventError()
{
	if (m_hcam)
	{
		Toupcam_Close(m_hcam);
		m_hcam = NULL;
	}
	AfxMessageBox(_T("Error"));
}

void CdemomfcDlg::OnEventExpo()
{
	unsigned nTime = 0;
	Toupcam_get_ExpoTime(m_hcam, &nTime);
	SetDlgItemInt(IDC_STATIC1, nTime / 1000, FALSE);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetPos(nTime / 1000);
}

void CdemomfcDlg::OnEventTempTint()
{
	int nTemp = TOUPCAM_TEMP_DEF, nTint = TOUPCAM_TINT_DEF;
	Toupcam_get_TempTint(m_hcam, &nTemp, &nTint);
	SetDlgItemInt(IDC_STATIC2, nTemp, TRUE);
	SetDlgItemInt(IDC_STATIC3, nTint, TRUE);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER2))->SetPos(nTemp);
	((CSliderCtrl*)GetDlgItem(IDC_SLIDER3))->SetPos(nTint);
}

void CdemomfcDlg::OnEventImage()
{
	HRESULT hr = Toupcam_PullImageV2(m_hcam, m_pImageData, 24, NULL);
	if (SUCCEEDED(hr))
	{
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
		StretchDIBits(dc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0, 0, m_header.biWidth, m_header.biHeight, m_pImageData, (BITMAPINFO*)&m_header, DIB_RGB_COLORS, SRCCOPY);
		dc.SetStretchBltMode(m);
	}
}

void CdemomfcDlg::OnEventStillImage()
{
	ToupcamFrameInfoV2 info = { 0 };
	HRESULT hr = Toupcam_PullStillImageV2(m_hcam, NULL, 24, &info);
	if (SUCCEEDED(hr))
	{
		void* pData = malloc(TDIBWIDTHBYTES(info.width * 24) * info.height);
		hr = Toupcam_PullStillImage(m_hcam, pData, 24, NULL, NULL);
		if (SUCCEEDED(hr))
		{
			BITMAPINFOHEADER header = { 0 };
			header.biSize = sizeof(header);
			header.biPlanes = 1;
			header.biBitCount = 24;
			header.biWidth = info.width;
			header.biHeight = info.height;
			header.biSizeImage = TDIBWIDTHBYTES(header.biWidth * header.biBitCount) * header.biHeight;
			SaveImageByWIC(L"demomfc.jpg", pData, &header);
		}
		free(pData);
	}
}

void CdemomfcDlg::OnDestroy()
{
	if (m_hcam)
	{
		Toupcam_Close(m_hcam);
		m_hcam = NULL;
	}
	if (m_pImageData)
	{
		free(m_pImageData);
		m_pImageData = NULL;
	}

	CDialog::OnDestroy();
}

void CdemomfcDlg::OnSnapResolution(UINT nID)
{
	Toupcam_Snap(m_hcam, nID - IDM_SNAP_RESOLUTION);
}

void CdemomfcDlg::OnBnClickedButton2()
{
	int n = Toupcam_get_StillResolutionNumber(m_hcam);
	if (n <= 0)
	{
		unsigned e = 0;
		Toupcam_get_eSize(m_hcam, &e);
		Toupcam_Snap(m_hcam, e);
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
			Toupcam_get_StillResolution(m_hcam, i, &w, &h);
			_stprintf(text, _T("%d * %d"), w, h);
			menu.AppendMenu(MF_STRING, IDM_SNAP_RESOLUTION + i, text);
		}
		menu.TrackPopupMenu(TPM_RIGHTALIGN, pt.x, pt.y, this);
	}
}

void CdemomfcDlg::OnBnClickedCheck1()
{
	if (m_hcam)
		Toupcam_put_AutoExpoEnable(m_hcam, IsDlgButtonChecked(IDC_CHECK1) ? TRUE : FALSE);
	GetDlgItem(IDC_SLIDER1)->EnableWindow(IsDlgButtonChecked(IDC_CHECK1) ? FALSE : TRUE);
}

void CdemomfcDlg::OnBnClickedButton3()
{
	if (m_hcam)
		Toupcam_AwbOnePush(m_hcam, NULL, NULL);
}

void CdemomfcDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == GetDlgItem(IDC_SLIDER1))
	{
		int nTime = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->GetPos();
		SetDlgItemInt(IDC_STATIC1, nTime, TRUE);
		Toupcam_put_ExpoTime(m_hcam, nTime * 1000);
	}
	else
	{
		int nTemp = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER2))->GetPos();
		int nTint = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER3))->GetPos();
		SetDlgItemInt(IDC_STATIC2, nTemp, TRUE);
		SetDlgItemInt(IDC_STATIC3, nTint, TRUE);
		Toupcam_put_TempTint(m_hcam, nTemp, nTint);
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}
