#include "convolve.h"
#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "stdbool.h"
#include "string.h"
#include <fftw3.h>

short CONVOLVE(
    // Source
    complexNum *srcSignal,
    int srcLen,
    // Impulse Response
    complexNum *irSignal,
    int irLen,
    // Destination
    complexNum **dstSignal,
    int* dstLen) {
  int maxLen = srcLen+irLen-1;

  //printf("(%d, %d) -> %d\n", srcLen, irLen, maxLen);

  complexNum* dst = malloc(maxLen*sizeof(complexNum));
  complexNum* ir = malloc(maxLen*sizeof(complexNum));

  fftw_plan dstFFT;
  dstFFT = fftw_plan_dft_1d(maxLen, dst, dst, FFTW_FORWARD, FFTW_ESTIMATE);

  fftw_plan irFFT;
  irFFT = fftw_plan_dft_1d(maxLen, ir, ir, FFTW_FORWARD, FFTW_ESTIMATE);

  fftw_plan dstIFFT;
  dstIFFT = fftw_plan_dft_1d(maxLen, dst, dst, FFTW_BACKWARD, FFTW_ESTIMATE);

  // set up the dst array as a copy of the src, 0-padded
  memcpy(dst, srcSignal, srcLen*sizeof(complexNum));
  memset(dst+srcLen, 0, (maxLen-srcLen)*sizeof(complexNum));

  // copy the ir signal
  memcpy(ir, irSignal, irLen*sizeof(complexNum));

  // zero pad the ir signal
  memset(ir+irLen, 0, (maxLen-irLen)*sizeof(complexNum));

  {
    fftw_execute(dstFFT);
    fftw_execute(irFFT);

    // pairwise multiply
    for (int i = 0; i < maxLen; i++) {
      double re = dst[i].re * ir[i].re - dst[i].im * ir[i].im;
      double im = dst[i].im * ir[i].re + dst[i].re * ir[i].im;
      dst[i].re = re;
      dst[i].im = im;
    }

    fftw_execute(dstIFFT);

    for (int i = 0; i < maxLen; i++) {
      dst[i].re = dst[i].re / maxLen;
      dst[i].im = dst[i].im / maxLen;
    }
  }

  free(ir);
  fftw_destroy_plan(dstFFT);
  fftw_destroy_plan(irFFT);
  fftw_destroy_plan(dstIFFT);

  *dstSignal = dst;
  *dstLen = srcLen + irLen - 1;
  return true;
}
