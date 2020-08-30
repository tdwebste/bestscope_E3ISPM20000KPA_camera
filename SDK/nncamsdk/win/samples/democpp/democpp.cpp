#include <windows.h>
#include <atlbase.h>
#include <atlwin.h>
#include <atlapp.h>
CAppModule _Module;
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <atlctrls.h>
#include <atlframe.h>
#include <atlcrack.h>
#include <atldlgs.h>
#include <atlstr.h>
#include "nncam.h"
#include "resource.h"
#include <sstream>
#include <iomanip>
#include <InitGuid.h>
#include <wincodec.h>
#include <wmsdkidl.h>

#define MSG_CAMEVENT			(WM_APP + 1)

class CMainFrame;

static BOOL SaveImageBmp(const wchar_t* strFilename, const void* pData, const BITMAPINFOHEADER* pHeader)
{
	FILE* fp = _wfopen(strFilename, L"wb");
	if (fp)
	{
		BITMAPFILEHEADER fheader = { 0 };
		fheader.bfType = 0x4d42;
		fheader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + pHeader->biSizeImage;
		fheader.bfOffBits = (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
		fwrite(&fheader, sizeof(fheader), 1, fp);
		fwrite(pHeader, 1, sizeof(BITMAPINFOHEADER), fp);
		fwrite(pData, 1, pHeader->biSizeImage, fp);
		fclose(fp);
		return TRUE;
	}
	return FALSE;
}

/* https://docs.microsoft.com/en-us/windows/desktop/wic/-wic-lh */
static BOOL SaveImageByWIC(const wchar_t* strFilename, const void* pData, const BITMAPINFOHEADER* pHeader)
{
	GUID guidContainerFormat;
	if (PathMatchSpec(strFilename, L"*.jpg"))
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

class CExposureTimeDlg : public CDialogImpl<CExposureTimeDlg>
{
	HNnCam	m_hCam;
public:
	enum { IDD = IDD_EXPOSURETIME };

	CExposureTimeDlg(HNnCam hCam)
	: m_hCam(hCam)
	{
	}

	BEGIN_MSG_MAP(CExposureTimeDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());

		unsigned nMin = 0, nMax = 0, nDef = 0, nTime = 0;
		if (SUCCEEDED(Nncam_get_ExpTimeRange(m_hCam, &nMin, &nMax, &nDef)))
		{
			CTrackBarCtrl ctrl(GetDlgItem(IDC_SLIDER1));
			ctrl.SetRangeMin(nMin);
			ctrl.SetRangeMax(nMax);

			if (SUCCEEDED(Nncam_get_ExpoTime(m_hCam, &nTime)))
				ctrl.SetPos(nTime);
		}
		
		return TRUE;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CTrackBarCtrl ctrl(GetDlgItem(IDC_SLIDER1));
		Nncam_put_ExpoTime(m_hCam, ctrl.GetPos());

		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CSpeedDlg : public CDialogImpl<CSpeedDlg>
{
	HNnCam	m_hCam;
public:
	enum { IDD = IDD_SPEED };

	CSpeedDlg(HNnCam hCam)
		: m_hCam(hCam)
	{
	}

	BEGIN_MSG_MAP(CSpeedDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());

		CTrackBarCtrl ctrl(GetDlgItem(IDC_SLIDER1));
		ctrl.SetRangeMin(0);
		ctrl.SetRangeMax(Nncam_get_MaxSpeed(m_hCam));

		unsigned short nSpeed = 0;
		if (SUCCEEDED(Nncam_get_Speed(m_hCam, &nSpeed)))
			ctrl.SetPos(nSpeed);

		return TRUE;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CTrackBarCtrl ctrl(GetDlgItem(IDC_SLIDER1));
		Nncam_put_Speed(m_hCam, ctrl.GetPos());

		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CMaxAEDlg : public CDialogImpl<CMaxAEDlg>
{
	HNnCam	m_hCam;
public:
	enum { IDD = IDD_MAXAE };

	CMaxAEDlg(HNnCam hCam)
		: m_hCam(hCam)
	{
	}

	BEGIN_MSG_MAP(CMaxAEDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());
		SetDlgItemInt(IDC_EDIT1, 350000);
		SetDlgItemInt(IDC_EDIT2, 500);
		return TRUE;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bTran1 = FALSE, bTran2 = FALSE;
		UINT nTime = GetDlgItemInt(IDC_EDIT1, &bTran1, FALSE);
		UINT nGain = GetDlgItemInt(IDC_EDIT2, &bTran2, FALSE);
		if (bTran1 && bTran2)
			Nncam_put_MaxAutoExpoTimeAGain(m_hCam, nTime, nGain);
		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CLedDlg : public CDialogImpl<CLedDlg>
{
	HNnCam	m_hCam;
public:
	enum { IDD = IDD_LED };

	CLedDlg(HNnCam hCam)
	: m_hCam(hCam)
	{
	}

	BEGIN_MSG_MAP(CLedDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_BUTTON1, OnButton1)
		COMMAND_ID_HANDLER(IDC_BUTTON2, OnButton2)
		COMMAND_ID_HANDLER(IDC_BUTTON3, OnButton3)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT OnButton1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		UINT nIndex = GetDlgItemInt(IDC_EDIT1);
		Nncam_put_LEDState(m_hCam, nIndex, 1, 0);
		return 0;
	}

	LRESULT OnButton2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		UINT nIndex = GetDlgItemInt(IDC_EDIT1);
		UINT nPeriod = GetDlgItemInt(IDC_EDIT2);
		Nncam_put_LEDState(m_hCam, nIndex, 2, nPeriod);
		return 0;
	}

	LRESULT OnButton3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		UINT nIndex = GetDlgItemInt(IDC_EDIT1);
		Nncam_put_LEDState(m_hCam, nIndex, 0, 0);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CPixelFormatDlg : public CDialogImpl<CPixelFormatDlg>
{
	const NncamInstV2&	m_inst;
	HNnCam				m_hCam;
public:
	enum { IDD = IDD_PIXELFORMAT };

	CPixelFormatDlg(const NncamInstV2& inst, HNnCam hCam)
	: m_inst(inst), m_hCam(hCam)
	{
	}

	BEGIN_MSG_MAP(CPixelFormatDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_RADIO1, OnRadio1)
		COMMAND_ID_HANDLER(IDC_RADIO2, OnRadio2)
		COMMAND_ID_HANDLER(IDC_RADIO3, OnRadio3)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());

		if (0 == (m_inst.model->flag & (NNCAM_FLAG_RAW8 | NNCAM_FLAG_GMCY8)))
			GetDlgItem(IDC_RADIO1).EnableWindow(FALSE);
		
		if (m_inst.model->flag & (NNCAM_FLAG_RAW10 | NNCAM_FLAG_RAW12 | NNCAM_FLAG_RAW14 | NNCAM_FLAG_RAW16 | NNCAM_FLAG_GMCY12))
		{
			TCHAR str[16];
			unsigned bits = 16;
			if (m_inst.model->flag & NNCAM_FLAG_RAW10)
				bits = 10;
			else if (m_inst.model->flag & (NNCAM_FLAG_RAW12 | NNCAM_FLAG_GMCY12))
				bits = 12;
			else if (m_inst.model->flag & NNCAM_FLAG_RAW14)
				bits = 14;
			_stprintf(str, _T("RAW%u"), bits);
			GetDlgItem(IDC_RADIO2).SetWindowText(str);
		}
		else
		{
			GetDlgItem(IDC_RADIO2).EnableWindow(FALSE);
		}

		if (0 == (m_inst.model->flag & (NNCAM_FLAG_VUYY | NNCAM_FLAG_UYVY)))
			GetDlgItem(IDC_RADIO3).EnableWindow(FALSE);

		int val = 0;
		Nncam_get_Option(m_hCam, NNCAM_OPTION_PIXEL_FORMAT, &val);
		if ((NNCAM_PIXELFORMAT_RAW8 == val) || (NNCAM_PIXELFORMAT_GMCY8 == val))
			CheckDlgButton(IDC_RADIO1, 1);
		else if ((NNCAM_PIXELFORMAT_RAW10 == val) || (NNCAM_PIXELFORMAT_RAW12 == val) || (NNCAM_PIXELFORMAT_RAW14 == val) || (NNCAM_PIXELFORMAT_RAW16 == val) || (NNCAM_PIXELFORMAT_GMCY12 == val))
			CheckDlgButton(IDC_RADIO2, 1);
		else if ((NNCAM_PIXELFORMAT_VUYY == val) || (NNCAM_PIXELFORMAT_UYVY == val))
			CheckDlgButton(IDC_RADIO3, 1);
		return TRUE;
	}

	LRESULT OnRadio1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (m_inst.model->flag & NNCAM_FLAG_RAW8)
			Nncam_put_Option(m_hCam, NNCAM_OPTION_PIXEL_FORMAT, NNCAM_PIXELFORMAT_RAW8);
		else if (m_inst.model->flag & NNCAM_FLAG_GMCY8)
			Nncam_put_Option(m_hCam, NNCAM_OPTION_PIXEL_FORMAT, NNCAM_PIXELFORMAT_GMCY8);
		return 0;
	}

	LRESULT OnRadio2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (m_inst.model->flag & NNCAM_FLAG_RAW10)
			Nncam_put_Option(m_hCam, NNCAM_OPTION_PIXEL_FORMAT, NNCAM_PIXELFORMAT_RAW10);
		else if (m_inst.model->flag & NNCAM_FLAG_RAW12)
			Nncam_put_Option(m_hCam, NNCAM_OPTION_PIXEL_FORMAT, NNCAM_PIXELFORMAT_RAW12);
		else if (m_inst.model->flag & NNCAM_FLAG_RAW14)
			Nncam_put_Option(m_hCam, NNCAM_OPTION_PIXEL_FORMAT, NNCAM_PIXELFORMAT_RAW14);
		else if (m_inst.model->flag & NNCAM_FLAG_RAW16)
			Nncam_put_Option(m_hCam, NNCAM_OPTION_PIXEL_FORMAT, NNCAM_PIXELFORMAT_RAW16);
		else if (m_inst.model->flag & NNCAM_FLAG_GMCY12)
			Nncam_put_Option(m_hCam, NNCAM_OPTION_PIXEL_FORMAT, NNCAM_PIXELFORMAT_GMCY12);
		return 0;
	}

	LRESULT OnRadio3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (m_inst.model->flag & NNCAM_FLAG_VUYY)
			Nncam_put_Option(m_hCam, NNCAM_OPTION_PIXEL_FORMAT, NNCAM_PIXELFORMAT_VUYY);
		else if (m_inst.model->flag & NNCAM_FLAG_UYVY)
			Nncam_put_Option(m_hCam, NNCAM_OPTION_PIXEL_FORMAT, NNCAM_PIXELFORMAT_UYVY);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CRoiDlg : public CDialogImpl<CRoiDlg>
{
public:
	enum { IDD = IDD_ROI };

	unsigned xOffset_;
	unsigned yOffset_;
	unsigned xWidth_;
	unsigned yHeight_;

	CRoiDlg()
		: xOffset_(0), yOffset_(0), xWidth_(0), yHeight_(0)
	{
	}

	BEGIN_MSG_MAP(CRoiDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());

		SetDlgItemInt(IDC_EDIT1, xOffset_);
		SetDlgItemInt(IDC_EDIT2, yOffset_);
		SetDlgItemInt(IDC_EDIT3, xWidth_);
		SetDlgItemInt(IDC_EDIT4, yHeight_);
		return TRUE;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		xOffset_ = GetDlgItemInt(IDC_EDIT1);
		yOffset_ = GetDlgItemInt(IDC_EDIT2);
		xWidth_ = GetDlgItemInt(IDC_EDIT3);
		yHeight_ = GetDlgItemInt(IDC_EDIT4);

		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CTriggerNumberDlg : public CDialogImpl<CTriggerNumberDlg>
{
public:
	enum { IDD = IDD_TRIGGERNUMBER };

	unsigned short number_;

	CTriggerNumberDlg() : number_(1)
	{
	}

	BEGIN_MSG_MAP(CTriggerNumberDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_CHECK1, BN_CLICKED, OnClickCheck1)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());

		if (0xffff == number_)
		{
			CheckDlgButton(IDC_CHECK1, 1);
			GetDlgItem(IDC_EDIT1).EnableWindow(FALSE);
		}
		SetDlgItemInt(IDC_EDIT1, number_);
		return TRUE;
	}

	LRESULT OnClickCheck1(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		GetDlgItem(IDC_EDIT1).EnableWindow(!IsDlgButtonChecked(IDC_CHECK1));
		return 0;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (IsDlgButtonChecked(IDC_CHECK1))
			number_ = 0xffff;
		else
		{
			number_ = GetDlgItemInt(IDC_EDIT1);
			if ((0 == number_) || (number_ >= SHRT_MAX))
			{
				AtlMessageBox(m_hWnd, L"Invalid number");
				return 0;
			}
		}

		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CTECTargetDlg : public CDialogImpl<CTECTargetDlg>
{
	HNnCam	m_hCam;
public:
	enum { IDD = IDD_TECTARGET };

	CTECTargetDlg(HNnCam hCam)
		: m_hCam(hCam)
	{
	}

	BEGIN_MSG_MAP(CTECTargetDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());

		int val = 0;
		Nncam_get_Option(m_hCam, NNCAM_OPTION_TECTARGET, &val);

		TCHAR str[256];
		_stprintf(str, _T("%d.%d"), val / 10, val % 10);
		SetDlgItemText(IDC_EDIT1, str);
		return TRUE;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CString str;
		GetDlgItemText(IDC_EDIT1, str);
		TCHAR* endptr;
		double d = _tcstod((LPCTSTR)str, &endptr);
		Nncam_put_Option(m_hCam, NNCAM_OPTION_TECTARGET, (int)(d * 10));

		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CSnapnDlg : public CDialogImpl<CSnapnDlg>
{
public:
	enum { IDD = IDD_SNAPN };

	unsigned m_nNum;
	CSnapnDlg() : m_nNum(3)
	{
	}

	BEGIN_MSG_MAP(CSnapnDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());
		SetDlgItemInt(IDC_EDIT1, m_nNum);
		return TRUE;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bTrans;
		m_nNum = GetDlgItemInt(IDC_EDIT1, &bTrans, FALSE);
		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CEEPROMDlg : public CDialogImpl<CEEPROMDlg>
{
	HNnCam	m_hCam;
public:
	enum { IDD = IDD_EEPROM };

	CEEPROMDlg(HNnCam hCam)
	: m_hCam(hCam)
	{
	}

	BEGIN_MSG_MAP(CEEPROMDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_BUTTON1, OnButton1)
		COMMAND_ID_HANDLER(IDC_BUTTON2, OnButton2)
		COMMAND_ID_HANDLER(IDC_BUTTON3, OnButton3)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT OnButton1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		TCHAR strAddr[64] = { 0 }, strLength[64] = { 0 };
		if (GetDlgItemText(IDC_EDIT1, strAddr, _countof(strAddr)) && GetDlgItemText(IDC_EDIT2, strLength, _countof(strLength)))
		{
			TCHAR* endptr = NULL;
			unsigned uAddr = _tcstoul(strAddr, &endptr, 16);
			unsigned uLength = _tcstoul(strLength, &endptr, 16);
			if (uLength)
			{
				unsigned char* tmpBuffer = (unsigned char*)alloca(uLength);
				HRESULT hr = Nncam_read_EEPROM(m_hCam, uAddr, tmpBuffer, uLength);
				if (FAILED(hr))
					AtlMessageBox(m_hWnd, _T("Failed to read EEPROM."));
				else if (0 == hr)
					AtlMessageBox(m_hWnd, _T("Read EEPROM, 0 byte."));
				else if (hr > 0)
				{
					std::wstringstream wstr;
					wstr << _T("EEPROM: length = ") << hr << _T(", data = ");
					for (int i = 0; i < hr; ++i)
						wstr << std::hex << std::setw(2) << std::setfill((TCHAR)'0') << tmpBuffer[i] << _T(" ");
					AtlMessageBox(m_hWnd, wstr.str().c_str());
				}
			}
		}
		return 0;
	}

	LRESULT OnButton2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		TCHAR strAddr[64] = { 0 }, strLength[64] = { 0 }, strData[1024] = { 0 };
		if (GetDlgItemText(IDC_EDIT1, strAddr, _countof(strAddr)) && GetDlgItemText(IDC_EDIT2, strLength, _countof(strLength)) && GetDlgItemText(IDC_EDIT3, strData, _countof(strData)))
		{
			TCHAR* endptr = NULL;
			unsigned uAddr = _tcstoul((LPCTSTR)strAddr, &endptr, 16);
			unsigned uLength = _tcstoul((LPCTSTR)strLength, &endptr, 16);
			if (uLength)
			{
				unsigned char* tmpBuffer = (unsigned char*)alloca(uLength);
				memset(tmpBuffer, 0, uLength);
				for (size_t i = 0; i < _countof(strData); i += 2)
				{
					if ('\0' == strData[0])
						break;
					if (strData[i] >= '0' && strData[i] <= '9')
						tmpBuffer[i / 2] = (strData[i] - '0') << 4;
					else if (strData[i] >= 'a' && strData[i] <= 'f')
						tmpBuffer[i / 2] = (strData[i] - 'a' + 10) << 4;
					else if (strData[i] >= 'A' && strData[i] <= 'F')
						tmpBuffer[i / 2] = (strData[i] - 'A' + 10) << 4;
					if (strData[i + 1] >= '0' && strData[i + 1] <= '9')
						tmpBuffer[i / 2] |= (strData[i + 1] - '0');
					else if (strData[i + 1] >= 'a' && strData[i + 1] <= 'f')
						tmpBuffer[i / 2] |= (strData[i + 1] - 'a' + 10);
					else if (strData[i + 1] >= 'A' && strData[i + 1] <= 'F')
						tmpBuffer[i / 2] |= (strData[i + 1] - 'A' + 10);
				}
				HRESULT hr = Nncam_write_EEPROM(m_hCam, uAddr, tmpBuffer, uLength);
				TCHAR strMessage[256];
				_stprintf(strMessage, _T("Write EEPROM, length = %u, result = 0x%08x"), uLength, hr);
				AtlMessageBox(m_hWnd, strMessage);
			}
		}
		return 0;
	}

	LRESULT OnButton3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		TCHAR strLength[64] = { 0 };
		if (GetDlgItemText(IDC_EDIT2, strLength, _countof(strLength)))
		{
			TCHAR* endptr = NULL;
			unsigned uLength = _tcstoul((LPCTSTR)strLength, &endptr, 16);
			if (uLength)
			{
				unsigned char* tmpWriteBuffer = (unsigned char*)alloca(uLength);
				unsigned char* tmpReadBuffer = (unsigned char*)alloca(uLength);
				HRESULT hr1 = Nncam_write_EEPROM(m_hCam, 0, tmpWriteBuffer, uLength);
				HRESULT hr2 = Nncam_read_EEPROM(m_hCam, 0, tmpReadBuffer, uLength);
				if ((hr1 == uLength) && (hr2 == uLength))
				{
					if (0 == memcmp(tmpWriteBuffer, tmpReadBuffer, uLength))
					{
						AtlMessageBox(m_hWnd, _T("Test OK"));
						return 0;
					}
				}

				AtlMessageBox(m_hWnd, _T("Test Failed"));
			}
		}
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CUARTDlg : public CDialogImpl<CUARTDlg>
{
	HNnCam	m_hCam;
public:
	enum { IDD = IDD_UART };

	CUARTDlg(HNnCam hCam)
		: m_hCam(hCam)
	{
	}

	BEGIN_MSG_MAP(CUARTDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_BUTTON1, OnButton1)
		COMMAND_ID_HANDLER(IDC_BUTTON2, OnButton2)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT OnButton1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		TCHAR strLength[64] = { 0 };
		if (GetDlgItemText(IDC_EDIT2, strLength, _countof(strLength)))
		{
			TCHAR* endptr = NULL;
			unsigned uLength = _tcstoul(strLength, &endptr, 16);
			if (uLength)
			{
				unsigned char* tmpBuffer = (unsigned char*)alloca(uLength);
				HRESULT hr = Nncam_read_UART(m_hCam, tmpBuffer, uLength);
				if (FAILED(hr))
					AtlMessageBox(m_hWnd, _T("Failed to read UART."));
				else if (0 == hr)
					AtlMessageBox(m_hWnd, _T("Read UART, 0 byte."));
				else if (hr > 0)
				{
					std::wstringstream wstr;
					wstr << _T("UART: length = ") << hr << _T(", data = ");
					for (int i = 0; i < hr; ++i)
						wstr << std::hex << std::setw(2) << std::setfill((TCHAR)'0') << tmpBuffer[i] << _T(" ");
					AtlMessageBox(m_hWnd, wstr.str().c_str());
				}
			}
		}
		return 0;
	}

	LRESULT OnButton2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		TCHAR strLength[64] = { 0 }, strData[1024] = { 0 };
		if (GetDlgItemText(IDC_EDIT2, strLength, _countof(strLength)) && GetDlgItemText(IDC_EDIT3, strData, _countof(strData)))
		{
			TCHAR* endptr = NULL;
			unsigned uLength = _tcstoul((LPCTSTR)strLength, &endptr, 16);
			if (uLength)
			{
				unsigned char* tmpBuffer = (unsigned char*)alloca(uLength);
				memset(tmpBuffer, 0, uLength);
				for (size_t i = 0; i < _countof(strData); i += 2)
				{
					if ('\0' == strData[0])
						break;
					if (strData[i] >= '0' && strData[i] <= '9')
						tmpBuffer[i / 2] = (strData[i] - '0') << 4;
					else if (strData[i] >= 'a' && strData[i] <= 'f')
						tmpBuffer[i / 2] = (strData[i] - 'a' + 10) << 4;
					else if (strData[i] >= 'A' && strData[i] <= 'F')
						tmpBuffer[i / 2] = (strData[i] - 'A' + 10) << 4;
					if (strData[i + 1] >= '0' && strData[i + 1] <= '9')
						tmpBuffer[i / 2] |= (strData[i + 1] - '0');
					else if (strData[i + 1] >= 'a' && strData[i + 1] <= 'f')
						tmpBuffer[i / 2] |= (strData[i + 1] - 'a' + 10);
					else if (strData[i + 1] >= 'A' && strData[i + 1] <= 'F')
						tmpBuffer[i / 2] |= (strData[i + 1] - 'A' + 10);
				}
				HRESULT hr = Nncam_write_UART(m_hCam, tmpBuffer, uLength);
				TCHAR strMessage[256];
				_stprintf(strMessage, _T("Write UART, length = %u, result = 0x%08x"), uLength, hr);
				AtlMessageBox(m_hWnd, strMessage);
			}
		}
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

class CIocontrolDlg : public CDialogImpl<CIocontrolDlg>
{
	HNnCam				m_hCam;
	const NncamInstV2&	m_inst;
public:
	enum { IDD = IDD_IOCONTROL };

	CIocontrolDlg(HNnCam hCam, const NncamInstV2& inst)
		: m_hCam(hCam), m_inst(inst)
	{
	}

	BEGIN_MSG_MAP(CIocontrolDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDC_COUNTERRESET, OnCounterReset)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());

		{
			CComboBox box(GetDlgItem(IDC_IOINDEX));
			box.AddString(_T("Isolated input"));
			box.AddString(_T("Isolated output"));
			box.AddString(_T("GPIO0"));
			box.AddString(_T("GPIO1"));
			box.SetCurSel(0);
		}
		{
			CComboBox box(GetDlgItem(IDC_GPIODIR));
			box.AddString(_T("Input"));
			box.AddString(_T("Output"));
			box.SetCurSel(0);
		}
		{
			CComboBox box(GetDlgItem(IDC_IOFORMAT));
			box.AddString(_T("Not connected"));
			box.AddString(_T("Tri-state"));
			box.AddString(_T("TTL"));
			box.AddString(_T("LVDS"));
			box.AddString(_T("RS422"));
			box.AddString(_T("Opto-coupled"));
			box.SetCurSel(5);
		}
		{
			CComboBox box(GetDlgItem(IDC_OUTPUTINVERTER));
			box.AddString(_T("No"));
			box.AddString(_T("Yes"));
			box.SetCurSel(0);
		}
		{
			CComboBox box(GetDlgItem(IDC_INPUTACTIVATION));
			box.AddString(_T("Positive"));
			box.AddString(_T("Negative"));
			box.SetCurSel(0);
		}
		{
			CComboBox box(GetDlgItem(IDC_TRIGGERSOURCE));
			box.AddString(_T("Isolated input"));
			box.AddString(_T("GPIO0"));
			box.AddString(_T("GPIO1"));
			box.AddString(_T("Counter"));
			box.AddString(_T("PWM"));
			box.AddString(_T("Software"));
			box.SetCurSel(0);
		}
		{
			CComboBox box(GetDlgItem(IDC_COUNTERSOURCE));
			box.AddString(_T("Isolated input"));
			box.AddString(_T("GPIO0"));
			box.AddString(_T("GPIO1"));
			box.SetCurSel(0);
		}
		{
			CComboBox box(GetDlgItem(IDC_PWMSOURCE));
			box.AddString(_T("Isolated input"));
			box.AddString(_T("GPIO0"));
			box.AddString(_T("GPIO1"));
			box.SetCurSel(0);
		}
		{
			CComboBox box(GetDlgItem(IDC_OUTPUTMODE));
			box.AddString(_T("Frame Trigger Wait"));
			box.AddString(_T("Exposure Active"));
			box.AddString(_T("Strobe")); 
			box.AddString(_T("User Output"));
			box.SetCurSel(0);
		}
		{
			CComboBox box(GetDlgItem(IDC_STROBEDELAYMODE));
			box.AddString(_T("pre-delay"));
			box.AddString(_T("delay"));
			box.SetCurSel(1);
		}
		SetDlgItemInt(IDC_DEBOUNCE_TIME, 0, TRUE);
		SetDlgItemInt(IDC_TRIGGER_DELAY, 0, TRUE);
		SetDlgItemInt(IDC_COUNTER_VALUE, 0, TRUE);
		SetDlgItemInt(IDC_STROBE_DELAY_TIME, 0, TRUE);
		SetDlgItemInt(IDC_STROBE_DURATION, 0, TRUE);
		SetDlgItemInt(IDC_USER_VALUE, 0, TRUE);
		GetDlgItem(IDC_GPIODIR).EnableWindow(FALSE);
		GetDlgItem(IDC_IOFORMAT).EnableWindow(FALSE);
		GetDlgItem(IDC_OUTPUTMODE).EnableWindow(FALSE); 
		GetDlgItem(IDC_OUTPUTINVERTER).EnableWindow(FALSE);
		return TRUE;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}

	LRESULT OnCounterReset(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Nncam_IoControl(m_hCam, 0, NNCAM_IOCONTROLTYPE_SET_RESETCOUNTER, 0, NULL);
		return 0;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		unsigned index = 0;
		{
			CComboBox box(GetDlgItem(IDC_IOINDEX));
			index = box.GetCurSel();
		}
		{
			CComboBox box(GetDlgItem(IDC_GPIODIR));
			if (0 == box.GetCurSel())
				Nncam_IoControl(m_hCam, index, NNCAM_IOCONTROLTYPE_SET_GPIODIR, 0x00, NULL);
			else
				Nncam_IoControl(m_hCam, index, NNCAM_IOCONTROLTYPE_SET_GPIODIR, 0x01, NULL);
		}
		{
			CComboBox box(GetDlgItem(IDC_OUTPUTINVERTER));
			Nncam_IoControl(m_hCam, index, NNCAM_IOCONTROLTYPE_SET_OUTPUTINVERTER, box.GetCurSel(), NULL);
		}
		{
			CComboBox box(GetDlgItem(IDC_INPUTACTIVATION));
			Nncam_IoControl(m_hCam, index, NNCAM_IOCONTROLTYPE_SET_INPUTACTIVATION, box.GetCurSel(), NULL);
		}
		{
			CString str;
			GetDlgItemText(IDC_STROBE_DURATION, str);
			str.Trim();
			if (!str.IsEmpty())
			{
				int val = _ttoi((LPCTSTR)str);
				Nncam_IoControl(m_hCam, 0, NNCAM_IOCONTROLTYPE_SET_STROBEDURATION, val, NULL);
			}
		}
		{
			CString str;
			GetDlgItemText(IDC_DEBOUNCE_TIME, str);
			str.Trim();
			if (!str.IsEmpty())
			{
				int val = _ttoi((LPCTSTR)str);
				Nncam_IoControl(m_hCam, index, NNCAM_IOCONTROLTYPE_SET_DEBOUNCERTIME, val, NULL);
			}
		}
		{
			CString str;
			GetDlgItemText(IDC_COUNTER_VALUE, str);
			str.Trim();
			if (!str.IsEmpty())
			{
				int val = _ttoi((LPCTSTR)str);
				Nncam_IoControl(m_hCam, 0, NNCAM_IOCONTROLTYPE_SET_COUNTERVALUE, val, NULL);
			}
		}
		{
			CComboBox box(GetDlgItem(IDC_OUTPUTMODE));
			Nncam_IoControl(m_hCam, index, NNCAM_IOCONTROLTYPE_SET_OUTPUTMODE, box.GetCurSel(), NULL);
		}
		{
			CString str;
			GetDlgItemText(IDC_USER_VALUE, str);
			str.Trim();
			if (!str.IsEmpty())
			{
				int val = _ttoi((LPCTSTR)str);
				Nncam_IoControl(m_hCam, 0, NNCAM_IOCONTROLTYPE_SET_USERVALUE, val, NULL);
			}
		}
		{
			CComboBox box(GetDlgItem(IDC_STROBEDELAYMODE));
			Nncam_IoControl(m_hCam, 0, NNCAM_IOCONTROLTYPE_SET_STROBEDELAYMODE, box.GetCurSel(), NULL);
		}
		{
			CString str;
			GetDlgItemText(IDC_STROBE_DELAY_TIME, str);
			str.Trim();
			if (!str.IsEmpty())
			{
				int val = _ttoi((LPCTSTR)str);
				Nncam_IoControl(m_hCam, 0, NNCAM_IOCONTROLTYPE_SET_STROBEDELAYTIME, val, NULL);
			}
		}
		{
			CString str;
			GetDlgItemText(IDC_TRIGGER_DELAY, str);
			str.Trim();
			if (!str.IsEmpty())
			{
				int val = _ttoi((LPCTSTR)str);
				Nncam_IoControl(m_hCam, 0, NNCAM_IOCONTROLTYPE_SET_TRIGGERDELAY, val, NULL);
			}
		}
		{
			CComboBox box(GetDlgItem(IDC_PWMSOURCE));
			Nncam_IoControl(m_hCam, index, NNCAM_IOCONTROLTYPE_SET_PWMSOURCE, box.GetCurSel(), NULL);
		}
		return 0;
	}
};

class CMainView : public CWindowImpl<CMainView>
{
	CMainFrame*	m_pMainFrame;
	LONG		m_nOldWidth, m_nOldHeight;

public:

	static ATL::CWndClassInfo& GetWndClassInfo()
	{
		static ATL::CWndClassInfo wc =
		{
			{ sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, StartWindowProc,
			  0, 0, NULL, NULL, NULL, (HBRUSH)NULL_BRUSH, NULL, NULL, NULL },
			NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
		};
		return wc;
	}

	CMainView(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame), m_nOldWidth(0), m_nOldHeight(0)
	{
	}

	BEGIN_MSG_MAP(CMainView)
		MESSAGE_HANDLER(WM_PAINT, OnWmPaint)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
	END_MSG_MAP()

	LRESULT OnWmPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 1;
	}
};

