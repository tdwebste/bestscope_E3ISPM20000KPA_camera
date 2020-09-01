#include <mex.h>
#include "toupcam.h"

#define PARAM_CLOSE        0x0000
#define PARAM_SNAP         0x0001
#define PARAM_AEXP         0x0002
#define PARAM_EXPTIME      0x0003
#define PARAM_WBONEPUSH    0x0004
#define PARAM_TEMP         0x0005
#define PARAM_TINT         0x0006
#define PARAM_HUE          0x0007
#define PARAM_SATURATION   0x0008
#define PARAM_BRIGHTNESS   0x0009
#define PARAM_CONTRAST     0x000a
#define PARAM_GAMMA        0x000b
#define PARAM_WBDFT        0x000c
#define PARAM_COLORDFT     0x000d
#define PARAM_BBONEPUSH    0x000e
#define PARAM_ROFFSET      0x000f
#define PARAM_GOFFSET      0x0010
#define PARAM_BOFFSET      0x0011
#define PARAM_BBDFT        0x0012
#define PARAM_CHROME       0x0013
#define PARAM_FFCONEPUSH   0x0014
#define PARAM_DFCONEPUSH   0x0015
#define PARAM_POWERSUPPLY  0x0016
#define PARAM_VFLIP        0x0017
#define PARAM_HFLIP        0x0018
#define PARAM_ROTATE       0x0019
#define PARAM_SHARPENING   0x001a
#define PARAM_BITDEPTH     0x001b
#define PARAM_BINNING      0x001c
#define PARAM_LINEAR       0x001d
#define PARAM_CURVE        0x001e

static unsigned char* g_imgData = nullptr;
static HToupcam g_hCam = nullptr;
static int g_captureIndex = 0;

static void saveDataAsBmp(const void* pData, const BITMAPINFOHEADER* pHeader)
{
    char str[256];
    sprintf(str, "%04d.bmp", ++g_captureIndex);
    FILE* fp = fopen(str, "wb");
    if (fp)
    {
        BITMAPFILEHEADER fh = { 0 };
        fh.bfType = 'M' << 8 | 'B';
        fh.bfSize = sizeof(fh) + sizeof(BITMAPINFOHEADER) + pHeader->biSizeImage;
        fh.bfOffBits = sizeof(fh) + sizeof(BITMAPINFOHEADER);
        fwrite(&fh, sizeof(fh), 1, fp);
        fwrite(pHeader, sizeof(BITMAPINFOHEADER), 1, fp);
        fwrite(pData, pHeader->biSizeImage, 1, fp);
        fclose(fp);
    }
}

static void DataCallback(const void* pData, const BITMAPINFOHEADER* pHeader, BOOL bSnap, void* pCallbackCtx)
{
    if (bSnap)
        saveDataAsBmp(pData, pHeader);
    else if (g_imgData)
    {
        if (TDIBWIDTHBYTES(pHeader->biWidth * 24) == pHeader->biWidth * 3)          // row pitch
            memcpy(g_imgData, pData, pHeader->biWidth * pHeader->biHeight * 3);
        else
        {
            unsigned char* pTmp = (unsigned char*)pData;
            for (int i = 0; i < pHeader->biHeight; ++i)
                memcpy(g_imgData + pHeader->biWidth * 3 * i, pTmp + TDIBWIDTHBYTES(pHeader->biWidth * 24) * i, pHeader->biWidth * 3);
        }
    }
}

static void EnumCameras(mxArray* plhs[])
{
    plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
    double* y = mxGetPr(plhs[0]);
    ToupcamDeviceV2 ti[TOUPCAM_MAX];
    int ret = Toupcam_EnumV2(ti);
    *y = ret;
    if (ret)
    {
        mwSize dims[2] = {1, ret};
        const char* field_names[] = {"name", "id"};
        plhs[1] = mxCreateStructArray(2, dims, 2, field_names);
        int name_field = mxGetFieldNumber(plhs[1], "name");
        int id_field = mxGetFieldNumber(plhs[1], "id");
        for (int i = 0; i < ret; i++)
        {
            char name[64] = { 0 }, id[64] = { 0 };
            wcstombs(name, ti[i].displayname, sizeof(name));
            wcstombs(id, ti[i].id, sizeof(id));
            mxSetFieldByNumber(plhs[1], i, name_field, mxCreateString(name));
            mxSetFieldByNumber(plhs[1], i, id_field, mxCreateString(id));
        }
    }
    else
    {
        plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);
    }
}

