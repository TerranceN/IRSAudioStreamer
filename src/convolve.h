#ifndef CONVOLVE_H
#define CONVOLVE_H

typedef struct {
  double re;
  double im;
} ComplexNum;
//#include "fft.h"

/* Convolves a signal in-place with a given impulse response
 * If there's not enough room, allocates a new float array. It's the caller's responsibility to free this array
 * If there was enough space, srcSignalL == dstSignalL and srcSignalR == dstSignalR
 */
short CONVOLVE(
    // Source
    ComplexNum *srcSignal,
    int srcLen,
    // Impulse Response
    ComplexNum *irSignal,
    int irLen,
    // Destination
    ComplexNum **dstSignal,
    int* dstLen);

#endif
