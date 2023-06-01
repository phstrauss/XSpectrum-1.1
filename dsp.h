/*

dsp.h

typesdef and constant for libdsp.c

(C) Philippe Strauss <philippe.strauss@urbanet.ch>, 1993, 1995, 1996.

this program is licensed under the term of the GNU Library General Public
License 2, which is included in the file LGPL2.

*/

#ifndef _dsp_h
#define _dsp_h

#include <math.h>

#define DIRECT   (int) 1
#define REVERSE  (int) -1 

#define OK         (int) 0
#define ARRAY_SIZE (int) -1
#define MEM_ALLOC  (int) -2

typedef int tInt;
typedef tInt *ptInt;

typedef float tFloat;
typedef tFloat *ptFloat;

typedef int tError;

typedef struct
{
    ptFloat pfReal, pfImag;
} tsCmplxRect;

typedef struct
{
    ptFloat pfMagn, pfPhase;
} tsCmplxPol;

/*
Exported funcs
*/

void   InitR2SwapTable (ptInt piSwapTable, tInt iN);
void   InitR4SwapTable (ptInt piSwapTable, tInt iLog4N, tInt iN);
void   InitTwidTable   (tsCmplxRect sTwidTable, int iDirectOrReverse, tInt iN);
void   InitWinBlackman (ptFloat pfWindow, tInt iN);
void   InitWinKaiser   (ptFloat pfWindow, tInt iBeta, tInt iN);
tFloat Bessel_Io       (tFloat fX, tInt iAccuracy);
tFloat Fact            (tInt iX);
void   WindowingCR     (tsCmplxRect sFFT, ptFloat pfWindow, tInt iN);
void   WindowingRR     (ptFloat pfData, ptFloat pfWindow, tInt iN);
void   SwapFFTC        (tsCmplxRect sFFT, ptInt piSwapTable, tInt iN);
void   SwapFFTR        (ptFloat pfData, ptInt piSwapTable, tInt iN);
void   DFT             (tsCmplxRect sIn, tsCmplxRect sOut,
                        tsCmplxRect sTwidTable, tInt iN);
void   R2FFTdif        (tsCmplxRect sFFT, tsCmplxRect sTwidTable,
                        tInt iLog2N, tInt iN);
void   R2FFTdit        (tsCmplxRect sFFT, tsCmplxRect sTwidTable,
                        tInt iLog2N, tInt iN);
void   R4FFTdif        (tsCmplxRect sFFT, tsCmplxRect sTwidTable,
                        tInt iLog4N, tInt iN);
void   R4FFTdit        (tsCmplxRect sFFT, tsCmplxRect sTwidTable,
                        tInt iLog4N, tInt iN);
void   Magnitude       (tsCmplxRect sIn, ptFloat pfOut, tInt iN);
void   Magnitude2      (tsCmplxRect sIn, ptFloat pfOut, tInt iN);
void   RectToPol       (tsCmplxRect sIn, tsCmplxPol sOut, tInt iN);
tInt   Sign            (tFloat fIn);
void   Scaling         (tsCmplxRect sFFT, tFloat fCste, tInt iN);
void   AddCmplxRect    (tsCmplxRect sA, tsCmplxRect sB, tInt iBOffset,
                        tInt iNA, tInt iNB);
void   MultCmplxRect   (tsCmplxRect sA, tsCmplxRect sB, tInt iBOffset,
                        tInt iNA, tInt iNB);
tError BlockConvolve   (ptFloat pfX, ptFloat pfH, ptFloat pfY, tInt iNX,
                        tInt iNH, tInt iNY);

#endif /* _dsp_h */