static int Init(unsigned nRes, unsigned short nSpeed, char* ID)
{
    wchar_t id[64] = { 0 };
    if (ID && ID[0])
        mbstowcs(id, ID, sizeof(id));
    g_hCam = Toupcam_Open(id);
    if (g_hCam)
    {
        Toupcam_put_Speed(g_hCam, nSpeed);
        Toupcam_put_eSize(g_hCam, nRes);
        Toupcam_StartPushMode(g_hCam, DataCallback, nullptr);
        return S_OK;
    }
    return S_FALSE;
}

static void ConstParams(mxArray* plhs[])
{
    const char* field_names[] = {"TOUPCAM_TEMP_DEF", "TOUPCAM_TEMP_MIN", "TOUPCAM_TEMP_MAX", "TOUPCAM_TINT_DEF", "TOUPCAM_TINT_MIN",
                                "TOUPCAM_TINT_MAX", "TOUPCAM_HUE_DEF", "TOUPCAM_HUE_MIN", "TOUPCAM_HUE_MAX", "TOUPCAM_SATURATION_DEF", "TOUPCAM_SATURATION_MIN",
                                "TOUPCAM_SATURATION_MAX", "TOUPCAM_BRIGHTNESS_DEF", "TOUPCAM_BRIGHTNESS_MIN", "TOUPCAM_BRIGHTNESS_MAX", "TOUPCAM_CONTRAST_DEF",
                                "TOUPCAM_CONTRAST_MIN", "TOUPCAM_CONTRAST_MAX", "TOUPCAM_GAMMA_DEF", "TOUPCAM_GAMMA_MIN", "TOUPCAM_GAMMA_MAX", "TOUPCAM_AETARGET_DEF",
                                "TOUPCAM_AETARGET_MIN", "TOUPCAM_AETARGET_MAX", "TOUPCAM_WBGAIN_DEF", "TOUPCAM_WBGAIN_MIN", "TOUPCAM_WBGAIN_MAX", "TOUPCAM_BLACKLEVEL_MIN",
                                "TOUPCAM_BLACKLEVEL8_MAX", "TOUPCAM_BLACKLEVEL10_MAX", "TOUPCAM_BLACKLEVEL12_MAX", "TOUPCAM_BLACKLEVEL14_MAX", "TOUPCAM_BLACKLEVEL16_MAX",
                                "TOUPCAM_SHARPENING_STRENGTH_DEF", "TOUPCAM_SHARPENING_STRENGTH_MIN", "TOUPCAM_SHARPENING_STRENGTH_MAX", "TOUPCAM_SHARPENING_RADIUS_DEF",
                                "TOUPCAM_SHARPENING_RADIUS_MIN", "TOUPCAM_SHARPENING_RADIUS_MAX", "TOUPCAM_SHARPENING_THRESHOLD_DEF", "TOUPCAM_SHARPENING_THRESHOLD_MIN",
                                "TOUPCAM_SHARPENING_THRESHOLD_MAX"};
    int TOUPCAM_TEMP_DEF_field, TOUPCAM_TEMP_MIN_field, TOUPCAM_TEMP_MAX_field, TOUPCAM_TINT_DEF_field, TOUPCAM_TINT_MIN_field,
                                TOUPCAM_TINT_MAX_field, TOUPCAM_HUE_DEF_field, TOUPCAM_HUE_MIN_field, TOUPCAM_HUE_MAX_field, TOUPCAM_SATURATION_DEF_field,
                                TOUPCAM_SATURATION_MIN_field,TOUPCAM_SATURATION_MAX_field, TOUPCAM_BRIGHTNESS_DEF_field, TOUPCAM_BRIGHTNESS_MIN_field,
                                TOUPCAM_BRIGHTNESS_MAX_field, TOUPCAM_CONTRAST_DEF_field, TOUPCAM_CONTRAST_MIN_field, TOUPCAM_CONTRAST_MAX_field,
                                TOUPCAM_GAMMA_DEF_field, TOUPCAM_GAMMA_MIN_field, TOUPCAM_GAMMA_MAX_field, TOUPCAM_AETARGET_DEF_field, TOUPCAM_AETARGET_MIN_field,
                                TOUPCAM_AETARGET_MAX_field, TOUPCAM_WBGAIN_DEF_field, TOUPCAM_WBGAIN_MIN_field, TOUPCAM_WBGAIN_MAX_field,
                                TOUPCAM_BLACKLEVEL_MIN_field, TOUPCAM_BLACKLEVEL8_MAX_field, TOUPCAM_BLACKLEVEL10_MAX_field, TOUPCAM_BLACKLEVEL12_MAX_field,
                                TOUPCAM_BLACKLEVEL14_MAX_field, TOUPCAM_BLACKLEVEL16_MAX_field, TOUPCAM_SHARPENING_STRENGTH_DEF_field, TOUPCAM_SHARPENING_STRENGTH_MIN_field,
                                TOUPCAM_SHARPENING_STRENGTH_MAX_field, TOUPCAM_SHARPENING_RADIUS_DEF_field, TOUPCAM_SHARPENING_RADIUS_MIN_field,
                                TOUPCAM_SHARPENING_RADIUS_MAX_field, TOUPCAM_SHARPENING_THRESHOLD_DEF_field, TOUPCAM_SHARPENING_THRESHOLD_MIN_field,
                                TOUPCAM_SHARPENING_THRESHOLD_MAX_field;
    plhs[3] = mxCreateStructMatrix(1, 1, 42, field_names);
    TOUPCAM_TEMP_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_TEMP_DEF");
    TOUPCAM_TEMP_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_TEMP_MIN");
    TOUPCAM_TEMP_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_TEMP_MAX");
    TOUPCAM_TINT_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_TINT_DEF");
    TOUPCAM_TINT_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_TINT_MIN");
    TOUPCAM_TINT_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_TINT_MAX");
    TOUPCAM_HUE_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_HUE_DEF");
    TOUPCAM_HUE_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_HUE_MIN");
    TOUPCAM_HUE_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_HUE_MAX");
    TOUPCAM_SATURATION_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SATURATION_DEF");
    TOUPCAM_SATURATION_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SATURATION_MIN");
    TOUPCAM_SATURATION_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SATURATION_MAX");
    TOUPCAM_BRIGHTNESS_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_BRIGHTNESS_DEF");
    TOUPCAM_BRIGHTNESS_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_BRIGHTNESS_MIN");
    TOUPCAM_BRIGHTNESS_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_BRIGHTNESS_MAX");
    TOUPCAM_CONTRAST_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_CONTRAST_DEF");
    TOUPCAM_CONTRAST_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_CONTRAST_MIN");
    TOUPCAM_CONTRAST_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_CONTRAST_MAX");
    TOUPCAM_GAMMA_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_GAMMA_DEF");
    TOUPCAM_GAMMA_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_GAMMA_MIN");
    TOUPCAM_GAMMA_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_GAMMA_MAX");
    TOUPCAM_AETARGET_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_AETARGET_DEF");
    TOUPCAM_AETARGET_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_AETARGET_MIN");
    TOUPCAM_AETARGET_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_AETARGET_MAX");
    TOUPCAM_WBGAIN_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_WBGAIN_DEF");
    TOUPCAM_WBGAIN_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_WBGAIN_MIN");
    TOUPCAM_WBGAIN_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_WBGAIN_MAX");
    TOUPCAM_BLACKLEVEL_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_BLACKLEVEL_MIN");
    TOUPCAM_BLACKLEVEL8_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_BLACKLEVEL8_MAX");
    TOUPCAM_BLACKLEVEL10_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_BLACKLEVEL10_MAX");
    TOUPCAM_BLACKLEVEL12_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_BLACKLEVEL12_MAX");
    TOUPCAM_BLACKLEVEL14_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_BLACKLEVEL14_MAX");
    TOUPCAM_BLACKLEVEL16_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_BLACKLEVEL16_MAX");
    TOUPCAM_SHARPENING_STRENGTH_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SHARPENING_STRENGTH_DEF");
    TOUPCAM_SHARPENING_STRENGTH_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SHARPENING_STRENGTH_MIN");
    TOUPCAM_SHARPENING_STRENGTH_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SHARPENING_STRENGTH_MAX");
    TOUPCAM_SHARPENING_RADIUS_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SHARPENING_RADIUS_DEF");
    TOUPCAM_SHARPENING_RADIUS_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SHARPENING_RADIUS_MIN");
    TOUPCAM_SHARPENING_RADIUS_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SHARPENING_RADIUS_MAX");
    TOUPCAM_SHARPENING_THRESHOLD_DEF_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SHARPENING_THRESHOLD_DEF");
    TOUPCAM_SHARPENING_THRESHOLD_MIN_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SHARPENING_THRESHOLD_MIN");
    TOUPCAM_SHARPENING_THRESHOLD_MAX_field = mxGetFieldNumber(plhs[3], "TOUPCAM_SHARPENING_THRESHOLD_MAX");
    mxArray *TOUPCAM_TEMP_DEF_value, *TOUPCAM_TEMP_MIN_value, *TOUPCAM_TEMP_MAX_value, *TOUPCAM_TINT_DEF_value, *TOUPCAM_TINT_MIN_value,
            *TOUPCAM_TINT_MAX_value, *TOUPCAM_HUE_DEF_value, *TOUPCAM_HUE_MIN_value, *TOUPCAM_HUE_MAX_value, *TOUPCAM_SATURATION_DEF_value,
            *TOUPCAM_SATURATION_MIN_value, *TOUPCAM_SATURATION_MAX_value, *TOUPCAM_BRIGHTNESS_DEF_value, *TOUPCAM_BRIGHTNESS_MIN_value,
            *TOUPCAM_BRIGHTNESS_MAX_value, *TOUPCAM_CONTRAST_DEF_value, *TOUPCAM_CONTRAST_MIN_value, *TOUPCAM_CONTRAST_MAX_value,
            *TOUPCAM_GAMMA_DEF_value, *TOUPCAM_GAMMA_MIN_value, *TOUPCAM_GAMMA_MAX_value, *TOUPCAM_AETARGET_DEF_value, *TOUPCAM_AETARGET_MIN_value,
            *TOUPCAM_AETARGET_MAX_value, *TOUPCAM_WBGAIN_DEF_value, *TOUPCAM_WBGAIN_MIN_value, *TOUPCAM_WBGAIN_MAX_value,
            *TOUPCAM_BLACKLEVEL_MIN_value, *TOUPCAM_BLACKLEVEL8_MAX_value, *TOUPCAM_BLACKLEVEL10_MAX_value, *TOUPCAM_BLACKLEVEL12_MAX_value,
            *TOUPCAM_BLACKLEVEL14_MAX_value, *TOUPCAM_BLACKLEVEL16_MAX_value, *TOUPCAM_SHARPENING_STRENGTH_DEF_value, *TOUPCAM_SHARPENING_STRENGTH_MIN_value,
            *TOUPCAM_SHARPENING_STRENGTH_MAX_value, *TOUPCAM_SHARPENING_RADIUS_DEF_value, *TOUPCAM_SHARPENING_RADIUS_MIN_value,
            *TOUPCAM_SHARPENING_RADIUS_MAX_value, *TOUPCAM_SHARPENING_THRESHOLD_DEF_value, *TOUPCAM_SHARPENING_THRESHOLD_MIN_value,
            *TOUPCAM_SHARPENING_THRESHOLD_MAX_value;
    TOUPCAM_TEMP_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_TEMP_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_TEMP_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_TINT_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_TINT_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_TINT_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_HUE_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_HUE_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_HUE_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SATURATION_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SATURATION_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SATURATION_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_BRIGHTNESS_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_BRIGHTNESS_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_BRIGHTNESS_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_CONTRAST_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_CONTRAST_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_CONTRAST_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_GAMMA_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_GAMMA_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_GAMMA_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_AETARGET_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_AETARGET_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_AETARGET_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_WBGAIN_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_WBGAIN_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_WBGAIN_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_BLACKLEVEL_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_BLACKLEVEL8_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_BLACKLEVEL10_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_BLACKLEVEL12_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_BLACKLEVEL14_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_BLACKLEVEL16_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SHARPENING_STRENGTH_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SHARPENING_STRENGTH_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SHARPENING_STRENGTH_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SHARPENING_RADIUS_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SHARPENING_RADIUS_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SHARPENING_RADIUS_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SHARPENING_THRESHOLD_DEF_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SHARPENING_THRESHOLD_MIN_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    TOUPCAM_SHARPENING_THRESHOLD_MAX_value = mxCreateDoubleMatrix(1, 1, mxREAL);
    *mxGetPr(TOUPCAM_TEMP_DEF_value) = static_cast<double>(TOUPCAM_TEMP_DEF);
    *mxGetPr(TOUPCAM_TEMP_MIN_value) = static_cast<double>(TOUPCAM_TEMP_MIN);
    *mxGetPr(TOUPCAM_TEMP_MAX_value) = static_cast<double>(TOUPCAM_TEMP_MAX);
    *mxGetPr(TOUPCAM_TINT_DEF_value) = static_cast<double>( TOUPCAM_TINT_DEF);
    *mxGetPr(TOUPCAM_TINT_MIN_value) = static_cast<double>(TOUPCAM_TINT_MIN);
    *mxGetPr(TOUPCAM_TINT_MAX_value) = static_cast<double>(TOUPCAM_TINT_MAX);
    *mxGetPr(TOUPCAM_HUE_DEF_value) = static_cast<double>(TOUPCAM_HUE_DEF);
    *mxGetPr(TOUPCAM_HUE_MIN_value) = static_cast<double>(TOUPCAM_HUE_MIN);
    *mxGetPr(TOUPCAM_HUE_MAX_value) = static_cast<double>(TOUPCAM_HUE_MAX);
    *mxGetPr(TOUPCAM_SATURATION_DEF_value) = static_cast<double>(TOUPCAM_SATURATION_DEF);
    *mxGetPr(TOUPCAM_SATURATION_MIN_value) = static_cast<double>(TOUPCAM_SATURATION_MIN);
    *mxGetPr(TOUPCAM_SATURATION_MAX_value) = static_cast<double>(TOUPCAM_SATURATION_MAX);
    *mxGetPr(TOUPCAM_BRIGHTNESS_DEF_value) = static_cast<double>(TOUPCAM_BRIGHTNESS_DEF);
    *mxGetPr(TOUPCAM_BRIGHTNESS_MIN_value) = static_cast<double>(TOUPCAM_BRIGHTNESS_MIN);
    *mxGetPr(TOUPCAM_BRIGHTNESS_MAX_value) = static_cast<double>(TOUPCAM_BRIGHTNESS_MAX);
    *mxGetPr(TOUPCAM_CONTRAST_DEF_value) = static_cast<double>(TOUPCAM_CONTRAST_DEF);
    *mxGetPr(TOUPCAM_CONTRAST_MIN_value) = static_cast<double>(TOUPCAM_CONTRAST_MIN);
    *mxGetPr(TOUPCAM_CONTRAST_MAX_value) = static_cast<double>(TOUPCAM_CONTRAST_MAX);
    *mxGetPr(TOUPCAM_GAMMA_DEF_value) = static_cast<double>(TOUPCAM_GAMMA_DEF);
    *mxGetPr(TOUPCAM_GAMMA_MIN_value) = static_cast<double>(TOUPCAM_GAMMA_MIN);
    *mxGetPr(TOUPCAM_GAMMA_MAX_value) = static_cast<double>(TOUPCAM_GAMMA_MAX);
    *mxGetPr(TOUPCAM_AETARGET_DEF_value) = static_cast<double>(TOUPCAM_AETARGET_DEF);
    *mxGetPr(TOUPCAM_AETARGET_MIN_value) = static_cast<double>(TOUPCAM_AETARGET_MIN);
    *mxGetPr(TOUPCAM_AETARGET_MAX_value) = static_cast<double>(TOUPCAM_AETARGET_MAX);
    *mxGetPr(TOUPCAM_WBGAIN_DEF_value) = static_cast<double>(TOUPCAM_WBGAIN_DEF);
    *mxGetPr(TOUPCAM_WBGAIN_MIN_value) = static_cast<double>(TOUPCAM_WBGAIN_MIN);
    *mxGetPr(TOUPCAM_WBGAIN_MAX_value) = static_cast<double>(TOUPCAM_WBGAIN_MAX);
    *mxGetPr(TOUPCAM_BLACKLEVEL_MIN_value) = static_cast<double>(TOUPCAM_BLACKLEVEL_MIN );
    *mxGetPr(TOUPCAM_BLACKLEVEL8_MAX_value) = static_cast<double>( TOUPCAM_BLACKLEVEL8_MAX);
    *mxGetPr(TOUPCAM_BLACKLEVEL10_MAX_value) = static_cast<double>(TOUPCAM_BLACKLEVEL10_MAX);
    *mxGetPr(TOUPCAM_BLACKLEVEL12_MAX_value) = static_cast<double>(TOUPCAM_BLACKLEVEL12_MAX);
    *mxGetPr(TOUPCAM_BLACKLEVEL14_MAX_value) = static_cast<double>(TOUPCAM_BLACKLEVEL14_MAX);
    *mxGetPr(TOUPCAM_BLACKLEVEL16_MAX_value) = static_cast<double>(TOUPCAM_BLACKLEVEL16_MAX);
    *mxGetPr(TOUPCAM_SHARPENING_STRENGTH_DEF_value) = static_cast<double>(TOUPCAM_SHARPENING_STRENGTH_DEF);
    *mxGetPr(TOUPCAM_SHARPENING_STRENGTH_MIN_value) = static_cast<double>(TOUPCAM_SHARPENING_STRENGTH_MIN);
    *mxGetPr(TOUPCAM_SHARPENING_STRENGTH_MAX_value) = static_cast<double>(TOUPCAM_SHARPENING_STRENGTH_MAX);
    *mxGetPr(TOUPCAM_SHARPENING_RADIUS_DEF_value) = static_cast<double>(TOUPCAM_SHARPENING_RADIUS_DEF);
    *mxGetPr(TOUPCAM_SHARPENING_RADIUS_MIN_value) = static_cast<double>(TOUPCAM_SHARPENING_RADIUS_MIN);
    *mxGetPr(TOUPCAM_SHARPENING_RADIUS_MAX_value) = static_cast<double>(TOUPCAM_SHARPENING_RADIUS_MAX);
    *mxGetPr(TOUPCAM_SHARPENING_THRESHOLD_DEF_value) = static_cast<double>(TOUPCAM_SHARPENING_THRESHOLD_DEF);
    *mxGetPr(TOUPCAM_SHARPENING_THRESHOLD_MIN_value) = static_cast<double>(TOUPCAM_SHARPENING_THRESHOLD_MIN);
    *mxGetPr(TOUPCAM_SHARPENING_THRESHOLD_MAX_value) = static_cast<double>(TOUPCAM_SHARPENING_THRESHOLD_MAX);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_TEMP_DEF_field, TOUPCAM_TEMP_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_TEMP_MIN_field, TOUPCAM_TEMP_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_TEMP_MAX_field, TOUPCAM_TEMP_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_TINT_DEF_field, TOUPCAM_TINT_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_TINT_MIN_field, TOUPCAM_TINT_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_TINT_MAX_field, TOUPCAM_TINT_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_HUE_DEF_field, TOUPCAM_HUE_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_HUE_MIN_field, TOUPCAM_HUE_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_HUE_MAX_field, TOUPCAM_HUE_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SATURATION_DEF_field, TOUPCAM_SATURATION_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SATURATION_MIN_field, TOUPCAM_SATURATION_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SATURATION_MAX_field, TOUPCAM_SATURATION_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_BRIGHTNESS_DEF_field, TOUPCAM_BRIGHTNESS_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_BRIGHTNESS_MIN_field, TOUPCAM_BRIGHTNESS_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_BRIGHTNESS_MAX_field, TOUPCAM_BRIGHTNESS_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_CONTRAST_DEF_field, TOUPCAM_CONTRAST_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_CONTRAST_MIN_field, TOUPCAM_CONTRAST_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_CONTRAST_MAX_field, TOUPCAM_CONTRAST_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_GAMMA_DEF_field, TOUPCAM_GAMMA_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_GAMMA_MIN_field, TOUPCAM_GAMMA_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_GAMMA_MAX_field, TOUPCAM_GAMMA_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_AETARGET_DEF_field, TOUPCAM_AETARGET_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_AETARGET_MIN_field, TOUPCAM_AETARGET_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_AETARGET_MAX_field, TOUPCAM_AETARGET_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_WBGAIN_DEF_field, TOUPCAM_WBGAIN_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_WBGAIN_MIN_field, TOUPCAM_WBGAIN_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_WBGAIN_MAX_field, TOUPCAM_WBGAIN_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_BLACKLEVEL_MIN_field, TOUPCAM_BLACKLEVEL_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_BLACKLEVEL8_MAX_field, TOUPCAM_BLACKLEVEL8_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_BLACKLEVEL10_MAX_field, TOUPCAM_BLACKLEVEL10_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_BLACKLEVEL12_MAX_field, TOUPCAM_BLACKLEVEL12_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_BLACKLEVEL14_MAX_field, TOUPCAM_BLACKLEVEL14_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_BLACKLEVEL16_MAX_field, TOUPCAM_BLACKLEVEL16_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SHARPENING_STRENGTH_DEF_field, TOUPCAM_SHARPENING_STRENGTH_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SHARPENING_STRENGTH_MIN_field, TOUPCAM_SHARPENING_STRENGTH_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SHARPENING_STRENGTH_MAX_field, TOUPCAM_SHARPENING_STRENGTH_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SHARPENING_RADIUS_DEF_field, TOUPCAM_SHARPENING_RADIUS_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SHARPENING_RADIUS_MIN_field, TOUPCAM_SHARPENING_RADIUS_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SHARPENING_RADIUS_MAX_field, TOUPCAM_SHARPENING_RADIUS_MAX_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SHARPENING_THRESHOLD_DEF_field, TOUPCAM_SHARPENING_THRESHOLD_DEF_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SHARPENING_THRESHOLD_MIN_field, TOUPCAM_SHARPENING_THRESHOLD_MIN_value);
    mxSetFieldByNumber(plhs[3], 0, TOUPCAM_SHARPENING_THRESHOLD_MAX_field, TOUPCAM_SHARPENING_THRESHOLD_MAX_value);
}

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
    if (nrhs == 0)
        EnumCameras(plhs);
    else if (nrhs == 4)
    {
        unsigned nRes = static_cast<unsigned>(*(mxGetPr(prhs[0])));
        unsigned short nSpeed = static_cast<unsigned short>(*(mxGetPr(prhs[1])));
        char ID[64] = { 0 };
        mxGetString(prhs[2], ID, sizeof(ID));
        if (S_OK == Init(nRes, nSpeed, ID))
        {
            int srcW = 0, srcH = 0;
            Toupcam_get_Resolution(g_hCam , 0, &srcW, &srcH);
            mwSize dims[2] = {srcW * 3, srcH};
            plhs[0] = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);
            g_imgData = static_cast<unsigned char*>(mxGetData(plhs[0]));
            mexMakeMemoryPersistent(g_imgData);
            plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);
            plhs[2] = mxCreateDoubleMatrix(1, 1, mxREAL);
            double* w = mxGetPr(plhs[1]);
            double* h = mxGetPr(plhs[2]);
            *w = static_cast<double>(srcW);
            *h = static_cast<double>(srcH);
            ConstParams(plhs);
        }
    }
    else if (nrhs == 2)
    {
        if (nullptr == g_hCam)
            return;
        const int value = static_cast<int>(*mxGetPr(prhs[1]));
        switch (static_cast<int>(*mxGetPr(prhs[0])))
        {
            case PARAM_CLOSE:
                Toupcam_Close(g_hCam);
                g_hCam = nullptr;
                break;
            case PARAM_SNAP:
                Toupcam_Snap(g_hCam, 0);
                break;
            case PARAM_AEXP:
                {
                    BOOL bAutoExposure;
                    unsigned nTime;
                    plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
                    plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);
                    double* y = mxGetPr(plhs[0]);
                    double* time = mxGetPr(plhs[1]);
                    Toupcam_get_AutoExpoEnable(g_hCam, &bAutoExposure);
                    Toupcam_put_AutoExpoEnable(g_hCam, !bAutoExposure);
                    Toupcam_get_ExpoTime(g_hCam, &nTime);
                    *y = !bAutoExposure;
                    *time = nTime;
                }
                break;
            case PARAM_EXPTIME:
                Toupcam_put_ExpoTime(g_hCam, static_cast<unsigned>(value));
                break;
            case PARAM_WBONEPUSH:
                {
                    int nTemp, nTint;
                    plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
                    plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);
                    double* temp = mxGetPr(plhs[0]);
                    double* tint = mxGetPr(plhs[1]);
                    Toupcam_AwbOnePush(g_hCam, nullptr, nullptr);
                    Toupcam_get_TempTint(g_hCam, &nTemp, &nTint);
                    *temp = (double)nTemp;
                    *tint = (double)nTint;
                }
                break;
            case PARAM_TEMP:
                {
                    int nTemp = 0, nTint = 0;
                    Toupcam_get_TempTint(g_hCam, &nTemp, &nTint);
                    Toupcam_put_TempTint(g_hCam, value, nTint);
                }
                break;
            case PARAM_TINT:
                {
                    int nTemp = 0, nTint = 0;
                    Toupcam_get_TempTint(g_hCam, &nTemp, &nTint);
                    Toupcam_put_TempTint(g_hCam, nTemp, value);
                }
                break;
            case PARAM_HUE:
                Toupcam_put_Hue(g_hCam, value);
                break;
            case PARAM_SATURATION:
                Toupcam_put_Saturation(g_hCam, value);
                break;
            case PARAM_BRIGHTNESS:
                Toupcam_put_Brightness(g_hCam, value);
                break;
            case PARAM_CONTRAST:
                Toupcam_put_Contrast(g_hCam, value);
                break;
            case PARAM_GAMMA:
                Toupcam_put_Gamma(g_hCam, value);
                break;
            case PARAM_WBDFT:
                Toupcam_put_TempTint(g_hCam, TOUPCAM_TEMP_DEF, TOUPCAM_TINT_DEF);
                break;
            case PARAM_COLORDFT:
                Toupcam_put_Hue(g_hCam, TOUPCAM_HUE_DEF);
                Toupcam_put_Saturation(g_hCam, TOUPCAM_SATURATION_DEF);
                Toupcam_put_Brightness(g_hCam, TOUPCAM_BRIGHTNESS_DEF);
                Toupcam_put_Contrast(g_hCam, TOUPCAM_CONTRAST_DEF);
                Toupcam_put_Gamma(g_hCam, TOUPCAM_GAMMA_DEF);
                break;
            case PARAM_BBONEPUSH:
                {
                    unsigned short aSub[3] = { 0 };
                    Toupcam_AbbOnePush(g_hCam, nullptr, nullptr);
                    Toupcam_get_BlackBalance(g_hCam, aSub);
                    plhs[0] = mxCreateDoubleMatrix(1, 3, mxREAL);
                    double* plaSub = mxGetPr(plhs[0]);
                    plaSub[0] = aSub[0];
                    plaSub[1] = aSub[1];
                    plaSub[2] = aSub[2];
                }
                break;
            case PARAM_ROFFSET:
                {
                    unsigned short aSub[3] = { 0 };
                    Toupcam_get_BlackBalance(g_hCam, aSub);
                    aSub[0] = value;
                    Toupcam_put_BlackBalance(g_hCam, aSub);
                    Toupcam_get_BlackBalance(g_hCam, aSub);
                }
                break;
            case PARAM_GOFFSET:
                {
                    unsigned short aSub[3] = { 0 };
                    Toupcam_get_BlackBalance(g_hCam, aSub);
                    aSub[1] = value;
                    Toupcam_put_BlackBalance(g_hCam, aSub);
                }
                break;
            case PARAM_BOFFSET:
                {
                    unsigned short aSub[3] = { 0 };
                    Toupcam_get_BlackBalance(g_hCam, aSub);
                    aSub[2] = value;
                    Toupcam_put_BlackBalance(g_hCam, aSub);
                }
                break;
            case PARAM_BBDFT:
                {
                    unsigned short aSub[3] = { 0 };
                    Toupcam_put_BlackBalance(g_hCam, aSub);
                }
                break;
            case PARAM_CHROME:
                Toupcam_put_Chrome(g_hCam, value);
                break;
            case PARAM_FFCONEPUSH:
                Toupcam_FfcOnePush(g_hCam);
                break;
            case PARAM_DFCONEPUSH:
                Toupcam_DfcOnePush(g_hCam);
                break;
            case PARAM_POWERSUPPLY:
                Toupcam_put_HZ(g_hCam, value);
                break;
            case PARAM_VFLIP:
                {
                    int bVFlip = 0;
                    Toupcam_get_VFlip(g_hCam, &bVFlip);  /* vertical flip */
                    Toupcam_put_VFlip(g_hCam, !bVFlip);
                }
                break;
            case PARAM_HFLIP:
                {
                    int bHFlip = 0;
                    Toupcam_get_HFlip(g_hCam, &bHFlip);  /* vertical flip */
                    Toupcam_put_HFlip(g_hCam, !bHFlip);
                }
                break;
            case PARAM_ROTATE:
                {
                    int iValue = 0;
                    Toupcam_get_Option(g_hCam, TOUPCAM_OPTION_ROTATE, &iValue);
                    iValue = 180 - iValue;
                    Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_ROTATE, iValue);
                }
                break;
            case PARAM_SHARPENING:
                Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_SHARPENING, value);
                break;
            case PARAM_BITDEPTH:
                Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_BITDEPTH, value);
                break;
            case PARAM_BINNING:
                Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_BINNING, value);
                break;
            case PARAM_LINEAR:
                Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_LINEAR, value);
                break;
            case PARAM_CURVE:
                Toupcam_put_Option(g_hCam, TOUPCAM_OPTION_CURVE, value);
                break;
            default:
                break;
        }
    }
    else
    {
        mexErrMsgIdAndTxt("MATLAB:mexFun:InvalidInput", "Invalid Input.");
    }
}
