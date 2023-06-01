/*

xspectrum.c

nice little program for visalizing spectrum of an audio signal under X11.

(C) Philippe Strauss <philippe.strauss@urbanet.ch>, 1995, 1996.

this program is licensed under the term of the GNU General Public License 2,
which is included in the file GPL2.

*/

#include "xspectrum.h"

/*****************************************************************************/
/* functions code */

/*
QuitOnError : Do a simple exit on error, print the errno value.
*/
static void QuitOnError (const char * pchrErrorMessage)
{
    int iZero = 0;

    perror (pchrErrorMessage);

    /* toggle enable bit to zero */

    if (ioctl (sPrivate.iFdDSP, SOUND_PCM_SETTRIGGER, &iZero))
        perror("Error on ioctl SNDCTL_DSP_SETTRIGGER");

    exit (1);
}

/*
AllocCmplxRect : Allocate memory for the arrays in a CmplxRect structure
*/
static void AllocCmplxRect (tsCmplxRect *psCmplxRect, tInt iN)
{
    psCmplxRect->pfReal = (ptFloat) calloc (iN, (size_t) sizeof (tFloat));
    psCmplxRect->pfImag = (ptFloat) calloc (iN, (size_t) sizeof (tFloat));

    if ((psCmplxRect->pfReal == NULL) || (psCmplxRect->pfImag == NULL))
        QuitOnError ("Error: Unable to allocate memory for CmplxRect");
}

static void FreeCmplxRect (tsCmplxRect *psCmplxRect)
{
    free (psCmplxRect->pfReal);
    free (psCmplxRect->pfImag);
}

/*
SoundRead_S16_LE : Read from soundcard, pBuffer has been mapped as the DMA
                   buffer. Data readed from the soundcard must be in
                   signed 16 bits Little Endian format.
*/
static void SoundRead_S16_LE ()
{
    static int iBuffSize, iBeginDMA, iCurrentDMA, iIdxDest, iIdxDMA;
    static count_info sCountInfo;
    static volatile int16 *pLocalPtr;
    static int iPrevCntPtr = 0;
#   ifdef DEBUG
    static int iDenom;
    static float fFps;
#   endif

    iBuffSize = sPrivate.iBuffLen / sizeof (int16);
    /* convenient way to convert data format. pBuffer is of type caddr_t */
    /* which is a typedef of char *, data returned by most soundcards is in */
    /* 16 bits little endian, which hoppefully is the short on i386 linux */
    pLocalPtr = (volatile int16 *) sPrivate.pBuffer;
    
    iPrevCntPtr = sCountInfo.ptr;

    if (ioctl (sPrivate.iFdDSP, SNDCTL_DSP_GETIPTR, &sCountInfo) != 0)
        QuitOnError ("Error on ioctl SNDCTL_DSP_GETIPTR");

#   ifdef DEBUG
    /* calculate the number of frame per second */
    iDenom = (sCountInfo.ptr - iPrevCntPtr + sPrivate.iBuffLen)
             % sPrivate.iBuffLen;
    if (iDenom != 0)
        fFps = (sPrivate.iSpeed * sizeof (int16)) / (float) iDenom;
    else
       fFps = 0.0;
 
    printf ("\rDebug:\tDMA pointer %05d, Refresh rate %3.1f fps",
            sCountInfo.ptr, fFps);
    fflush (stdout);
#   endif /* DEBUG */

/*  iCurrentDMA = (sCountInfo.ptr - 1) / sizeof (int16);*/
    iCurrentDMA = (sCountInfo.ptr) / sizeof (int16);

    /* circular adressing of DMA buffer */
    iBeginDMA   = (iCurrentDMA + iBuffSize - sPrivate.iN) % iBuffSize;

    for (iIdxDest = 0, iIdxDMA = iBeginDMA;
         iIdxDest <= sPrivate.iN;
         ++iIdxDest, iIdxDMA = (iIdxDMA + 1) % iBuffSize)
    {
        sPrivate.sFFT.pfReal [iIdxDest] = pLocalPtr [iIdxDMA] / 32768.0;
        sPrivate.sFFT.pfImag [iIdxDest] = 0.0;
    }
}