class CWmvRecord
{
	const LONG				m_lFrameWidth, m_lFrameHeight;
	CComPtr<IWMWriter>		m_spIWMWriter;
public:
	CWmvRecord(LONG lFrameWidth, LONG lFrameHeight)
	: m_lFrameWidth(lFrameWidth), m_lFrameHeight(lFrameHeight)
	{
	}

	BOOL StartRecord(const wchar_t* strFilename, DWORD dwBitrate)
	{
		CComPtr<IWMProfileManager> spIWMProfileManager;
		HRESULT hr = WMCreateProfileManager(&spIWMProfileManager);
		if (FAILED(hr))
			return FALSE;

		CComPtr<IWMCodecInfo> spIWMCodecInfo;
		hr = spIWMProfileManager->QueryInterface(__uuidof(IWMCodecInfo), (void**)&spIWMCodecInfo);
		if (FAILED(hr))
			return FALSE;

		DWORD cCodecs = 0;
        hr = spIWMCodecInfo->GetCodecInfoCount(WMMEDIATYPE_Video, &cCodecs);
		if (FAILED(hr))
			return FALSE;

		CComPtr<IWMStreamConfig> spIWMStreamConfig;
        //
        // Search from the last codec because the last codec usually 
        // is the newest codec. 
        //
        for (int i = cCodecs - 1; i >= 0; i--)
        {
            DWORD cFormats;
            hr = spIWMCodecInfo->GetCodecFormatCount(WMMEDIATYPE_Video, i, &cFormats);
			if (FAILED(hr))
				break;

            for (DWORD j = 0; j < cFormats; j++)
			{
                hr = spIWMCodecInfo->GetCodecFormat(WMMEDIATYPE_Video, i, j, &spIWMStreamConfig);
				if (FAILED(hr))
					break;

				hr = ConfigureInput(spIWMStreamConfig, dwBitrate);
				if (SUCCEEDED(hr))
					break;
				spIWMStreamConfig = NULL;
			}
			if (spIWMStreamConfig)
				break;
		}
		if (spIWMStreamConfig == NULL)
			return FALSE;

		CComPtr<IWMProfile> spIWMProfile;
		hr = spIWMProfileManager->CreateEmptyProfile(WMT_VER_8_0, &spIWMProfile);
		if (FAILED(hr))
			return FALSE;

		{
			CComPtr<IWMStreamConfig> spIWMStreamConfig2;
			hr = spIWMProfile->CreateNewStream(WMMEDIATYPE_Video, &spIWMStreamConfig2);
			if (FAILED(hr))
				return FALSE;

			WORD wStreamNum = 1;
			hr = spIWMStreamConfig2->GetStreamNumber(&wStreamNum);
			if (FAILED(hr))
				return FALSE;
			
			hr = spIWMStreamConfig->SetStreamNumber(wStreamNum);
			if (FAILED(hr))
				return FALSE;
		}
		spIWMStreamConfig->SetBitrate(dwBitrate);

		hr = spIWMProfile->AddStream(spIWMStreamConfig);
		if (FAILED(hr))
			return FALSE;

		hr = WMCreateWriter(NULL, &m_spIWMWriter);
		if (FAILED(hr))
			return FALSE;

		hr = m_spIWMWriter->SetProfile(spIWMProfile);
		if (FAILED(hr))
			return FALSE;

		hr = SetInputProps();
		if (FAILED(hr))
			return FALSE;

		hr = m_spIWMWriter->SetOutputFilename(strFilename);
		if (FAILED(hr))
			return FALSE;

		hr = m_spIWMWriter->BeginWriting();
		if (FAILED(hr))
			return FALSE;

		{
			CComPtr<IWMWriterAdvanced> spIWMWriterAdvanced;
			m_spIWMWriter->QueryInterface(__uuidof(IWMWriterAdvanced), (void**)&spIWMWriterAdvanced);
			if (spIWMWriterAdvanced)
				spIWMWriterAdvanced->SetLiveSource(TRUE);
		}

		return TRUE;
	}

