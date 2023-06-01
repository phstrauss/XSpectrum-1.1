/*

xspectrum.h

Philippe strauss, 8.1996

*/

/*****************************************************************************/
/* cpp */

#include "dsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <sys/stat.h>  /* needed by open() */
#include <fcntl.h>     /* needed by open() */
#include <unistd.h>    /* needed by open(), read() */
#include <sys/types.h> /* needed by open(), read(), mmap() */

#include <sys/mman.h>  /* mmap() */

#include <sys/soundcard.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/extensions/Xdbe.h>

#include <EZ.h>

#define RADIX4	1
#define NPOINTS	1024 /* number of points of the fft */
#define ORDER	5    /* order of the fft */ 
#define TRUE	1
#define FALSE	0
#define GRID_H	10
#define GRID_V	12
#define DB_DIV	10
#define CHART_H	360
#define DB2PIXEL	(((float) CHART_H) / (GRID_V * DB_DIV))
#define REF_LEVEL	(2.0 * log10 (NPOINTS))


/*****************************************************************************/
/* typedef */

typedef char		int8;
typedef unsigned char	uint8;
typedef short		int16;
typedef unsigned short	uint16;
typedef int		int32;
typedef unsigned int	uint32;

typedef float		float32;
typedef double		float64;

typedef struct
{
    tInt           iN;          /* number of points of the fft */
    tInt           iOrder;      /* order of the radix fft */
    tsCmplxRect    sFFT;        /* structure of arrays for fft */
    tsCmplxRect    sTwidTable;  /* structure of arrays for the twiddles */
    ptInt          piSwapTable; /* swapping address for "undecimation" */
    ptFloat        pfWindow;    /* window */
    tInt           iBeta;       /* window shape param for kaiser windows */
    ptFloat        pfMag;       /* magnitude in the freq. domain */
    int            iFdDSP;      /* file descriptor for /dev/dsp */
    int            iNChannels;  /* number of channels */
    int            iSpeed;      /* sampling rate in Hz */
    int            iFormat;     /* format (little endian 16 bits)*/
    int            iCaps;       /* Bits field containing sound capabilities */
    int            iRun;        /* Are we running ? */
    caddr_t        pBuffer;     /* DMA buffer. note: caddr_t = char * */
    int            iBuffLen;    /* length of the input buffer in bytes */
    EZ_Widget     *wTopLevel, *wDialog, *wFrameSF, *wEntrySF, *wApplySF, *wFreeze, *wQuit;
    GC             gcChart, gcClearChart, gcGrid; /* chart's graphic context */
    XPoint        *pPoints;     /* for XDrawLines */
    XSegment      *pGrid;       /* contain the grid segments */
    unsigned int   uiWidth;
    unsigned int   uiHeight;
    Window         winChart;
    Display       *pdispChart;
    XdbeBackBuffer BackBuff;
    XdbeSwapInfo   SwapInfo;
    float          fDB2Pixel, fRefLevel;
} tsPrivateAppData;

/*****************************************************************************/
/* global data */

tsPrivateAppData sPrivate;

/*****************************************************************************/
/* functions prototyping */

static void    QuitOnError      (const char * pchrErrorMessage);
static void    AllocCmplxRect   (tsCmplxRect *psCmplxRect, tInt iN);
static void    FreeCmplxRect    (tsCmplxRect *psCmplxRect);
static void    SoundRead_S16_LE ();
static void    Initialize       ();
static void    InitMemAlloc     ();
static void    InitSoundDevice  ();
static void    CloseSoundDevice ();
static void    SetSF            ();
static void    InitFFT          ();
static void    InitChart        ();

static void    StartRecording   ();
static void    StopRecording    ();

static void    DrawChart        ();
/* work proc */
static Boolean DoFFT            ();
/* callback procedures */
static void    ApplySF          (EZ_Widget *w);
static void    Freeze           (EZ_Widget *w);
static void    Quit             (EZ_Widget *w);








