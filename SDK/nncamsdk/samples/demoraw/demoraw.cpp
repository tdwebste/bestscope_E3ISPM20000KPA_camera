#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <conio.h>
#endif
#include "toupcam.h"

HToupcam g_hcam = NULL;
void* g_pImageData = NULL;
unsigned g_total = 0, g_totalstill = 0;

static void __stdcall EventCallback(unsigned nEvent, void* pCallbackCtx)
{
    if (TOUPCAM_EVENT_IMAGE == nEvent)
    {
        ToupcamFrameInfoV2 info = { 0 };
        HRESULT hr = Toupcam_PullImageV2(g_hcam, g_pImageData, 24, &info);
        if (FAILED(hr))
            printf("failed to pull image, hr = %08x\n", hr);
        else
            printf("pull image ok, total = %u, resolution = %u x %u\n", ++g_total, info.width, info.height);
    }
    else if (TOUPCAM_EVENT_STILLIMAGE == nEvent)
    {
		ToupcamFrameInfoV2 info = { 0 };
        HRESULT hr = Toupcam_PullStillImageV2(g_hcam, g_pImageData, 24, &info);
        if (FAILED(hr))
            printf("failed to pull still image, hr = %08x\n", hr);
        else
            printf("pull still image ok, total = %u, resolution = %u x %u\n", ++g_totalstill, info.width, info.height);
    }
    else
    {
        printf("event callback: %d\n", nEvent);
    }
}

int main(int, char**)
{
    ToupcamDeviceV2 tdev[TOUPCAM_MAX] = { 0 };
    unsigned ncam = Toupcam_EnumV2(tdev);
    if (0 == ncam)
    {
        printf("no camera found\n");
        return -1;
    }
    
    g_hcam = Toupcam_Open(tdev[0].id);
    if (NULL == g_hcam)
    {
        printf("failed to open camera\n");
        return -1;
    }
    
    HRESULT hr = Toupcam_put_Option(g_hcam, TOUPCAM_OPTION_RAW, 1); /* use raw image data */
    printf("put option raw: hr = %08x\n", hr);

    if (tdev[0].model->preview > 1)
        Toupcam_put_eSize(g_hcam, 1);

    bool bbitdepth = false;
    if (tdev[0].model->flag & (TOUPCAM_FLAG_RAW10 | TOUPCAM_FLAG_RAW12 | TOUPCAM_FLAG_RAW14 | TOUPCAM_FLAG_RAW16))  /* bitdepth supported */
    {
        Toupcam_put_Option(g_hcam, TOUPCAM_OPTION_BITDEPTH, 1); /* enable bitdepth */
        bbitdepth = true;
    }

    int nMaxWidth = 0, nMaxHeight = 0;
    hr = Toupcam_get_Resolution(g_hcam, 0, &nMaxWidth, &nMaxHeight);
    if (FAILED(hr))
        printf("failed to get size, hr = %08x\n", hr);
    else
    {
        g_pImageData = malloc(nMaxWidth * nMaxHeight * (bbitdepth ? 2 : 1));
        if (NULL == g_pImageData)
            printf("failed to malloc\n");
        else
        {
            hr = Toupcam_StartPullModeWithCallback(g_hcam, EventCallback, NULL);
            if (FAILED(hr))
                printf("failed to start camera, hr = %08x\n", hr);
            else
            {
                bool bloop = true;
                while (bloop)
                {
					if (tdev[0].model->still)
						printf("press x to exit, press other key to snap\n");
					else
						printf("press any key to exit\n");
#ifdef _WIN32
                    switch (_getch())
#else
                    switch (getchar())
#endif
                    {
                    case 'x':
                        bloop = false;
                        break;
                    default:
                        if (tdev[0].model->still)             /* snap supported */
                        {
                            hr = Toupcam_Snap(g_hcam, 0);
                            printf("snap: %08x\n", hr);
                        }
                        else
                        {
                            printf("snap not supported\n");
                        }
                        break;
                    }
                }
            }
        }
    }
    
    /* cleanup */
    Toupcam_Close(g_hcam);
    if (g_pImageData)
        free(g_pImageData);
    return 0;
}