	void StopRecord()
	{
		if (m_spIWMWriter)
		{
			m_spIWMWriter->Flush();
			m_spIWMWriter->EndWriting();
			m_spIWMWriter = NULL;
		}
	}

	BOOL WriteSample(const void* pData)
	{
		CComPtr<INSSBuffer> spINSSBuffer;
		if (SUCCEEDED(m_spIWMWriter->AllocateSample(TDIBWIDTHBYTES(m_lFrameWidth * 24) * m_lFrameHeight, &spINSSBuffer)))
		{
			BYTE* pBuffer = NULL;
			if (SUCCEEDED(spINSSBuffer->GetBuffer(&pBuffer)))
			{
				memcpy(pBuffer, pData, TDIBWIDTHBYTES(m_lFrameWidth * 24) * m_lFrameHeight);
				spINSSBuffer->SetLength(TDIBWIDTHBYTES(m_lFrameWidth * 24) * m_lFrameHeight);
				QWORD cnsSampleTime = GetTickCount();
				m_spIWMWriter->WriteSample(0, cnsSampleTime * 1000 * 10, 0, spINSSBuffer);
				return TRUE;
			}
		}

		return FALSE;
	}

private:
	HRESULT SetInputProps()
	{
		DWORD dwForamts = 0;
		HRESULT hr = m_spIWMWriter->GetInputFormatCount(0, &dwForamts);
		if (FAILED(hr))
			return hr;

		for (DWORD i = 0; i < dwForamts; ++i)
		{
			CComPtr<IWMInputMediaProps> spIWMInputMediaProps;
			hr = m_spIWMWriter->GetInputFormat(0, i, &spIWMInputMediaProps);
			if (FAILED(hr))
				return hr;

			DWORD cbSize = 0;
			hr = spIWMInputMediaProps->GetMediaType(NULL, &cbSize);
			if (FAILED(hr))
				return hr;

			WM_MEDIA_TYPE* pMediaType = (WM_MEDIA_TYPE*)alloca(cbSize);
			hr = spIWMInputMediaProps->GetMediaType(pMediaType, &cbSize);
			if (FAILED(hr))
				return hr;

			if (pMediaType->subtype == WMMEDIASUBTYPE_RGB24)
			{
				hr = spIWMInputMediaProps->SetMediaType(pMediaType);
				if (FAILED(hr))
					return hr;

				return m_spIWMWriter->SetInputProps(0, spIWMInputMediaProps);
			}
		}

		return E_FAIL;
	}

