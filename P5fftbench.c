/*

P5fftbench.c

run some FFT to measure compiler performance with pentium.

(C) Philippe Strauss <philippe.strauss@urbanet.ch>, 1993, 1995, 1996.

this program is licensed under the term of the GNU General Public License 2,
which is included in the file GPL2.

*/

#include "dsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h> /* struct timeval */

#define N        1024
#define ORDER_R2 10
#define ORDER_R4 5

typedef struct
{
  unsigned int uiLow, uiHigh;
} tsCycles;

/*
InitSignal : Initialize an array with a complex exponetial
*/

void InitSignal (tsCmplxRect sFFT, tInt iN)
{
    tInt iCnt;
    tFloat fF = 50.0;
    tFloat fTheta;

    fTheta = (2 * M_PI) / iN;
    for (iCnt = 0; iCnt < iN; ++iCnt)
    {
        sFFT.pfReal [iCnt] = cos (fTheta * iCnt * fF);
        sFFT.pfImag [iCnt] = sin (fTheta * iCnt * fF);
    }
}

/*
AllocCmplxRect : Allocate memory
*/

tError AllocCmplxRect (tsCmplxRect *psCmplxRect, tInt iN)
{
    psCmplxRect->pfReal = (ptFloat) calloc (iN, (size_t) sizeof (tFloat));
    psCmplxRect->pfImag = (ptFloat) calloc (iN, (size_t) sizeof (tFloat));
    if ((psCmplxRect->pfReal == NULL) || (psCmplxRect->pfImag == NULL))
        return (ARRAY_SIZE);
    else
        return (OK);
}

void FreeCmplxRect (tsCmplxRect sCmplxRect)
{
    free (sCmplxRect.pfReal);
    free (sCmplxRect.pfImag);
}

/* from Larry McVoy lmbench */

void tvsub (struct timeval *tdiff, struct timeval *t1, struct timeval *t0)
{
        tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
        tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
        if (tdiff->tv_usec < 0)
                tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

#define COUNT_CYCLES(_S) asm("rdtsc":"=a"(_S.uiLow),"=d"(_S.uiHigh))

inline void Timing (tsCycles *sInit, tsCycles *sNow, char *pcWhat, int iNOps)
{
    static float fTime_us;

    fTime_us = (sNow->uiLow  - sInit->uiLow) / (float) MHZ;
    printf (pcWhat);
    printf (": Time spent %f us", fTime_us);
    printf ("\tAprox. %f MFLOPS\n", iNOps / fTime_us);
}

int main ()
{
    tInt iN = N, iLog2N = ORDER_R2, iLog4N = ORDER_R4;
    ptInt piSwapTable;
    ptFloat pfWindow, Mag;
    tsCmplxRect sFFT, sTwidTable, OutDFT;

    tsCycles sInit, sNow;

    if ((AllocCmplxRect (&sFFT, iN)       != OK) ||
        (AllocCmplxRect (&sTwidTable, iN) != OK) ||
        (AllocCmplxRect (&OutDFT, iN)     != OK))
    {
        FreeCmplxRect (sFFT);
        FreeCmplxRect (sTwidTable);
        FreeCmplxRect (OutDFT);
        perror ("FFTbench: malloc error on AllocCmplxRect");
        exit (errno);
     }

    piSwapTable = (ptInt) calloc (iN, (size_t) sizeof (tInt));
    if (piSwapTable== NULL)
    {
        perror ("FFTbench: malloc error on SwaptTable");
        exit (errno);
    }

    pfWindow = (ptFloat) calloc (iN, (size_t) sizeof (tFloat));
    if (pfWindow == NULL)
    {
        perror ("FFTbench: malloc error on Window");
        exit (errno);
    }

    Mag = (ptFloat) calloc (iN, (size_t) sizeof (tFloat));
    if (Mag  == NULL)
    {
        perror ("FFTbench: malloc error on Mag");
        exit (errno);
    }

    printf ("We compute %i pts FFT complex in and output\n", iN);
    printf ("CPU clock compiled in as %f\n", (float) MHZ);


    InitWinBlackman (pfWindow, iN);
    InitTwidTable   (sTwidTable, DIRECT, iN);

    InitR2SwapTable (piSwapTable, iN);

    InitSignal      (sFFT, iN);
    WindowingCR     (sFFT, pfWindow, iN);

    COUNT_CYCLES    (sInit);
    COUNT_CYCLES    (sNow);

    Timing          (&sInit, &sNow, "Measurement overhead\t\t\t", 0);

    COUNT_CYCLES    (sInit);
    R2FFTdif        (sFFT, sTwidTable, iLog2N, iN);
    COUNT_CYCLES    (sNow);
    Timing          (&sInit, &sNow, "Radix 2 FFT decimated in frequency\t", (iN * iLog2N * 10) / 2);

    SwapFFTC        (sFFT, piSwapTable, iN);

    InitSignal      (sFFT, iN);
    WindowingCR     (sFFT, pfWindow, iN);
    SwapFFTC        (sFFT, piSwapTable, iN);

    COUNT_CYCLES    (sInit);
    R2FFTdit        (sFFT, sTwidTable, iLog2N, iN);
    COUNT_CYCLES    (sNow);
    Timing          (&sInit, &sNow, "Radix 2 FFT decimated in time\t\t", (iN * iLog2N * 10) / 2);

    InitR4SwapTable (piSwapTable, iLog4N, iN);

    InitSignal      (sFFT, iN);
    WindowingCR     (sFFT, pfWindow, iN);

    COUNT_CYCLES    (sInit);
    R4FFTdif        (sFFT, sTwidTable, iLog4N, iN);
    COUNT_CYCLES    (sNow);
    Timing          (&sInit, &sNow, "Radix 4 FFT decimated in frequency\t", (iN * iLog4N * 42) / 4);

    SwapFFTC        (sFFT, piSwapTable, iN);

    InitSignal      (sFFT, iN);
    WindowingCR     (sFFT, pfWindow, iN);
    SwapFFTC        (sFFT, piSwapTable, iN);

    COUNT_CYCLES    (sInit);
    R4FFTdit        (sFFT, sTwidTable, iLog4N, iN);
    COUNT_CYCLES    (sNow);
    Timing          (&sInit, &sNow, "Radix 4 FFT decimated in time\t\t", (iN * iLog4N * 42) / 4);

    FreeCmplxRect   (sFFT);
    FreeCmplxRect   (sTwidTable);
    free            (piSwapTable);
    free            (pfWindow);
    free            (Mag);

    exit(0);
}