/*
Initialize : Top-level initialization function.
*/
static void Initialize ()
{
    sPrivate.iSpeed      = 40000;
    sPrivate.iN          = NPOINTS;
    sPrivate.iOrder      = ORDER;
    sPrivate.iNChannels  = 1;
    sPrivate.iFormat     = AFMT_S16_LE;
    sPrivate.iBeta       = 5;
    sPrivate.uiWidth     = sPrivate.iN / 2; /* width of the chart window */
    sPrivate.uiHeight    = CHART_H;  /* height of the chart window */
    sPrivate.fDB2Pixel   = DB2PIXEL;
    sPrivate.fRefLevel   = REF_LEVEL;
    sPrivate.iBuffLen    = sizeof (int16) * sPrivate.iN;
    sPrivate.iRun        = 1;

#   ifdef DEBUG
    printf ("Debug:\tdB2Pixel = %f\n", sPrivate.fDB2Pixel);
    printf ("Debug:\tRefLevel = %f\n", sPrivate.fRefLevel);
#   endif

    InitMemAlloc    ();
    InitSoundDevice ();
    InitFFT         ();
}

/*
InitMemAlloc : do all  memory allocation needed.
*/
static void InitMemAlloc ()
{
    AllocCmplxRect (&(sPrivate.sFFT), sPrivate.iN);
    AllocCmplxRect (&(sPrivate.sTwidTable), sPrivate.iN);

    sPrivate.piSwapTable = (ptInt) calloc (sPrivate.iN,
                                           (size_t) sizeof (tInt));
    if (sPrivate.piSwapTable == NULL)
        QuitOnError ("Cannot allocate memory for IntArray");

    sPrivate.pfWindow = (ptFloat) calloc (sPrivate.iN,
                                          (size_t) sizeof (tFloat));
    if (sPrivate.pfWindow == NULL)
        QuitOnError ("Cannot allocate memory for RealArray");

    sPrivate.pfMag = (ptFloat) calloc (sPrivate.iN, (size_t) sizeof (tFloat));
    if (sPrivate.pfMag == NULL)
        QuitOnError ("Cannot allocate memory for RealArray");

    sPrivate.pPoints =
        (XPoint *) calloc (sPrivate.iN / 2, (size_t) sizeof (XPoint));
    if (sPrivate.pPoints == NULL)
        QuitOnError ("Cannot allocate memory for Points");
 
    sPrivate.pGrid =
        (XSegment *) calloc ((GRID_H + GRID_V + 2),
                             (size_t) sizeof (XSegment));
    if (sPrivate.pGrid == NULL)
        QuitOnError ("Cannot allocate memory for Grid");

} /* InitMemAlloc */