	HRESULT ConfigureInput(CComPtr<IWMStreamConfig>& spIWMStreamConfig, DWORD dwBitRate)
	{
		CComPtr<IWMVideoMediaProps> spIWMVideoMediaProps;
		HRESULT hr = spIWMStreamConfig->QueryInterface(__uuidof(IWMVideoMediaProps), (void**)&spIWMVideoMediaProps);
		if (FAILED(hr))
			return hr;

		DWORD cbMT = 0;
		hr = spIWMVideoMediaProps->GetMediaType(NULL, &cbMT);
		if (FAILED(hr))
			return hr;

		// Allocate memory for the media type structure.
		WM_MEDIA_TYPE* pType = (WM_MEDIA_TYPE*)alloca(cbMT);
		// Get the media type structure.
		hr = spIWMVideoMediaProps->GetMediaType(pType, &cbMT);
		if (FAILED(hr) || (pType->formattype != WMFORMAT_VideoInfo) || (NULL == pType->pbFormat))
			return E_FAIL;
		
		bool bFound = false;
		// First set pointers to the video structures.
		WMVIDEOINFOHEADER* pVidHdr = (WMVIDEOINFOHEADER*)pType->pbFormat;
		{
			const DWORD FourCC[] =
			{
				MAKEFOURCC('W', 'M', 'V', '3'),
				MAKEFOURCC('W', 'M', 'V', '2'),
				MAKEFOURCC('W', 'M', 'V', '1')
			};
			for (size_t i = 0; i < _countof(FourCC); ++i)
			{
				if (FourCC[i] == pVidHdr->bmiHeader.biCompression)
				{
					bFound = true;
					break;
				}
			}
		}
		if (!bFound)
			return E_FAIL;

		pVidHdr->dwBitRate = dwBitRate;
		pVidHdr->rcSource.right = m_lFrameWidth;
		pVidHdr->rcSource.bottom = m_lFrameHeight;
		pVidHdr->rcTarget.right = m_lFrameWidth;
		pVidHdr->rcTarget.bottom = m_lFrameHeight;

		BITMAPINFOHEADER* pbmi = &(pVidHdr->bmiHeader);
		pbmi->biWidth  = m_lFrameWidth;
		pbmi->biHeight = m_lFrameHeight;
    
		// Stride = (width * bytes/pixel), rounded to the next DWORD boundary.
		LONG lStride = (m_lFrameWidth * (pbmi->biBitCount / 8) + 3) & ~3;

		// Image size = stride * height. 
		pbmi->biSizeImage = m_lFrameHeight * lStride;

		// Apply the adjusted type to the video input. 
		hr = spIWMVideoMediaProps->SetMediaType(pType);
		if (FAILED(hr))
			return hr;

		/* you can change this quality */
		spIWMVideoMediaProps->SetQuality(100);
		return hr;
	}
};

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CUpdateUI<CMainFrame>
{
	HNnCam		m_hCam;
	CMainView		m_view;
	NncamInstV2	m_ti[NNCAM_MAX];
	unsigned		m_nIndex;
	BOOL			m_bPaused;
	int				m_nSnapType; // 0-> not snaping, 1 -> single snap, 2 -> multiple snap
	unsigned		m_nSnapSeq;
	unsigned		m_nSnapFile;
	unsigned		m_nFrameCount;
	DWORD			m_dwStartTick, m_dwLastTick;

	wchar_t			m_szFilePath[MAX_PATH];

	CWmvRecord*		m_pWmvRecord;
	BYTE*			m_pData;
	BITMAPINFOHEADER	m_header;

	typedef enum {
		eTriggerNumber,
		eTriggerLoop
	}eTriggerType;
	eTriggerType	m_eTriggerType;
	unsigned short	m_nTriggerNumber;

	unsigned		m_xRoiOffset, m_yRoiOffset, m_xRoiWidth, m_yRoiHeight;
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAIN)

	BEGIN_MSG_MAP_EX(CMainFrame)
		MSG_WM_CREATE(OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnWmDestroy)
		MESSAGE_HANDLER(MSG_CAMEVENT, OnMsgCamEvent)
		COMMAND_RANGE_HANDLER_EX(ID_DEVICE_DEVICE0, ID_DEVICE_DEVICEF, OnOpenDevice)
		COMMAND_RANGE_HANDLER_EX(ID_PREVIEW_RESOLUTION0, ID_PREVIEW_RESOLUTION4, OnPreviewResolution)
		COMMAND_RANGE_HANDLER_EX(ID_SNAP_RESOLUTION0, ID_SNAP_RESOLUTION4, OnSnapResolution)
		COMMAND_RANGE_HANDLER_EX(ID_SNAPN_RESOLUTION0, ID_SNAPN_RESOLUTION4, OnSnapnResolution)
		COMMAND_RANGE_HANDLER_EX(ID_TESTPATTERN0, ID_TESTPATTERN3, OnTestPattern)
		COMMAND_ID_HANDLER_EX(ID_CONFIG_WHITEBALANCE, OnWhiteBalance)
		COMMAND_ID_HANDLER_EX(ID_CONFIG_AUTOEXPOSURE, OnAutoExposure)
		COMMAND_ID_HANDLER_EX(ID_CONFIG_VERTICALFLIP, OnVerticalFlip)
		COMMAND_ID_HANDLER_EX(ID_CONFIG_HORIZONTALFLIP, OnHorizontalFlip)
		COMMAND_ID_HANDLER_EX(ID_ACTION_PAUSE, OnPause)
		COMMAND_ID_HANDLER_EX(ID_CONFIG_EXPOSURETIME, OnExposureTime)
		COMMAND_ID_HANDLER_EX(ID_ACTION_STARTRECORD, OnStartRecord)
		COMMAND_ID_HANDLER_EX(ID_ACTION_STOPRECORD, OnStopRecord)
		COMMAND_ID_HANDLER_EX(ID_ACTION_LED, OnLed)
		COMMAND_ID_HANDLER_EX(ID_PIXELFORMAT, OnPixelFormat)
		COMMAND_ID_HANDLER_EX(ID_TECTARGET, OnTECTarget)
		COMMAND_ID_HANDLER_EX(ID_ACTION_EEPROM, OnEEPROM)
		COMMAND_ID_HANDLER_EX(ID_ACTION_UART, OnUART)
		COMMAND_ID_HANDLER_EX(ID_ACTION_FWVER, OnFwVer)
		COMMAND_ID_HANDLER_EX(ID_ACTION_HWVER, OnHwVer)
		COMMAND_ID_HANDLER_EX(ID_ACTION_FPGAVER, OnFpgaVer)
		COMMAND_ID_HANDLER_EX(ID_ACTION_PRODUCTIONDATE, OnProductionDate)
		COMMAND_ID_HANDLER_EX(ID_ACTION_SN, OnSn)
		COMMAND_ID_HANDLER_EX(ID_ACTION_RAWFORMAT, OnRawformat)
		COMMAND_ID_HANDLER_EX(ID_ACTION_ROI, OnRoi)
		COMMAND_ID_HANDLER_EX(ID_TRIGGER_MODE, OnTriggerMode)
		COMMAND_ID_HANDLER_EX(ID_TRIGGER_TRIGGER, OnTriggerTrigger)
		COMMAND_ID_HANDLER_EX(ID_TRIGGER_NUMBER, OnTriggerNumber)
		COMMAND_ID_HANDLER_EX(ID_TRIGGER_IOCONFIG, OnIoControl)
		COMMAND_ID_HANDLER_EX(ID_MAXAE, OnMaxAE)
		COMMAND_ID_HANDLER_EX(ID_TRIGGER_LOOP, OnTriggerLoop)
		COMMAND_ID_HANDLER_EX(ID_SPEED, OnSpeed)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_CONFIG_WHITEBALANCE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_CONFIG_AUTOEXPOSURE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_CONFIG_VERTICALFLIP, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_CONFIG_HORIZONTALFLIP, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_STARTRECORD, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_STOPRECORD, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_PAUSE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_LED, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_PIXELFORMAT, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TECTARGET, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SPEED, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_EEPROM, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_UART, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_FWVER, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_HWVER, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_FPGAVER, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TRIGGER_IOCONFIG, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_PRODUCTIONDATE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_SN, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTION_RAWFORMAT, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_CONFIG_EXPOSURETIME, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_PREVIEW_RESOLUTION0, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_PREVIEW_RESOLUTION1, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_PREVIEW_RESOLUTION2, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_PREVIEW_RESOLUTION3, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_PREVIEW_RESOLUTION4, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SNAP_RESOLUTION0, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SNAP_RESOLUTION1, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SNAP_RESOLUTION2, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SNAP_RESOLUTION3, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SNAP_RESOLUTION4, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SNAPN_RESOLUTION0, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SNAPN_RESOLUTION1, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SNAPN_RESOLUTION2, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SNAPN_RESOLUTION3, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_SNAPN_RESOLUTION4, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TESTPATTERN0, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TESTPATTERN1, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TESTPATTERN2, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TESTPATTERN3, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TRIGGER_MODE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TRIGGER_TRIGGER, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_MAXAE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TRIGGER_LOOP, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()

	LRESULT OnMsgCamEvent(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		switch (wParam)
		{
		case NNCAM_EVENT_ERROR:
		case NNCAM_EVENT_TIMEOUT:
			OnEventError();
			break;
		case NNCAM_EVENT_DISCONNECTED:
			OnEventDisconnected();
			break;
		case NNCAM_EVENT_EXPOSURE:
			OnEventExpo();
			break;
		case NNCAM_EVENT_TEMPTINT:
			OnEventTemptint();
			break;
		case NNCAM_EVENT_IMAGE:
			OnEventImage();
			if (eTriggerLoop == m_eTriggerType)
				Nncam_Trigger(m_hCam, 1);
			break;
		case NNCAM_EVENT_STILLIMAGE:
			OnEventSnap();
			break;
		}
		return 0;
	}

	CMainFrame()
	: m_hCam(NULL), m_nIndex(0), m_bPaused(FALSE), m_nSnapType(0), m_nSnapSeq(0), m_nSnapFile(0), m_nFrameCount(0), m_dwStartTick(0), m_dwLastTick(0), m_pWmvRecord(NULL), m_pData(NULL), m_view(this)
	{
		m_nTriggerNumber = 1;
		m_eTriggerType = eTriggerNumber;

		memset(m_ti, 0, sizeof(m_ti));
		memset(m_szFilePath, 0, sizeof(m_szFilePath));
		
		memset(&m_header, 0, sizeof(m_header));
		m_header.biSize = sizeof(BITMAPINFOHEADER);
		m_header.biPlanes = 1;
		m_header.biBitCount = 24;

		m_xRoiOffset = m_yRoiOffset = m_xRoiWidth = m_yRoiHeight = 0;
	}

	int OnCreate(LPCREATESTRUCT /*lpCreateStruct*/)
	{
		CMenuHandle menu = GetMenu();
		CMenuHandle submenu = menu.GetSubMenu(0);
		while (submenu.GetMenuItemCount() > 0)
			submenu.RemoveMenu(submenu.GetMenuItemCount() - 1, MF_BYPOSITION);

		unsigned cnt = Nncam_EnumV2(m_ti);
		if (0 == cnt)
			submenu.AppendMenu(MF_GRAYED | MF_STRING, ID_DEVICE_DEVICE0, L"No Device");
		else
		{
			for (unsigned i = 0; i < cnt; ++i)
				submenu.AppendMenu(MF_STRING, ID_DEVICE_DEVICE0 + i, m_ti[i].displayname);
		}

		CreateSimpleStatusBar();
		{
			int iWidth[] = { 150, 400, 600, -1 };
			CStatusBarCtrl statusbar(m_hWndStatusBar);
			statusbar.SetParts(_countof(iWidth), iWidth);
		}

		m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
		
		OnDeviceChanged();
		return 0;
	}

	void OnWhiteBalance(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
			Nncam_AwbOnePush(m_hCam, NULL, NULL);
	}

	void OnAutoExposure(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			BOOL bAutoExposure = FALSE;
			if (SUCCEEDED(Nncam_get_AutoExpoEnable(m_hCam, &bAutoExposure)))
			{
				bAutoExposure = !bAutoExposure;
				Nncam_put_AutoExpoEnable(m_hCam, bAutoExposure);
				UISetCheck(ID_CONFIG_AUTOEXPOSURE, bAutoExposure ? 1 : 0);
				UIEnable(ID_CONFIG_EXPOSURETIME, !bAutoExposure);
			}
		}
	}

	void OnVerticalFlip(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			BOOL b = FALSE;
			if (SUCCEEDED(Nncam_get_VFlip(m_hCam, &b)))
			{
				b = !b;
				Nncam_put_VFlip(m_hCam, b);
				UISetCheck(ID_CONFIG_VERTICALFLIP, b ? 1 : 0);
			}
		}
	}

	void OnHorizontalFlip(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			BOOL b = FALSE;
			if (SUCCEEDED(Nncam_get_HFlip(m_hCam, &b)))
			{
				b = !b;
				Nncam_put_HFlip(m_hCam, b);
				UISetCheck(ID_CONFIG_HORIZONTALFLIP, b ? 1 : 0);
			}
		}
	}

	void OnPause(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			m_bPaused = !m_bPaused;
			Nncam_Pause(m_hCam, m_bPaused);
			
			UISetCheck(ID_ACTION_PAUSE, m_bPaused ? 1 : 0);
			UIEnable(ID_ACTION_STARTRECORD, !m_bPaused);
		}
	}

	void OnExposureTime(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			CExposureTimeDlg dlg(m_hCam);
			if (IDOK == dlg.DoModal())
				UpdateExposureTimeText();
		}
	}

	void OnMaxAE(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			CMaxAEDlg dlg(m_hCam);
			dlg.DoModal();
		}
	}

	void OnLed(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			CLedDlg dlg(m_hCam);
			dlg.DoModal();
		}
	}

	void OnPixelFormat(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			CPixelFormatDlg dlg(m_ti[m_nIndex], m_hCam);
			dlg.DoModal();
		}
	}

	void OnTECTarget(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam && (m_ti[m_nIndex].model->flag & NNCAM_FLAG_TEC_ONOFF)) // support set the tec target
		{
			CTECTargetDlg dlg(m_hCam);
			dlg.DoModal();
		}
	}

	void OnSpeed(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			CSpeedDlg dlg(m_hCam);
			dlg.DoModal();
		}
	}

	void OnEEPROM(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			CEEPROMDlg dlg(m_hCam);
			dlg.DoModal();
		}
	}

	void OnUART(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			CUARTDlg dlg(m_hCam);
			dlg.DoModal();
		}
	}

	void OnRoi(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			CRoiDlg dlg;
			Nncam_get_Roi(m_hCam, &dlg.xOffset_, &dlg.yOffset_, &dlg.xWidth_, &dlg.yHeight_);
			if (IDOK == dlg.DoModal())
			{
				if (SUCCEEDED(Nncam_put_Roi(m_hCam, dlg.xOffset_, dlg.yOffset_, dlg.xWidth_, dlg.yHeight_)))
				{
					Nncam_get_Roi(m_hCam, NULL, NULL, (unsigned*)&m_header.biWidth, (unsigned*)&m_header.biHeight);
					m_header.biSizeImage = TDIBWIDTHBYTES(m_header.biWidth * m_header.biBitCount) * m_header.biHeight;
					UpdateResolutionText();
				}
			}
		}
		else
		{
			CRoiDlg dlg;
			dlg.xOffset_ = m_xRoiOffset;
			dlg.yOffset_ = m_yRoiOffset;
			dlg.xWidth_ = m_xRoiWidth;
			dlg.yHeight_ = m_yRoiHeight;
			if (IDOK == dlg.DoModal())
			{
				m_xRoiOffset = dlg.xOffset_;
				m_yRoiOffset = dlg.yOffset_;
				m_xRoiWidth = dlg.xWidth_;
				m_yRoiHeight = dlg.yHeight_;
			}
		}
	}

	void OnFwVer(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		char ver[16] = { 0 };
		if (SUCCEEDED(Nncam_get_FwVersion(m_hCam, ver)))
		{
			CA2T a2t(ver);
			AtlMessageBox(m_hWnd, a2t.m_psz, L"FwVer");
		}
	}

	void OnHwVer(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		char ver[16] = { 0 };
		if (SUCCEEDED(Nncam_get_HwVersion(m_hCam, ver)))
		{
			CA2T a2t(ver);
			AtlMessageBox(m_hWnd, a2t.m_psz, L"HwVer");
		}
	}

	void OnIoControl(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			if (m_ti[m_nIndex].model->ioctrol <= 0)
				AtlMessageBox(m_hWnd, L"No IoControl");
			else
			{
				CIocontrolDlg dlg(m_hCam, m_ti[m_nIndex]);
				dlg.DoModal();
			}
		}
	}

	void OnFpgaVer(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		char ver[16] = { 0 };
		if (SUCCEEDED(Nncam_get_FpgaVersion(m_hCam, ver)))
		{
			CA2T a2t(ver);
			AtlMessageBox(m_hWnd, a2t.m_psz, L"FPGAVer");
		}
	}

	void OnTestPattern(UINT /*uNotifyCode*/, int nID, HWND /*wndCtl*/)
	{
		if (NULL == m_hCam)
			return;
		int val = nID - ID_TESTPATTERN0;
		if (val)
			val = val * 2 + 1;
		Nncam_put_Option(m_hCam, NNCAM_OPTION_TESTPATTERN, val);
	}

	void OnProductionDate(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		char pdate[10] = { 0 };
		if (SUCCEEDED(Nncam_get_ProductionDate(m_hCam, pdate)))
		{
			CA2T a2t(pdate);
			AtlMessageBox(m_hWnd, a2t.m_psz, L"ProductionDate");
		}
	}

	void OnSn(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		char sn[32] = { 0 };
		if (SUCCEEDED(Nncam_get_SerialNumber(m_hCam, sn)))
		{
			CA2T a2t(sn);
			AtlMessageBox(m_hWnd, a2t.m_psz, L"Serial Number");
		}
	}

	void OnRawformat(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		unsigned nFourCC = 0, bitsperpixel = 0;
		if (SUCCEEDED(Nncam_get_RawFormat(m_hCam, &nFourCC, &bitsperpixel)))
		{
			wchar_t str[257];
			swprintf(str, L"FourCC:0x%08x, %c%c%c%c\nBits per Pixel: %u", nFourCC, (char)(nFourCC & 0xff), (char)((nFourCC >> 8) & 0xff), (char)((nFourCC >> 16) & 0xff), (char)((nFourCC >> 24) & 0xff), bitsperpixel);
			AtlMessageBox(m_hWnd, str, L"Raw Format");
		}
	}

	void OnTriggerMode(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			int val = 0;
			Nncam_get_Option(m_hCam, NNCAM_OPTION_TRIGGER, &val);
			if (val == 0)
				val = 2;
			else
				val = 0;
			Nncam_put_Option(m_hCam, NNCAM_OPTION_TRIGGER, val);
			UISetCheck(ID_TRIGGER_MODE, (val == 2)? 1 : 0);
			UIEnable(ID_ACTION_STOPRECORD, (m_hCam && (val == 2)) ? TRUE : FALSE);
			UIEnable(ID_TRIGGER_TRIGGER, (val == 2) ? 1 : 0);
			UIEnable(ID_TRIGGER_LOOP, (val == 2) ? 1 : 0);
			UIEnable(ID_TRIGGER_IOCONFIG, (val == 2) ? 1 : 0);
		}
	}

	void OnTriggerNumber(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		CTriggerNumberDlg dlg;
		dlg.number_ = m_nTriggerNumber;
		if (IDOK == dlg.DoModal())
		{
			m_nTriggerNumber = dlg.number_;
			if (m_ti[m_nIndex].model->ioctrol > 0)
				Nncam_IoControl(m_hCam, 0, NNCAM_IOCONTROLTYPE_SET_BURSTCOUNTER, m_nTriggerNumber, NULL);
		}
	}

	void OnTriggerTrigger(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			int val = 0;
			Nncam_get_Option(m_hCam, NNCAM_OPTION_TRIGGER, &val);
			if (val == 2)
			{
				m_eTriggerType = eTriggerNumber;
				Nncam_IoControl(m_hCam, 0, NNCAM_IOCONTROLTYPE_SET_TRIGGERSOURCE, 5, NULL);
				HRESULT hr = Nncam_Trigger(m_hCam, m_nTriggerNumber);
				if (E_INVALIDARG == hr)
				{
					if (m_nTriggerNumber > 1)
						AtlMessageBox(m_hWnd, L"NNCAM_FLAG_TRIGGER_SINGLE: only number = 1 supported");
				}
			}
		}
	}

	void OnTriggerLoop(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		if (m_hCam)
		{
			int val = 0;
			Nncam_get_Option(m_hCam, NNCAM_OPTION_TRIGGER, &val);
			if (val == 2)
			{
				if (eTriggerLoop == m_eTriggerType)
				{
					m_eTriggerType = eTriggerNumber;
					UIEnable(ID_TRIGGER_MODE, TRUE);
					UIEnable(ID_TRIGGER_TRIGGER, TRUE);
					Nncam_Trigger(m_hCam, 0);
				}
				else
				{
					UIEnable(ID_TRIGGER_MODE, FALSE);
					UIEnable(ID_TRIGGER_TRIGGER, FALSE);
					m_eTriggerType = eTriggerLoop;
					Nncam_IoControl(m_hCam, 0, NNCAM_IOCONTROLTYPE_SET_TRIGGERSOURCE, 5, NULL);
					Nncam_Trigger(m_hCam, 1);
				}
			}
		}
	}

	void OnPreviewResolution(UINT /*uNotifyCode*/, int nID, HWND /*wndCtl*/)
	{
		if (NULL == m_hCam)
			return;

		unsigned eSize = 0;
		if (SUCCEEDED(Nncam_get_eSize(m_hCam, &eSize)))
		{
			if (eSize != nID - ID_PREVIEW_RESOLUTION0)
			{
				if (SUCCEEDED(Nncam_Stop(m_hCam)))
				{
					OnStopRecord(0, 0, NULL);

					m_bPaused = FALSE;
					m_nSnapType = 0;
					m_nSnapSeq = 0;
					UISetCheck(ID_ACTION_PAUSE, FALSE);
					m_nFrameCount = 0;
					m_dwStartTick = m_dwLastTick = 0;

					Nncam_put_eSize(m_hCam, nID - ID_PREVIEW_RESOLUTION0);
					for (unsigned i = 0; i < m_ti[m_nIndex].model->preview; ++i)
						UISetCheck(ID_PREVIEW_RESOLUTION0 + i, (nID - ID_PREVIEW_RESOLUTION0 == i) ? 1 : 0);
					UpdateSnapMenu();
					if (SUCCEEDED(Nncam_get_Size(m_hCam, (int*)&m_header.biWidth, (int*)&m_header.biHeight)))
					{
						UpdateResolutionText();
						UpdateFrameText(L"");
						UpdateExposureTimeText();

						m_header.biSizeImage = TDIBWIDTHBYTES(m_header.biWidth * m_header.biBitCount) * m_header.biHeight;
						if (m_pData)
						{
							free(m_pData);
							m_pData = NULL;
						}
						m_pData = (BYTE*)malloc(m_header.biSizeImage);
						if (SUCCEEDED(Nncam_StartPullModeWithWndMsg(m_hCam, m_hWnd, MSG_CAMEVENT)))
						{
							UIEnable(ID_ACTION_PAUSE, TRUE);
							UIEnable(ID_ACTION_STARTRECORD, TRUE);
							UIEnable(ID_TESTPATTERN0, TRUE);
							UIEnable(ID_TESTPATTERN1, TRUE);
							UIEnable(ID_TESTPATTERN2, TRUE);
							UIEnable(ID_TESTPATTERN3, TRUE);
						}
					}
				}
			}
		}
	}

	void OnSnapResolution(UINT /*uNotifyCode*/, int nID, HWND /*wndCtl*/)
	{
		if (NULL == m_hCam)
			return;

		CFileDialog dlg(FALSE, L"jpg");
		if (IDOK == dlg.DoModal())
		{
			wcscpy(m_szFilePath, dlg.m_szFileName);
			if (SUCCEEDED(Nncam_Snap(m_hCam, nID - ID_SNAP_RESOLUTION0)))
			{
				m_nSnapType = 1;
				m_nSnapSeq = 0;
				UpdateSnapMenu();
			}
		}
	}

	void OnSnapnResolution(UINT /*uNotifyCode*/, int nID, HWND /*wndCtl*/)
	{
		if (NULL == m_hCam)
			return;

		CSnapnDlg dlg;
		if ((IDOK == dlg.DoModal()) && (dlg.m_nNum > 0))
		{
			if (SUCCEEDED(Nncam_SnapN(m_hCam, nID - ID_SNAPN_RESOLUTION0, dlg.m_nNum)))
			{
				m_nSnapType = 2;
				m_nSnapSeq = dlg.m_nNum;
				UpdateSnapMenu();
			}
		}
	}

	void OnOpenDevice(UINT /*uNotifyCode*/, int nID, HWND /*wndCtl*/)
	{
		CloseDevice();

		m_header.biWidth = m_header.biHeight = 0;
		m_header.biSizeImage = 0;
		m_bPaused = FALSE;
		m_nSnapType = 0;
		m_nSnapSeq = 0;
		UISetCheck(ID_ACTION_PAUSE, FALSE);
		m_nFrameCount = 0;
		m_dwStartTick = m_dwLastTick = 0;
		m_nIndex = nID - ID_DEVICE_DEVICE0;
		m_hCam = Nncam_Open(m_ti[m_nIndex].id);
		if (m_hCam)
		{
			/* just to demo put roi befor the camera is started */
			if (m_xRoiWidth && m_yRoiHeight)
			{
				Nncam_put_Roi(m_hCam, m_xRoiOffset, m_yRoiOffset, m_xRoiWidth, m_yRoiHeight);
				Nncam_get_Roi(m_hCam, NULL, NULL, (unsigned*)&m_header.biWidth, (unsigned*)&m_header.biHeight);
			}
			else
			{
				Nncam_get_Size(m_hCam, (int*)&m_header.biWidth, (int*)&m_header.biHeight);
			}

			OnDeviceChanged();
			UpdateFrameText(L"");

			if ((m_header.biWidth > 0) && (m_header.biHeight > 0))
			{
				m_header.biSizeImage = TDIBWIDTHBYTES(m_header.biWidth * m_header.biBitCount) * m_header.biHeight;
				m_pData = (BYTE*)malloc(m_header.biSizeImage);
				unsigned eSize = 0;
				if (SUCCEEDED(Nncam_get_eSize(m_hCam, &eSize)))
				{
					for (unsigned i = 0; i < m_ti[m_nIndex].model->preview; ++i)
						UISetCheck(ID_PREVIEW_RESOLUTION0 + i, (eSize == i) ? 1 : 0);
				}
				if (SUCCEEDED(Nncam_StartPullModeWithWndMsg(m_hCam, m_hWnd, MSG_CAMEVENT)))
				{
					UIEnable(ID_ACTION_PAUSE, TRUE);
					UIEnable(ID_ACTION_STARTRECORD, TRUE);
					UIEnable(ID_TESTPATTERN0, TRUE);
					UIEnable(ID_TESTPATTERN1, TRUE);
					UIEnable(ID_TESTPATTERN2, TRUE);
					UIEnable(ID_TESTPATTERN3, TRUE);
				}
			}
		}
	}

	void OnStartRecord(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		CFileDialog dlg(FALSE, L"wmv");
		if (IDOK == dlg.DoModal())
		{
			StopRecord();

			DWORD dwBitrate = 4 * 1024 * 1024; /* bitrate, you can change this setting */
			CWmvRecord* pWmvRecord = new CWmvRecord(m_header.biWidth, m_header.biHeight);
			if (pWmvRecord->StartRecord(dlg.m_szFileName, dwBitrate))
			{
				m_pWmvRecord = pWmvRecord;
				UIEnable(ID_ACTION_STARTRECORD, FALSE);
				UIEnable(ID_ACTION_STOPRECORD, TRUE);
			}
			else
			{
				delete pWmvRecord;
			}
		}
	}

	void OnStopRecord(UINT /*uNotifyCode*/, int /*nID*/, HWND /*wndCtl*/)
	{
		StopRecord();

		UIEnable(ID_ACTION_STARTRECORD, m_hCam ? TRUE : FALSE);
		UIEnable(ID_ACTION_STOPRECORD, FALSE);
	}

	void OnEventImage()
	{
		NncamFrameInfoV2 info = { 0 };
		HRESULT hr = Nncam_PullImageV2(m_hCam, m_pData, m_header.biBitCount, &info);
		if (FAILED(hr))
			return;
		if ((info.width != m_header.biWidth) || (info.height != m_header.biHeight))
			return;

		++m_nFrameCount;
		if (0 == m_dwStartTick)
			m_dwLastTick = m_dwStartTick = GetTickCount();
		else
			m_dwLastTick = GetTickCount();
		m_view.Invalidate();

		UpdateFrameText(info);
		if (m_pWmvRecord)
			m_pWmvRecord->WriteSample(m_pData);
	}

	void OnEventSnap()
	{
		BITMAPINFOHEADER header = { 0 };
		header.biSize = sizeof(header);
		header.biPlanes = 1;
		header.biBitCount = 24;
		HRESULT hr = Nncam_PullStillImage(m_hCam, NULL, 24, (unsigned*)&header.biWidth, (unsigned*)&header.biHeight); //first, peek the width and height
		if (SUCCEEDED(hr))
		{
			header.biSizeImage = TDIBWIDTHBYTES(header.biWidth * header.biBitCount) * header.biHeight;
			void* pSnapData = malloc(header.biSizeImage);
			if (pSnapData)
			{
				hr = Nncam_PullStillImage(m_hCam, pSnapData, 24, NULL, NULL);
				if (SUCCEEDED(hr))
				{
					if (2 == m_nSnapType)
					{
						TCHAR strPath[MAX_PATH];
						_stprintf(strPath, _T("%04u.jpg"), m_nSnapFile++);
						SaveImageByWIC(strPath, pSnapData, &header);
					}
					else
					{
						if (PathMatchSpec(m_szFilePath, L"*.bmp"))
							SaveImageBmp(m_szFilePath, pSnapData, &header);
						else
							SaveImageByWIC(m_szFilePath, pSnapData, &header);
					}
				}

				free(pSnapData);
			}
		}

		if (1 == m_nSnapType)
			m_nSnapType = 0;
		else if (2 == m_nSnapType)
		{
			if (m_nSnapSeq > 0)
			{
				if (--m_nSnapSeq == 0)
					m_nSnapType = 0;
			}
		}
		UpdateSnapMenu();
	}

	LRESULT OnWmDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		CloseDevice();

		CFrameWindowImpl<CMainFrame>::OnDestroy(uMsg, wParam, lParam, bHandled);
		return 0;
	}

	void OnEventError()
	{
		CloseDevice();
		AtlMessageBox(m_hWnd, _T("Error"));
	}

	void OnEventDisconnected()
	{
		CloseDevice();
		AtlMessageBox(m_hWnd, _T("The camera is disconnected, maybe has been pulled out."));
	}

	void OnEventTemptint()
	{
		CStatusBarCtrl statusbar(m_hWndStatusBar);
		wchar_t res[128];
		int nTemp = NNCAM_TEMP_DEF, nTint = NNCAM_TINT_DEF;
		Nncam_get_TempTint(m_hCam, &nTemp, &nTint);
		swprintf(res, L"Temp = %d, Tint = %d", nTemp, nTint);
		statusbar.SetText(2, res);
	}

	void OnEventExpo()
	{
		CStatusBarCtrl statusbar(m_hWndStatusBar);
		wchar_t res[128];
		unsigned nTime = 0;
		unsigned short AGain = 0;
		if (SUCCEEDED(Nncam_get_ExpoTime(m_hCam, &nTime)) && SUCCEEDED(Nncam_get_ExpoAGain(m_hCam, &AGain)))
		{
			swprintf(res, L"ExposureTime = %u, AGain = %hu", nTime, AGain);
			statusbar.SetText(1, res);
		}
	}

	BOOL GetData(BITMAPINFOHEADER** pHeader, BYTE** pData)
	{
		if (m_pData)
		{
			*pData = m_pData;
			*pHeader = &m_header;
			return TRUE;
		}

		return FALSE;
	}

