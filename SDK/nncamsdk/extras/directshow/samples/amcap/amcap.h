//------------------------------------------------------------------------------
// File: AMCap.h
//
// Desc: DirectShow sample code - audio/video capture.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


// Macros
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }
#endif


extern "C"
{
    typedef BOOL (/* WINUSERAPI */ WINAPI *PUnregisterDeviceNotification)(
        IN HDEVNOTIFY Handle
        );

    typedef HDEVNOTIFY (/* WINUSERAPI */ WINAPI *PRegisterDeviceNotificationA)(
        IN HANDLE hRecipient,
        IN LPVOID NotificationFilter,
        IN DWORD Flags
        );

    typedef HDEVNOTIFY (/* WINUSERAPI */ WINAPI *PRegisterDeviceNotificationW)(
        IN HANDLE hRecipient,
        IN LPVOID NotificationFilter,
        IN DWORD Flags
        );
}

#define PRegisterDeviceNotification  PRegisterDeviceNotificationW


//
// Resource constants
//
#define ID_APP      1000

/* Menu Items */
#define MENU_EXIT           4
#define MENU_SET_CAP_FILE   5
#define MENU_ALLOC_CAP_FILE 6
#define MENU_START_CAP      7
#define MENU_STOP_CAP       8
#define MENU_CAP_CC         9
#define MENU_CAP_AUDIO      12
#define MENU_AUDIOFORMAT    13
#define MENU_FRAMERATE      14
#define MENU_PREVIEW        15
#define MENU_ADEVICE0       26
#define MENU_ADEVICE1       27
#define MENU_ADEVICE2       28
#define MENU_ADEVICE3       29
#define MENU_ADEVICE4       30
#define MENU_ADEVICE5       31
#define MENU_ADEVICE6       32
#define MENU_ADEVICE7       33
#define MENU_ADEVICE8       34
#define MENU_ADEVICE9       35
#define MENU_ABOUT          36
#define MENU_SAVE_CAP_FILE  37
#define MENU_NOMASTER       38
#define MENU_AUDIOMASTER    39
#define MENU_VIDEOMASTER    40
#define MENU_TIMELIMIT      41
#define MENU_DIALOG0        42
#define MENU_DIALOG1        43
#define MENU_DIALOG2        44
#define MENU_DIALOG3        45
#define MENU_DIALOG4        46
#define MENU_DIALOG5        47
#define MENU_DIALOG6        48
#define MENU_DIALOG7        49
#define MENU_DIALOG8        50
#define MENU_DIALOG9        51
#define MENU_DIALOGA        52
#define MENU_DIALOGB        53
#define MENU_DIALOGC        54
#define MENU_DIALOGD        55
#define MENU_DIALOGE        56
#define MENU_DIALOGF        57
#define MENU_MPEG2          58  // !!! more?
#define MENU_VDEVICE00      100
#define MENU_VDEVICE01      101
#define MENU_VDEVICE02      102
#define MENU_VDEVICE03      103
#define MENU_VDEVICE04      104
#define MENU_VDEVICE05      105
#define MENU_VDEVICE06      106
#define MENU_VDEVICE07      107
#define MENU_VDEVICE08      108
#define MENU_VDEVICE09      109
#define MENU_VDEVICE10      110
#define MENU_VDEVICE11      111
#define MENU_VDEVICE12      112
#define MENU_VDEVICE13      113
#define MENU_VDEVICE14      114
#define MENU_VDEVICE15      115
#define MENU_VDEVICE16      116
#define MENU_VDEVICE17      117
#define MENU_VDEVICE18      118
#define MENU_VDEVICE19      119
#define MENU_VDEVICE20      120
#define MENU_VDEVICE21      121
#define MENU_VDEVICE22      122
#define MENU_VDEVICE23      123
#define MENU_VDEVICE24      124
#define MENU_VDEVICE25      125
#define MENU_VDEVICE26      126
#define MENU_VDEVICE27      127
#define MENU_VDEVICE28      128
#define MENU_VDEVICE29      129
#define MENU_VDEVICE30      130
#define MENU_VDEVICE31      131
#define MENU_VDEVICE32      132
#define MENU_VDEVICE33      133
#define MENU_VDEVICE34      134
#define MENU_VDEVICE35      135
#define MENU_VDEVICE36      136
#define MENU_VDEVICE37      137
#define MENU_VDEVICE38      138
#define MENU_VDEVICE39      139

// Dialogs
#define IDD_ABOUT               600
#define IDD_AllocCapFileSpace   601
#define IDD_FrameRateDialog     602
#define IDD_PressAKeyDialog     603
#define IDD_TimeLimitDialog     604

// defines for dialogs
#define IDD_SetCapFileFree      400
#define IDD_SetCapFileSize      401
#define IDC_FRAMERATE           402
#define IDC_CAPFILENAME         403
#define IDC_TIMELIMIT           404
#define IDC_USETIMELIMIT        405
#define IDC_USEFRAMERATE        406

// window messages
#define WM_FGNOTIFY WM_USER+1