/*
InitSoundDevice : deal with the kernel for opening /dev/dsp
*/
static void InitSoundDevice ()
{
    audio_buf_info sBuffInfo;

    if ((sPrivate.iFdDSP = open ("/dev/dsp", O_RDONLY)) == -1)
        QuitOnError ("Cannot open /dev/dsp");

    /* set number of channels */
    if (ioctl (sPrivate.iFdDSP, SNDCTL_DSP_CHANNELS,
               &(sPrivate.iNChannels)))
        QuitOnError ("Error on ioctl SNDCTL_DSP_CHANNELS");

    /* set the format */
    if (ioctl (sPrivate.iFdDSP, SNDCTL_DSP_SETFMT, &(sPrivate.iFormat)))
        QuitOnError ("Error on ioctl SNDCTL_DSP_SETFMT");
    if (sPrivate.iFormat != AFMT_S16_LE)
        QuitOnError ("What shitty hardware are you using?");

    /* set the sampling rate */
    if (ioctl (sPrivate.iFdDSP, SNDCTL_DSP_SPEED, &(sPrivate.iSpeed)))
        QuitOnError ("Error on ioctl SNDCTL_DSP_SPEED");

#   ifdef DEBUG
    /* write on stdout how the sound driver has done things for us */
    printf ("Debug:\tNumber of channels is %d\n", sPrivate.iNChannels);
    printf ("Debug:\tSample rate is %d Hz\n", sPrivate.iSpeed);
    printf ("Debug:\tFormat is AFMT_S16_LE\n");
#   endif

    /* query capabilites */
    if (ioctl (sPrivate.iFdDSP, SNDCTL_DSP_GETCAPS,
               &(sPrivate.iCaps)) != 0)
        QuitOnError ("Error on ioctl SNDCTL_DSP_GETCAPS");

#   ifdef DEBUG
    if (sPrivate.iCaps & DSP_CAP_DUPLEX)
        printf ("Debug:\tSound device support full duplex mode\n");

    if (sPrivate.iCaps & DSP_CAP_REALTIME)
        printf ("Debug:\tSound device has realtime capabilities\n");

    if (sPrivate.iCaps & DSP_CAP_MMAP)
        printf ("Debug:\tSound device support memory mapping I/O\n");

    if (sPrivate.iCaps & DSP_CAP_TRIGGER)
        printf ("Debug:\tSound device support triggering\n");
#   endif /* DEBUG */

    /* we must obtain mmaping */
    if ((sPrivate.iCaps & DSP_CAP_MMAP) &&
        (sPrivate.iCaps & DSP_CAP_TRIGGER))
    {
        if (ioctl (sPrivate.iFdDSP, SNDCTL_DSP_GETISPACE,
                   &sBuffInfo) != 0)
            QuitOnError ("Error on ioctl SNDCTL_DSP_GETISPACE");

        sPrivate.iBuffLen = sBuffInfo.fragsize * sBuffInfo.fragstotal; 

#       ifdef DEBUG
        printf ("Debug:\tDMA Buffer size is %d bytes\n", sBuffInfo.fragsize
                                                       * sBuffInfo.fragstotal);
        printf ("Debug:\tNumber of available fragments is %d\n",
                sBuffInfo.fragments);
        printf ("Debug:\tNumber of fragments allocated is %d\n",
                sBuffInfo.fragstotal);
        printf ("Debug:\tSize of a fragment is %d bytes\n",
                sBuffInfo.fragsize);
        printf ("Debug:\tAvailable size is %d bytes\n", sBuffInfo.bytes);
        printf ("Debug:\tWe use %d bytes\n", sPrivate.iBuffLen);
#       endif /* DEBUG */

        /* map the DMA buffer in the proc vm */
        if ((sPrivate.pBuffer
            = (caddr_t) mmap (
                     NULL, /* hint adress for kernel, free if NULL */
                     sPrivate.iBuffLen,   /* length of buffer */
                     PROT_READ,           /* reading access only */
                     MAP_FILE|MAP_SHARED, /* map to a file or a char device */
                     sPrivate.iFdDSP,     /* file descriptor */
                     0                    /* offset inside of file */)
            ) == (void *) -1)
            QuitOnError ("Cannot mmap, Arrgh");

#       ifdef DEBUG
        printf ("Debug:\tDMA buffer adress is %x\n",
                (unsigned int) sPrivate.pBuffer);
#       endif /* DEBUG */

    }
    else
        QuitOnError ("Sound device does not support memory mapping I/O");
} /* InitSoundDevice */

static void CloseSoundDevice ()
{
    StopRecording ();
    munmap        (sPrivate.pBuffer, sPrivate.iBuffLen);
    close         (sPrivate.iFdDSP);
}

static void SetSF ()
{
    StopRecording ();
 
    /* set the sampling rate */
    if (ioctl (sPrivate.iFdDSP, SNDCTL_DSP_SPEED, &(sPrivate.iSpeed)))
        QuitOnError ("Error on ioctl SNDCTL_DSP_SPEED");

    StartRecording ();
}