private:
	void CloseDevice()
	{
		OnStopRecord(0, 0, NULL);

		if (m_hCam)
		{
			Nncam_Close(m_hCam);
			m_hCam = NULL;

			if (m_pData)
			{
				free(m_pData);
				m_pData = NULL;
			}
		}
		OnDeviceChanged();
	}

	void OnDeviceChanged()
	{
		CMenuHandle menu = GetMenu();
		CMenuHandle submenu = menu.GetSubMenu(1);
		CMenuHandle previewsubmenu = submenu.GetSubMenu(0);
		CMenuHandle snapsubmenu = submenu.GetSubMenu(1);
		CMenuHandle snapnsubmenu = submenu.GetSubMenu(2);
		while (previewsubmenu.GetMenuItemCount() > 0)
			previewsubmenu.RemoveMenu(previewsubmenu.GetMenuItemCount() - 1, MF_BYPOSITION);
		while (snapsubmenu.GetMenuItemCount() > 0)
			snapsubmenu.RemoveMenu(snapsubmenu.GetMenuItemCount() - 1, MF_BYPOSITION);
		while (snapnsubmenu.GetMenuItemCount() > 0)
			snapnsubmenu.RemoveMenu(snapnsubmenu.GetMenuItemCount() - 1, MF_BYPOSITION);

		CStatusBarCtrl statusbar(m_hWndStatusBar);

		if (NULL == m_hCam)
		{
			previewsubmenu.AppendMenu(MF_STRING | MF_GRAYED, ID_PREVIEW_RESOLUTION0, L"Empty");
			snapsubmenu.AppendMenu(MF_STRING | MF_GRAYED, ID_SNAP_RESOLUTION0, L"Empty");
			snapnsubmenu.AppendMenu(MF_STRING | MF_GRAYED, ID_SNAP_RESOLUTION0, L"Empty");
			UIEnable(ID_SNAP_RESOLUTION0, FALSE);
			UIEnable(ID_PREVIEW_RESOLUTION0, FALSE);

			statusbar.SetText(0, L"");
			statusbar.SetText(1, L"");
			statusbar.SetText(2, L"");
			statusbar.SetText(3, L"");

			UIEnable(ID_CONFIG_EXPOSURETIME, FALSE);
		}
		else
		{
			unsigned eSize = 0;
			Nncam_get_eSize(m_hCam, &eSize);

			wchar_t res[128];
			for (unsigned i = 0; i < m_ti[m_nIndex].model->preview; ++i)
			{
				swprintf(res, L"%u * %u", m_ti[m_nIndex].model->res[i].width, m_ti[m_nIndex].model->res[i].height);
				previewsubmenu.AppendMenu(MF_STRING, ID_PREVIEW_RESOLUTION0 + i, res);
				snapsubmenu.AppendMenu(MF_STRING, ID_SNAP_RESOLUTION0 + i, res);
				snapnsubmenu.AppendMenu(MF_STRING, ID_SNAPN_RESOLUTION0 + i, res);

				UIEnable(ID_PREVIEW_RESOLUTION0 + i, TRUE);
			}
			UpdateSnapMenu();

			UpdateResolutionText();
			UpdateExposureTimeText();

			int nTemp = NNCAM_TEMP_DEF, nTint = NNCAM_TINT_DEF;
			if (SUCCEEDED(Nncam_get_TempTint(m_hCam, &nTemp, &nTint)))
			{
				swprintf(res, L"Temp = %d, Tint = %d", nTemp, nTint);
				statusbar.SetText(2, res);
			}

			BOOL bAutoExposure = TRUE;
			if (SUCCEEDED(Nncam_get_AutoExpoEnable(m_hCam, &bAutoExposure)))
			{
				UISetCheck(ID_CONFIG_AUTOEXPOSURE, bAutoExposure ? 1 : 0);
				UIEnable(ID_CONFIG_EXPOSURETIME, !bAutoExposure);
			}
		}

		UIEnable(ID_ACTION_PAUSE, FALSE);
		UIEnable(ID_ACTION_STARTRECORD, FALSE);
		UIEnable(ID_ACTION_STOPRECORD, FALSE);
		UIEnable(ID_CONFIG_AUTOEXPOSURE, m_hCam ? TRUE : FALSE);
		UIEnable(ID_CONFIG_HORIZONTALFLIP, m_hCam ? TRUE : FALSE);
		UIEnable(ID_CONFIG_VERTICALFLIP, m_hCam ? TRUE : FALSE);
		UIEnable(ID_CONFIG_WHITEBALANCE, m_hCam ? TRUE : FALSE);
		UIEnable(ID_ACTION_LED, m_hCam ? TRUE : FALSE);
		UIEnable(ID_PIXELFORMAT, m_hCam ? TRUE : FALSE);
		UIEnable(ID_ACTION_EEPROM, m_hCam ? TRUE : FALSE);
		UIEnable(ID_ACTION_UART, m_hCam ? TRUE : FALSE);
		UIEnable(ID_ACTION_FWVER, m_hCam ? TRUE : FALSE);
		UIEnable(ID_ACTION_HWVER, m_hCam ? TRUE : FALSE);
		UIEnable(ID_ACTION_FPGAVER, m_hCam ? TRUE : FALSE);
		UIEnable(ID_ACTION_PRODUCTIONDATE, m_hCam ? TRUE : FALSE);
		UIEnable(ID_ACTION_SN, m_hCam ? TRUE : FALSE);
		UIEnable(ID_ACTION_RAWFORMAT, m_hCam ? TRUE : FALSE);
		UISetCheck(ID_ACTION_PAUSE, 0);
		UISetCheck(ID_CONFIG_HORIZONTALFLIP, 0);
		UISetCheck(ID_CONFIG_VERTICALFLIP, 0);
		UIEnable(ID_ACTION_STOPRECORD, m_hCam ? TRUE : FALSE);
		UIEnable(ID_TECTARGET, (m_hCam && (m_ti[m_nIndex].model->flag & NNCAM_FLAG_TEC_ONOFF)) ? TRUE : FALSE);
		UIEnable(ID_SPEED, m_hCam ? TRUE : FALSE);
		UIEnable(ID_MAXAE, m_hCam ? TRUE : FALSE);

		UIEnable(ID_TRIGGER_MODE, m_hCam ? TRUE : FALSE);
		UIEnable(ID_TRIGGER_NUMBER, m_hCam ? TRUE : FALSE);
		UIEnable(ID_TRIGGER_TRIGGER, FALSE);
		UIEnable(ID_TRIGGER_LOOP, FALSE); 
		UIEnable(ID_TRIGGER_IOCONFIG, FALSE);
		UIEnable(ID_TESTPATTERN0, FALSE);
		UIEnable(ID_TESTPATTERN1, FALSE);
		UIEnable(ID_TESTPATTERN2, FALSE);
		UIEnable(ID_TESTPATTERN3, FALSE);
	}

	void UpdateSnapMenu()
	{
		if (m_nSnapType)
		{
			for (unsigned i = 0; i < m_ti[m_nIndex].model->preview; ++i)
			{
				UIEnable(ID_SNAP_RESOLUTION0 + i, FALSE);
				UIEnable(ID_SNAPN_RESOLUTION0 + i, FALSE);
			}
			return;
		}

		unsigned eSize = 0;
		if (SUCCEEDED(Nncam_get_eSize(m_hCam, &eSize)))
		{
			for (unsigned i = 0; i < m_ti[m_nIndex].model->preview; ++i)
			{
				if (m_ti[m_nIndex].model->still == m_ti[m_nIndex].model->preview) /* still capture full supported */
				{
					UIEnable(ID_SNAP_RESOLUTION0 + i, TRUE);
					UIEnable(ID_SNAPN_RESOLUTION0 + i, TRUE);
				}
				else if (0 == m_ti[m_nIndex].model->still) /* still capture not supported */
				{
					UIEnable(ID_SNAP_RESOLUTION0 + i, (eSize == i) ? TRUE : FALSE);
					UIEnable(ID_SNAPN_RESOLUTION0 + i, (eSize == i) ? TRUE : FALSE);
				}
				else if (m_ti[m_nIndex].model->still < m_ti[m_nIndex].model->preview)
				{
					if ((eSize == i) || (i < m_ti[m_nIndex].model->still))
					{
						UIEnable(ID_SNAP_RESOLUTION0 + i, TRUE);
						UIEnable(ID_SNAPN_RESOLUTION0 + i, TRUE);
					}
					else
					{
						UIEnable(ID_SNAP_RESOLUTION0 + i, FALSE);
						UIEnable(ID_SNAPN_RESOLUTION0 + i, FALSE);
					}
				}
			}
		}
	}

	void UpdateResolutionText()
	{
		CStatusBarCtrl statusbar(m_hWndStatusBar);
		wchar_t res[128];
		unsigned xOffset = 0, yOffset = 0, nWidth = 0, nHeight = 0;
		if (SUCCEEDED(Nncam_get_Roi(m_hCam, &xOffset, &yOffset, &nWidth, &nHeight)))
		{
			swprintf(res, L"%u, %u, %u * %u", xOffset, yOffset, nWidth, nHeight);
			statusbar.SetText(0, res);
		}
	}

	void UpdateFrameText(const wchar_t* str)
	{
		CStatusBarCtrl statusbar(m_hWndStatusBar);
		statusbar.SetText(3, str);
	}

	void UpdateFrameText(const NncamFrameInfoV2& info)
	{
		wchar_t str[256];
		if (m_dwLastTick != m_dwStartTick)
		{
			if (info.flag & (NNCAM_FRAMEINFO_FLAG_SEQ | NNCAM_FRAMEINFO_FLAG_TIMESTAMP))
				swprintf(str, L"%u, %.2f, %u, %llu", m_nFrameCount, m_nFrameCount / ((m_dwLastTick - m_dwStartTick) / 1000.0), info.seq, info.timestamp);
			else
				swprintf(str, L"%u, %.2f", m_nFrameCount, m_nFrameCount / ((m_dwLastTick - m_dwStartTick) / 1000.0));
		}
		else
		{
			if (info.flag & (NNCAM_FRAMEINFO_FLAG_SEQ | NNCAM_FRAMEINFO_FLAG_TIMESTAMP))
				swprintf(str, L"%u, %u, %llu", m_nFrameCount, info.seq, info.timestamp);
			else
				swprintf(str, L"%u", m_nFrameCount);
		}
		UpdateFrameText(str);
	}

	void UpdateExposureTimeText()
	{
		CStatusBarCtrl statusbar(m_hWndStatusBar);
		wchar_t res[128];
		unsigned nTime = 0;
		unsigned short AGain = 0;
		if (SUCCEEDED(Nncam_get_ExpoTime(m_hCam, &nTime)) && SUCCEEDED(Nncam_get_ExpoAGain(m_hCam, &AGain)))
		{
			swprintf(res, L"ExposureTime = %u, AGain = %hu", nTime, AGain);
			statusbar.SetText(1, res);
		}
	}

	/* this is called in the UI thread */
	void StopRecord()
	{
		if (m_pWmvRecord)
		{
			m_pWmvRecord->StopRecord();

			delete m_pWmvRecord;
			m_pWmvRecord = NULL;
		}
	}
};

