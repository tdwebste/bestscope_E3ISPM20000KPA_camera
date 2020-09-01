#include "stdafx.h"
#include "AutoTestTool.h"
#include "AutoTestToolDlg.h"
#include "afxdialogex.h"
#include "global.h"
#include "CSettingPropertySheet.h"
#include "CExposureGainPropertyPage.h"
#include "CWhiteBalancePropertyPage.h"
#include "CTestPropertySheet.h"
#include <Dbt.h>

CAutoTestToolDlg* g_pMainDlg = nullptr;

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
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory), (LPVOID*)&spIWICImagingFactory);
	if (FAILED(hr))
		return FALSE;

	CComPtr<IWICBitmapEncoder> spIWICBitmapEncoder;
	hr = spIWICImagingFactory->CreateEncoder(guidContainerFormat, nullptr, &spIWICBitmapEncoder);
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

CAutoTestToolDlg::CAutoTestToolDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_AUTOTESTTOOL_DIALOG, pParent), m_pImageData(nullptr), m_pSettingPropertySheet(nullptr)
{
	g_pMainDlg = this;

	memset(&m_header, 0, sizeof(m_header));
	m_header.biSize = sizeof(m_header);
	m_header.biPlanes = 1;
	m_header.biBitCount = 24;
}

void CAutoTestToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_CAMERA_LIST, m_cameraList);
}

BEGIN_MESSAGE_MAP(CAutoTestToolDlg, CDialogEx)
	ON_WM_DEVICECHANGE()
	ON_BN_CLICKED(IDC_BUTTON_START, &CAutoTestToolDlg::OnBnClickedButtonStart)
	ON_MESSAGE(MSG_CAMEVENT, &CAutoTestToolDlg::OnMsgCamevent)
	ON_MESSAGE(WM_USER_PREVIEW_CHANGE, &CAutoTestToolDlg::OnPreviewResChanged)
	ON_MESSAGE(WM_USER_OPEN_CLOSE, &CAutoTestToolDlg::OnCloseOpen)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_SETTING, &CAutoTestToolDlg::OnBnClickedButtonSetting)
	ON_BN_CLICKED(IDC_BUTTON_TEST, &CAutoTestToolDlg::OnBnClickedButtonTest)
	ON_BN_CLICKED(IDC_BUTTON1, &CAutoTestToolDlg::OnBnClickedButton1)
END_MESSAGE_MAP()

BOOL CAutoTestToolDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	EnumCameras();
	return TRUE;  
}

BOOL CAutoTestToolDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	if (DBT_DEVNODES_CHANGED == nEventType)
	{
		EnumCameras();
		if (IsDlgButtonChecked(IDC_CHECK1) && (nullptr == g_hCam) && (g_cameraCnt > 0))
		{
			Sleep(1500);
			OnBnClickedButtonStart();
		}
	}

	return FALSE;
}