/*
InitFFT : initalize window, twiddle table and swap table.
*/
static void InitFFT ()
{
    /* initialize pfWindow with a Kaiser window in it */
    InitWinKaiser (sPrivate.pfWindow, sPrivate.iBeta, sPrivate.iN);

    /* initialize sTwidTable with the twiddle factors */
    InitTwidTable (sPrivate.sTwidTable, DIRECT, sPrivate.iN);

    /* initialize piSwapTable with reversed digit for reordering fft coeffs */
#   ifdef RADIX4
    InitR4SwapTable (sPrivate.piSwapTable, sPrivate.iOrder, sPrivate.iN);
#   endif
#   ifdef RADIX2
    InitR2SwapTable (sPrivate.piSwapTable, sPrivate.iN);
#   endif
}

static void InitChart ()
{
    int idx, iH, iV, iX, iY;
    float fHstep, fVstep;
    Pixel pxChartBG, pxChartFG, pxGridFG;
    static XColor colRight, colApprox;
    XGCValues sGChartValues, sGClearValues, sGCgridValues;
    static Window winRoot;
    static Colormap cmapDefault;

    sPrivate.pdispChart = EZ_GetDisplay();
    winRoot = DefaultRootWindow (sPrivate.pdispChart);
    cmapDefault = DefaultColormap(sPrivate.pdispChart,
                                  DefaultScreen(sPrivate.pdispChart));

    if (XAllocNamedColor (sPrivate.pdispChart, cmapDefault, "Aquamarine",
                      &colApprox, &colRight) == 0)
        QuitOnError ("Cannot lookup color");
    pxChartFG = colApprox.pixel;

    if (XAllocNamedColor (sPrivate.pdispChart, cmapDefault, "black",
                      &colApprox, &colRight) == 0)
        QuitOnError ("Cannot lookup color");
    pxChartBG = colApprox.pixel;

    if (XAllocNamedColor (sPrivate.pdispChart, cmapDefault, "grey50",
                      &colApprox, &colRight) == 0)
        QuitOnError ("Cannot lookup color");
    pxGridFG = colApprox.pixel;

    /* GC for drawing the chart */
    sGChartValues.background = pxChartBG;
    sGChartValues.foreground = pxChartFG;

    /* create the drawing GC */
    sPrivate.gcChart
        = XCreateGC (sPrivate.pdispChart,
                     winRoot,
                     GCForeground | GCBackground,
                     &sGChartValues);

    /* GC used for erasing */
    sGClearValues.foreground = pxChartBG;

    /* create the erasing GC */
    sPrivate.gcClearChart
        = XCreateGC (sPrivate.pdispChart,
                     winRoot,
                     GCForeground,
                     &sGClearValues);

    /* GC for the grid lines */
    sGCgridValues.foreground = pxGridFG;
    sGCgridValues.line_style = LineSolid;

    /* create the grid GC */
    sPrivate.gcGrid
        = XCreateGC (sPrivate.pdispChart,
                     winRoot,
                     GCForeground | GCLineStyle,
                     &sGCgridValues);

    /* initialize the segments array for drawing the grid */
    idx = 0;
    fHstep = sPrivate.uiWidth / (float) GRID_H;
    for (iH = 0; iH <= GRID_H; ++iH)
    {
        iX = (int) ceil (iH * fHstep); 
        sPrivate.pGrid[idx].x1 = iX;
        sPrivate.pGrid[idx].y1 = 0;
        sPrivate.pGrid[idx].x2 = iX;
        sPrivate.pGrid[idx].y2 = sPrivate.uiHeight;
        ++idx;
    }

    fVstep = sPrivate.uiHeight / (float) GRID_V;
    for (iV = 0; iV <= GRID_V; ++iV)
    {
        iY = (int) ceil (iV * fVstep); 
        sPrivate.pGrid[idx].x1 = 0;
        sPrivate.pGrid[idx].y1 = iY;
        sPrivate.pGrid[idx].x2 = sPrivate.uiWidth;
        sPrivate.pGrid[idx].y2 = iY;
        ++idx;
    }

    /* we create a simple window with XLib */
    sPrivate.winChart =
        XCreateSimpleWindow (sPrivate.pdispChart,
            winRoot,
            0, 0,
            sPrivate.uiWidth+2, sPrivate.uiHeight+2,
            0,
            0,
            0x0000);

    /* we make this window appear */
    XMapWindow (sPrivate.pdispChart, sPrivate.winChart);

    /* ask for a back buffer */
    sPrivate.BackBuff = XdbeAllocateBackBufferName (sPrivate.pdispChart,
                                                    sPrivate.winChart,
                                                    XdbeUndefined);

    /* fill-in some info for swapping buffers*/
    sPrivate.SwapInfo.swap_window = sPrivate.winChart;
    /* we don't care about what hapend to the ex-front buffer */
    sPrivate.SwapInfo.swap_action = XdbeUndefined;
}