LRESULT CMainView::OnWmPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CPaintDC dc(m_hWnd);

	RECT rc;
	GetClientRect(&rc);
	BITMAPINFOHEADER* pHeader = NULL;
	BYTE* pData = NULL;
	if (m_pMainFrame->GetData(&pHeader, &pData))
	{
		if ((m_nOldWidth != pHeader->biWidth) || (m_nOldHeight != pHeader->biHeight))
		{
			m_nOldWidth = pHeader->biWidth;
			m_nOldHeight = pHeader->biHeight;
			dc.FillRect(&rc, (HBRUSH)WHITE_BRUSH);
		}
		int m = dc.SetStretchBltMode(COLORONCOLOR);
		StretchDIBits(dc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0, 0, pHeader->biWidth, pHeader->biHeight, pData, (BITMAPINFO*)pHeader, DIB_RGB_COLORS, SRCCOPY);
		dc.SetStretchBltMode(m);
	}
	else
	{
		dc.FillRect(&rc, (HBRUSH)WHITE_BRUSH);
	}

	return 0;
}

static int Run(int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame frmMain;

	if (frmMain.CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	frmMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*pCmdLine*/, int nCmdShow)
{
	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(iccx);
	iccx.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
	InitCommonControlsEx(&iccx);

	OleInitialize(NULL);

	_Module.Init(NULL, hInstance);

	int nRet = Run(nCmdShow);

	_Module.Term();
	return nRet;
}