LRESULT CAutoTestToolDlg::OnMsgCamevent(WPARAM wp, LPARAM lp)
{
	switch (wp)
	{
	case TOUPCAM_EVENT_ERROR:
	case TOUPCAM_EVENT_NOFRAMETIMEOUT:
	case TOUPCAM_EVENT_DISCONNECTED:
		OnEventError();
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

LRESULT CAutoTestToolDlg::OnPreviewResChanged(WPARAM wp, LPARAM lp)
{
	if (nullptr == g_hCam)
		return -1;

	unsigned nSel = wp;
	unsigned nResolutionIndex = 0;
	HRESULT hr = Toupcam_get_eSize(g_hCam, &nResolutionIndex);
	if (FAILED(hr))
		return -1;

	if (nResolutionIndex != nSel)
	{
		hr = Toupcam_Stop(g_hCam);
		if (FAILED(hr))
			return -1;
		Toupcam_put_eSize(g_hCam, nSel);

		StartCamera();
		UpdateInfo();
	}

	g_bResChangedFinished = true;
	return 0;
}

LRESULT CAutoTestToolDlg::OnCloseOpen(WPARAM wp, LPARAM lp)
{
	OnBnClickedButtonStart();
	g_bOpenCloseFinished = true;
	return 0;
}

void CAutoTestToolDlg::EnumCameras()
{
	g_cameraCnt = Toupcam_EnumV2(g_cameras);
	m_cameraList.ResetContent();
	int index = 0;
	for (int i = 0; i < g_cameraCnt; ++i)
	{
		m_cameraList.AddString(g_cameras[i].displayname);
		if (0 == m_cameraID.Compare(g_cameras[i].id))
			index = i;
	}

	if (g_cameraCnt > 0)
		m_cameraList.SetCurSel(index);

	UpdateButtonsStates();
}

void CAutoTestToolDlg::UpdateButtonsStates()
{
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(g_cameraCnt > 0);
	CString startBtnText;
	GetDlgItemText(IDC_BUTTON_START, startBtnText);
	BOOL bStart = (0 == startBtnText.Compare(_T("Close")));
	GetDlgItem(IDC_BUTTON_SETTING)->EnableWindow(g_cameraCnt > 0 && bStart);
	GetDlgItem(IDC_BUTTON_TEST)->EnableWindow(g_cameraCnt > 0 && bStart);
}

void CAutoTestToolDlg::OnBnClickedButtonStart()
{
	CString startBtnText;
	GetDlgItemText(IDC_BUTTON_START, startBtnText);
	if (0 == startBtnText.Compare(_T("Open")))
	{
		if (g_cameraCnt <= 0)
			return;

		m_cameraID = g_cameras[m_cameraList.GetCurSel()].id;
		m_cameraName = g_cameras[m_cameraList.GetCurSel()].displayname;
		g_hCam = Toupcam_Open(m_cameraID);
		if (nullptr == g_hCam)
			return;

		StartCamera();

		SetDlgItemText(IDC_BUTTON_START, _T("Close"));
		UpdateInfo();
	}
	else
	{
		CloseCamera();

		SetDlgItemText(IDC_BUTTON_START, _T("Open"));
		SetWindowText(_T(""));
	}
	
	UpdateButtonsStates();
}

void CAutoTestToolDlg::StartCamera()
{
	int width = 0, height = 0;
	Toupcam_get_Size(g_hCam, &width, &height);
	m_header.biWidth = width;
	m_header.biHeight = height;
	m_header.biSizeImage = TDIBWIDTHBYTES(width * 24) * height;
	if (m_pImageData)
	{
		free(m_pImageData);
		m_pImageData = nullptr;
	}
	m_pImageData = malloc(m_header.biSizeImage);
	Toupcam_StartPullModeWithWndMsg(g_hCam, m_hWnd, MSG_CAMEVENT);
}

void CAutoTestToolDlg::CloseCamera()
{
	if (g_hCam)
	{
		Toupcam_Close(g_hCam);
		g_hCam = nullptr;
	}

	if (m_pImageData)
	{
		free(m_pImageData);
		m_pImageData = nullptr;
	}
}

void CAutoTestToolDlg::OnEventError()
{
	CloseCamera();
	if (IsDlgButtonChecked(IDC_CHECK1))
		OnBnClickedButton1();

	SetDlgItemText(IDC_BUTTON_START, _T("Open"));
	SetWindowText(_T(""));

	UpdateButtonsStates();
}

void CAutoTestToolDlg::OnEventImage()
{
	ToupcamFrameInfoV2 info = { 0 };
	HRESULT hr = Toupcam_PullImageV2(g_hCam, m_pImageData, 24, &info);
	if (SUCCEEDED(hr))
	{
		if (g_bROITesting && g_bROITest_StartSnap)
		{
			unsigned offsetX = 0, offsetY = 0,  width = 0, height = 0;
			Toupcam_get_Roi(g_hCam, &offsetX, &offsetY, &width, &height);
			if (width == info.width && height == info.height)
			{
				BITMAPINFOHEADER header = { 0 };
				header.biSize = sizeof(header);
				header.biPlanes = 1;
				header.biBitCount = 24;
				header.biWidth = info.width;
				header.biHeight = info.height;
				header.biSizeImage = TDIBWIDTHBYTES(header.biWidth * header.biBitCount) * header.biHeight;
				CString str;
				SYSTEMTIME tm;
				GetLocalTime(&tm);
				str.Format(g_snapDir + _T("\\%d_%dx%d_%04hu%02hu%02hu_%02hu%02hu%02hu_%03hu.jpg"), g_ROITestCount++, info.width, info.height, tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);
				SaveImageByWIC(str, m_pImageData, &header);
				g_bROITest_StartSnap = false;
				g_bROITest_SnapFinished = true;
			}
		}
		else if (g_bTriggerTesting)
		{
			SYSTEMTIME tm;
			GetLocalTime(&tm);
			CString str;
			str.Format(g_snapDir + _T("\\%04hu%02hu%02hu_%02hu%02hu%02hu_%03hu.jpg"), tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);
			SaveImageByWIC(str, m_pImageData, &m_header);
		}
		else if (g_bImageShoot)
		{
			int resWidth = 0, resHeight = 0;
			Toupcam_get_Size(g_hCam, &resWidth, &resHeight);
			CString str;
			SYSTEMTIME tm;
			GetLocalTime(&tm);
			str.Format(g_snapDir + _T("\\%d_%dx%d_%04hu%02hu%02hu_%02hu%02hu%02hu_%03hu.jpg"), g_snapCount, resWidth, resHeight, tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);
			SaveImageByWIC(str, m_pImageData, &m_header);
			if (g_bEnableCheckBlack)
			{
				char* pData = static_cast<char*>(m_pImageData);
				int pitchWidth = TDIBWIDTHBYTES(m_header.biWidth * 24);
				bool bBlack = false;
				int blackCnt = 0;
				for (int i = 0; i < m_header.biHeight; ++i)
				{
					for (int j = 0; j < m_header.biWidth; ++j)
					{
						int value0 = *(pData + i * pitchWidth + j);
						int value1 = *(pData + i * pitchWidth + j + 1);
						int value2 = *(pData + i * pitchWidth + j + 2);
						if (0 == value0 && 0 == value1 && 0 == value2)
						{
							++blackCnt;
							if (blackCnt > m_header.biHeight * m_header.biWidth / 2)
							{
								bBlack = true;
								break;
							}
						}
					}
					if (bBlack)
						break;
				}
				g_bBlack = bBlack;
			}
			g_bImageShoot = false;
		}
		else
		{
			CClientDC dc(this);
			CRect rc;
			GetClientRect(&rc);
			GetDlgItem(IDC_STATIC_IMAGE)->GetWindowRect(&rc);
			ScreenToClient(&rc);

			int m = dc.SetStretchBltMode(COLORONCOLOR);
			StretchDIBits(dc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0, 0,
				m_header.biWidth, m_header.biHeight, m_pImageData, (BITMAPINFO*)&m_header, DIB_RGB_COLORS, SRCCOPY);
			dc.SetStretchBltMode(m);
		}
	}
}

void CAutoTestToolDlg::OnEventExpo()
{
	if (m_pSettingPropertySheet)
	{
		CExposureGainPropertyPage* pPage = m_pSettingPropertySheet->GetExposureGainPropertyPage();
		if (pPage->GetSafeHwnd())
			pPage->PostMessage(WM_USER_AUTO_EXPOSURE);
	}
}

void CAutoTestToolDlg::OnEventTempTint()
{
	if (m_pSettingPropertySheet)
	{
		CWhiteBalancePropertyPage* pPage = m_pSettingPropertySheet->GetWhiteBalancePropertyPage();
		if (pPage->GetSafeHwnd())
			pPage->PostMessage(WM_USER_WHITE_BALANCE);
	}
}

void CAutoTestToolDlg::OnEventStillImage()
{
	ToupcamFrameInfoV2 info = { 0 };
	HRESULT hr = Toupcam_PullStillImageV2(g_hCam, nullptr, 24, &info);
	if (SUCCEEDED(hr))
	{
		void* pData = malloc(TDIBWIDTHBYTES(info.width * 24) * info.height);
		hr = Toupcam_PullStillImage(g_hCam, pData, 24, nullptr, nullptr);
		if (SUCCEEDED(hr))
		{
			BITMAPINFOHEADER header = { 0 };
			header.biSize = sizeof(header);
			header.biPlanes = 1;
			header.biBitCount = 24;
			header.biWidth = info.width;
			header.biHeight = info.height;
			header.biSizeImage = TDIBWIDTHBYTES(header.biWidth * header.biBitCount) * header.biHeight;

			if (g_bSnapTesting)
			{
				int resWidth = 0, resHeight = 0;
				Toupcam_get_Size(g_hCam, &resWidth, &resHeight);
				CString str;
				str.Format(g_snapDir + _T("\\%d_%dx%d_%dx%d.jpg"), g_snapCount, resWidth, resHeight, info.width, info.height);
				SaveImageByWIC(str, pData, &header);
				g_bSnapFinished = true;
			}
			else
			{
				static int index = 1;
				CString str;
				str.Format(_T("autotesttool_%d.jpg"), ++index);
				SaveImageByWIC(str, pData, &header);
			}
		}
		free(pData);
	}
}

void CAutoTestToolDlg::UpdateInfo()
{
	if (g_hCam)
	{
		int width = 0, height = 0;
		Toupcam_get_Size(g_hCam, &width, &height);
		CString str;
		str.Format(_T(" [%d x %d]"), width, height);
		SetWindowText(m_cameraName + str);
	}
}

void CAutoTestToolDlg::OnClose()
{
	CloseCamera();
	CDialogEx::OnClose();
}

void CAutoTestToolDlg::OnBnClickedButtonSetting()
{
	CSettingPropertySheet setting(_T("Settings"));
	m_pSettingPropertySheet = &setting;
	setting.DoModal();
	m_pSettingPropertySheet = nullptr;
}

void CAutoTestToolDlg::OnBnClickedButtonTest()
{
	CTestPropertySheet test(_T("Test"));
	test.DoModal();
}

void CAutoTestToolDlg::OnBnClickedButton1()
{
	if (g_cameraCnt <= 0)
		return;
	Toupcam_Replug(g_cameras[m_cameraList.GetCurSel()].id);
}
