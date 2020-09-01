#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _AFX_ALL_WARNINGS

#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif 

#include <afxcontrolbars.h>
#include <afxdlgs.h>
#include <afxdialogex.h>

#define MSG_CAMEVENT					(WM_APP + 1)
#define WM_USER_PREVIEW_CHANGE			(WM_USER + 1)
#define WM_USER_AUTO_EXPOSURE			(WM_USER + 2)
#define WM_USER_WHITE_BALANCE			(WM_USER + 3)
#define WM_USER_SNAP_COUNT				(WM_USER + 4)
#define WM_USER_ROI_TEST_ONE			(WM_USER + 5)
#define WM_USER_ROI_TEST_FINISHED		(WM_USER + 6)
#define WM_USER_RES_TEST_COUNT			(WM_USER + 7)
#define WM_USER_BITDEPTH_TEST_COUNT		(WM_USER + 8)
#define WM_USER_OPEN_CLOSE				(WM_USER + 9)
#define WM_USER_OPEN_CLOSE_TEST_COUNT	(WM_USER + 10)
#define WM_USER_TRIGGER_TEST_COUNT		(WM_USER + 11)

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
