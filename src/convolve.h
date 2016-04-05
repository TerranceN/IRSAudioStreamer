#ifndef CONVOLVE_H
#define CONVOLVE_H

typedef struct {
  double re;
  double im;
} complexNum;
//#include "fft.h"

/* Convolves a signal in-place with a given impulse response
 * If there's not enough room, allocates a new float array. It's the caller's responsibility to free this array
 * If there was enough space, srcSignalL == dstSignalL and srcSignalR == dstSignalR
 */
short CONVOLVE(
    // Source
    complexNum *srcSignal,
    int srcLen,
    // Impulse Response
    complexNum *irSignal,
    int irLen,
    // Destination
    complexNum **dstSignal,
    int* dstLen);

#endif
