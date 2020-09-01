#pragma once

#include "toupcam.h"

extern HToupCam g_hCam;
extern ToupcamInstV2 g_cameras[TOUPCAM_MAX];
extern int g_cameraCnt;
extern int g_snapCount;
extern int g_ROITestCount;
extern volatile bool g_bSnapFinished;
extern bool g_bSnapTesting;
extern volatile bool g_bResChangedFinished;
extern volatile bool g_bImageShoot;
extern volatile bool g_bROITesting;
extern volatile bool g_bROITest_SnapFinished;
extern volatile bool g_bOpenCloseFinished;
extern bool g_bROITest_StartSnap;
extern bool g_bTriggerTesting;
extern bool g_bEnableCheckBlack;
extern bool g_bBlack;
extern CString g_snapDir;

CString GetAppTimeDir(const CString& header);