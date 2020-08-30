#include "stdafx.h"
#include "demodshow.h"
#include "MainFrm.h"
#include <Dbt.h>
#include <uuids.h>
#include "SizeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)
BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_DEVICECHANGE()
	ON_UPDATE_COMMAND_UI(ID_CAMERA00, OnUpdateCamera)
	ON_COMMAND_RANGE(ID_CAMERA00, ID_CAMERA39, OnCamera)
	ON_COMMAND_RANGE(ID_SETUP_SETUP0, ID_SETUP_SETUP4, OnSetup)
	ON_WM_DESTROY()
	ON_MESSAGE(MSG_DSNOTIFY, OnMsgDsnotify)
	ON_MESSAGE(MSG_DSSIZE, OnMsgDssize)
	ON_WM_SIZE()
	ON_COMMAND(ID_EXAMPLE_SIZE, OnExampleSize)
	ON_COMMAND(ID_EXAMPLE_FLIPHORIZONTAL, OnExampleFliphorizontal)
	ON_COMMAND(ID_EXAMPLE_FLIPVERTICAL, OnExampleFlipvertical)
	ON_COMMAND(ID_EXAMPLE_PREVIEW_SNAPSHOT, OnPreviewSnapshot)
	ON_COMMAND(ID_EXAMPLE_STILLIMAGE_SNAPSHOT, OnStillimageSnapshot)
	ON_COMMAND(ID_EXAMPLE_CAPTURE, OnCapture)
	ON_COMMAND(ID_EXAMPLE_STOPCAPTURE, OnStopCapture)
	ON_UPDATE_COMMAND_UI(ID_EXAMPLE_SIZE, OnUpdateExampleSize)
	ON_UPDATE_COMMAND_UI(ID_EXAMPLE_FLIPHORIZONTAL, OnUpdateExampleFliphorizontal)
	ON_UPDATE_COMMAND_UI(ID_EXAMPLE_FLIPVERTICAL, OnUpdateExampleFlipvertical)
	ON_UPDATE_COMMAND_UI(ID_EXAMPLE_PREVIEW_SNAPSHOT, OnUpdatePreviewSnapshot)
	ON_UPDATE_COMMAND_UI(ID_EXAMPLE_STILLIMAGE_SNAPSHOT, OnUpdateStillimageSnapshot)
	ON_UPDATE_COMMAND_UI(ID_EXAMPLE_CAPTURE, OnUpdateCapture)
	ON_UPDATE_COMMAND_UI(ID_EXAMPLE_STOPCAPTURE, OnUpdateStopCapture)
	ON_COMMAND(ID_EXAMPLE_FRAMERATE, OnFramerate)
	ON_COMMAND(ID_EXAMPLE_SN, OnSn)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