static void StartRecording ()
{
    int iEnableInput = PCM_ENABLE_INPUT;

    /* toggle bit to zero */
    StopRecording ();

    /* toggle enable bit to one, start recording */
    if (ioctl (sPrivate.iFdDSP, SOUND_PCM_SETTRIGGER, &iEnableInput))
        QuitOnError ("Error on ioctl SNDCTL_DSP_SETTRIGGER");
}

static void StopRecording ()
{
    int iZero = 0;

    /* toggle enable bit to zero */
    if (ioctl (sPrivate.iFdDSP, SOUND_PCM_SETTRIGGER, &iZero))
        QuitOnError ("Error on ioctl SNDCTL_DSP_SETTRIGGER");
}

static void DrawChart ()
{
    XdbeBeginIdiom(sPrivate.pdispChart);

    XFillRectangle (sPrivate.pdispChart,
                    sPrivate.BackBuff,
                    sPrivate.gcClearChart,
                    0, 0,
                    sPrivate.uiWidth, sPrivate.uiHeight);

    XDrawSegments (sPrivate.pdispChart,
                   sPrivate.BackBuff,
                   sPrivate.gcGrid,
                   sPrivate.pGrid,
                   (GRID_H + GRID_V + 2));

    XDrawLines (sPrivate.pdispChart, /* X display */
                sPrivate.BackBuff,   /* X drawable */
                sPrivate.gcChart,    /* GC */
                sPrivate.pPoints,    /* XPoint * */
                sPrivate.uiWidth,    /* points number */
                CoordModeOrigin      /* normal coord */);

    XdbeSwapBuffers (sPrivate.pdispChart, &(sPrivate.SwapInfo), 1);

    XSync (sPrivate.pdispChart, False);

    XdbeEndIdiom(sPrivate.pdispChart);
}

/* main processing task as a work procedure */
static Boolean DoFFT ()
{
    static int iIdx, iYPointVal;

    SoundRead_S16_LE ();
    WindowingCR (sPrivate.sFFT, sPrivate.pfWindow, sPrivate.iN);
#   ifdef RADIX4
    R4FFTdif    (sPrivate.sFFT, sPrivate.sTwidTable, sPrivate.iOrder,
                 sPrivate.iN);
#   endif
#   ifdef RADIX2
    R2FFTdif    (sPrivate.sFFT, sPrivate.sTwidTable, sPrivate.iOrder,
                 sPrivate.iN);
#   endif
    SwapFFTC    (sPrivate.sFFT, sPrivate.piSwapTable, sPrivate.iN);

    Magnitude2  (sPrivate.sFFT, sPrivate.pfMag, sPrivate.iN);

    for (iIdx = 0; iIdx < (int) sPrivate.uiWidth; ++ iIdx)
    {
        sPrivate.pPoints[iIdx].x = iIdx;
        /* must be non-zero otherwise log10 go -infinity ! */
        if (sPrivate.pfMag [iIdx] > 0.0)
        {
            iYPointVal = -sPrivate.fDB2Pixel
                          * 10 * (log10 (sPrivate.pfMag [iIdx])
                                         - sPrivate.fRefLevel);
            /* not under the bottom of the chart */
            if (iYPointVal >= (int) sPrivate.uiHeight)
                sPrivate.pPoints[iIdx].y = sPrivate.uiHeight - 1;
            /* not over the top of the chart */
            else if (iYPointVal < 0)
                sPrivate.pPoints[iIdx].y = 0;
            else
                /* normal case */
                sPrivate.pPoints[iIdx].y = iYPointVal;
        }
        else
            /* rather than -infinity */  
            sPrivate.pPoints[iIdx].y = sPrivate.uiHeight - 1;
    }
    /* redraw */
    DrawChart ();

    return (False); /* so we'll be soon back here */
} /* proc */

