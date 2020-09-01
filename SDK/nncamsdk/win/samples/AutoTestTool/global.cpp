#include "stdafx.h"
#include "global.h"

HToupCam g_hCam = nullptr;
ToupcamInstV2 g_cameras[TOUPCAM_MAX];
int g_cameraCnt = 0;
int g_snapCount = 0;
int g_ROITestCount = 0;
volatile bool g_bSnapFinished = false;
volatile bool g_bResChangedFinished = false;
volatile bool g_bImageShoot = false;
volatile bool g_bROITesting = false;
volatile bool g_bROITest_SnapFinished = false;
volatile bool g_bOpenCloseFinished = false;
bool g_bROITest_StartSnap = false;
bool g_bTriggerTesting = false;
bool g_bEnableCheckBlack = false;
bool g_bBlack = false;
bool g_bSnapTesting = false;
CString g_snapDir;

CString GetAppTimeDir(const CString& header)
{
	CTime tm = CTime::GetCurrentTime();
	CString strTime = tm.Format(_T("%Y%m%d%H%M%S"));
	TCHAR path[MAX_PATH] = { 0 };
	GetModuleFileName(nullptr, path, MAX_PATH);
	PathRemoveFileSpec(path);
	CString str = header + _T("_") + strTime;
	PathAppend(path, (LPCTSTR)str);
	str = path;
	return str;
}