CMainFrame::CMainFrame()
: m_pDshowContext(NULL)
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));

	{
		// Register for device add/remove notifications
		DEV_BROADCAST_DEVICEINTERFACE filterData;
		ZeroMemory(&filterData, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
		filterData.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		filterData.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		filterData.dbcc_classguid = AM_KSCATEGORY_CAPTURE;
        RegisterDeviceNotification(m_hWnd, &filterData, DEVICE_NOTIFY_WINDOW_HANDLE);
	}

	CMenu* pMenu = GetMenu()->GetSubMenu(2);
	while (pMenu->GetMenuItemCount())
		pMenu->RemoveMenu(0, MF_BYPOSITION);

	m_vecDshowDevice = dscap_enum_device();
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL CMainFrame::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{	
	if ((DBT_DEVICEARRIVAL == nEventType)
		|| (DBT_DEVICEREMOVECOMPLETE == nEventType)
		|| (DBT_DEVNODES_CHANGED == nEventType))
	{
		m_vecDshowDevice = dscap_enum_device();
	}

	return TRUE;
}

void CMainFrame::MenuCamera(CMenu* pMenu)
{
	while (pMenu->GetMenuItemCount())
		pMenu->RemoveMenu(0, MF_BYPOSITION);

	if (m_vecDshowDevice.empty())
		pMenu->AppendMenu(MF_GRAYED|MF_STRING, ID_CAMERA00, _T("No Device"));
	else
	{
		UINT nFlag;
		for (size_t i = 0; i < m_vecDshowDevice.size(); ++i)
		{
			nFlag = MF_STRING;
			if (m_curDshowDevice == m_vecDshowDevice[i].DisplayName)
				nFlag |= MF_CHECKED | MF_GRAYED;
			pMenu->AppendMenu(nFlag, ID_CAMERA00+i, m_vecDshowDevice[i].FriendlyName.c_str());
		}
	}
}

void CMainFrame::OnUpdateCamera(CCmdUI* pCmdUI)
{
	if (ID_CAMERA00 == pCmdUI->m_nID)
	{
		if (pCmdUI->m_pSubMenu)
			MenuCamera(pCmdUI->m_pSubMenu);
		else if (pCmdUI->m_pMenu)
		{
			MenuCamera(pCmdUI->m_pMenu);
			pCmdUI->m_nIndex = pCmdUI->m_nIndexMax = pCmdUI->m_pMenu->GetMenuItemCount();
		}
	}
	
	pCmdUI->m_bEnableChanged = TRUE;
}

void CMainFrame::OnSetup(UINT nID)
{
	if (m_pDshowContext)
		m_pDshowContext->on_dialog(nID - ID_SETUP_SETUP0);
}

void CMainFrame::OnCamera(UINT nID)
{
	CMenu* pMenu = GetMenu()->GetSubMenu(2);
	while (pMenu->GetMenuItemCount())
		pMenu->RemoveMenu(0, MF_BYPOSITION);

	if (m_pDshowContext)
	{
		m_pDshowContext->stop();
		m_pDshowContext = NULL;
		m_curDshowDevice.clear();
	}

	nID -= ID_CAMERA00;
	if (nID < m_vecDshowDevice.size())
	{
		m_curDshowDevice = m_vecDshowDevice[nID].DisplayName;
		m_pDshowContext = NewDshowContext(m_hWnd, m_wndView.m_hWnd, m_curDshowDevice, MSG_DSNOTIFY);
		if (m_pDshowContext)
			m_pDshowContext->start(GetMenu()->GetSubMenu(2)->GetSafeHmenu(), 0, 5, ID_SETUP_SETUP0);
	}
}

void CMainFrame::OnCapture()
{
	if (m_pDshowContext)
	{
		if (!m_pDshowContext->IsCapturing())
			m_pDshowContext->startcapture(L"demo.avi");
	}
}

void CMainFrame::OnStopCapture()
{
	if (m_pDshowContext)
	{
		if (m_pDshowContext->IsCapturing())
			m_pDshowContext->stopcapture();
	}
}

void CMainFrame::OnDestroy()
{
	if (m_pDshowContext)
	{
		m_pDshowContext->stop();
		m_pDshowContext = NULL;
		m_curDshowDevice.clear();
	}

	CFrameWnd::OnDestroy();
}

LRESULT CMainFrame::OnMsgDsnotify(WPARAM wp, LPARAM lp)
{
	if (m_pDshowContext)
		m_pDshowContext->on_notify();
	return 0;
}

LRESULT CMainFrame::OnMsgDssize(WPARAM wp, LPARAM lp)
{
	if (m_pDshowContext)
		m_pDshowContext->move_window();
	return 0;
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	if (nType != SIZE_MINIMIZED)
	{
		if (m_pDshowContext)
			m_pDshowContext->move_window();
	}
}

void CMainFrame::OnUpdateExampleSize(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pDshowContext ? TRUE : FALSE);
}

void CMainFrame::OnUpdateExampleFliphorizontal(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pDshowContext ? TRUE : FALSE);
}