/*****************************************************************************/
/* callback procedures */

static void ApplySF (EZ_Widget *w)
{
    static char strSpeed[12];

    sPrivate.iSpeed = strtol (EZ_GetEntryString (w), NULL, 10);
    SetSF ();
    sprintf (strSpeed, "%d", sPrivate.iSpeed);
    EZ_SetEntryString (w, strSpeed);
}

static void Freeze (EZ_Widget *w)
{
    int iOnOff, iValue;

    iOnOff = EZ_GetCheckButtonState (w, &iValue);
    if (iOnOff == 1)
    {
        StopRecording();
        sPrivate.iRun = 0;
    }
    else
    {
        StartRecording();
        sPrivate.iRun = 1;
    }
}

static void Quit (EZ_Widget *w)
{
    CloseSoundDevice ();
    FreeCmplxRect    (&(sPrivate.sFFT));
    FreeCmplxRect    (&(sPrivate.sTwidTable));
    free             (sPrivate.piSwapTable);
    free             (sPrivate.pfWindow);
    free             (sPrivate.pfMag);
    /*XdbeDeallocateBackBufferName (sPrivate.pdispChart, sPrivate.BackBuff);*/
    /*XFreeGC*/
#   ifdef DEBUG
    /* if we outputed some debug info in SoundRead_S16_LE, we need this */
    fflush           (stdout);
    printf           ("\n");
#   endif
    EZ_Shutdown();
    exit (0);
}

/*****************************************************************************/

int main (int argc, char *argv[])
{
    Initialize ();

    EZ_Initialize (argc, argv, 0);

    InitChart ();

    sPrivate.wDialog = EZ_CreateFrame (NULL, "XSpectrum Control");
    EZ_ConfigureWidget (sPrivate.wDialog,
                        EZ_TRANSIENT, True,
                        EZ_STACKING, EZ_VERTICAL,
                        EZ_BACKGROUND, "grey55", 0);

    sPrivate.wFrameSF = EZ_CreateFrame (sPrivate.wDialog, "Sampling Frequency [Hz]");
    sPrivate.wEntrySF = EZ_CreateEntry (sPrivate.wFrameSF, "40000");
    EZ_ConfigureWidget (sPrivate.wEntrySF,
                       EZ_CALL_BACK, ApplySF, 0);

    sPrivate.wFreeze = EZ_CreateCheckButton (sPrivate.wDialog, "Freeze", 0, 1, 0, 0);
    EZ_ConfigureWidget (sPrivate.wFreeze,
                        EZ_CALLBACK, Freeze, 0);

    sPrivate.wQuit = EZ_CreateButton (sPrivate.wDialog, "Quit", 0);
    EZ_ConfigureWidget (sPrivate.wQuit,
                        EZ_CALL_BACK, Quit, 0);

    EZ_DisplayWidget (sPrivate.wDialog);

    StartRecording ();

    for (;;)
    {
        if (sPrivate.iRun)
            DoFFT();
        EZ_ServiceEvents();
    }

    return 0;
}