void CMainFrame::OnUpdateExampleFlipvertical(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pDshowContext ? TRUE : FALSE);
}

void CMainFrame::OnExampleSize()
{
	if (NULL == m_pDshowContext)
		return;

	CSizeDlg dlg;
	if (IDOK == dlg.DoModal())
	{
		if (dlg.m_nSize < 10)
			dlg.m_nSize = 10;
		else if (dlg.m_nSize > 1600)
			dlg.m_nSize = 1600;
		
		if (IsZoomed())
			ShowWindow(SW_RESTORE);
		else if (IsIconic())
			ShowWindow(SW_RESTORE);
		long Width = 0, Height = 0;
		if (m_pDshowContext->video_size(&Width, &Height))
		{
			CRect rect, offset;
			GetClientRect(&rect);
			offset = rect;
			RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0, reposQuery, &offset, NULL);
		
			Width = Width * dlg.m_nSize / 100 + rect.Width() - offset.Width();
			Height = Height * dlg.m_nSize / 100 + rect.Height() - offset.Height();

			rect.SetRect(0, 0, Width, Height);
			::AdjustWindowRectEx(&rect, GetStyle(), TRUE, GetExStyle());
			SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOMOVE | SWP_NOZORDER);
		}

		CRect rect;
		GetClientRect(&rect);
		RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0, reposQuery, &rect, NULL);
		m_pDshowContext->move_window();
	}
}

void CMainFrame::OnExampleFliphorizontal()
{
	if (m_pDshowContext)
	{
		CComPtr<INncam> spIcam;
		if (SUCCEEDED(m_pDshowContext->queryinterface(IID_INncam, (void**)&spIcam)))
		{
			BOOL bHFlip;
			if  (SUCCEEDED(spIcam->get_HFlip(&bHFlip)))
				spIcam->put_HFlip(!bHFlip);
		}
	}
}

void CMainFrame::OnExampleFlipvertical()
{
	if (m_pDshowContext)
	{
		CComPtr<INncam> spIcam;
		if (SUCCEEDED(m_pDshowContext->queryinterface(IID_INncam, (void**)&spIcam)))
		{
			BOOL bHFlip;
			if  (SUCCEEDED(spIcam->get_VFlip(&bHFlip)))
				spIcam->put_VFlip(!bHFlip);
		}
	}
}

void CMainFrame::OnPreviewSnapshot()
{
	if (m_pDshowContext)
		m_pDshowContext->preview_snapshot(L"start.bmp");
}

void CMainFrame::OnStillimageSnapshot()
{
	if (m_pDshowContext)
		m_pDshowContext->stillimage_snapshot(L"stillimage.bmp");
}

void CMainFrame::OnFramerate()
{
	if (m_pDshowContext)
	{
		double fr = m_pDshowContext->get_framerate();
		wchar_t txt[32];
		swprintf(txt, L"Frame rate: %.1f", fr);
		AfxMessageBox(txt);
	}
}

void CMainFrame::OnSn()
{
	if (m_pDshowContext)
	{
		CComPtr<INncamSerialNumber> spISN;
		m_pDshowContext->queryinterface(IID_INncamSerialNumber, (void**)&spISN);
		if (spISN)
		{
			char sn[32];
			if (SUCCEEDED(spISN->get_SerialNumber(sn)))
				MessageBoxA(m_hWnd, sn, "SN", MB_OK);
		}
	}
}

void CMainFrame::OnUpdatePreviewSnapshot(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pDshowContext ? TRUE : FALSE);
}

void CMainFrame::OnUpdateStillimageSnapshot(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;
	if (m_pDshowContext)
		bEnable = m_pDshowContext->stillimage_supported();
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnUpdateCapture(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((m_pDshowContext && (!m_pDshowContext->IsCapturing())) ? TRUE : FALSE);
}

void CMainFrame::OnUpdateStopCapture(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((m_pDshowContext && m_pDshowContext->IsCapturing()) ? TRUE : FALSE);
}