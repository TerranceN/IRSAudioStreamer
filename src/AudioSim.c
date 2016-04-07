#include "AudioSim.h"
#include <stdlib.h>
#include <sndfile.h>
#include <string.h>
#include <math.h>
#include "convolve.h"
#include "irs.h"

typedef struct {
  SNDFILE* sf;
  SF_INFO info;
  ComplexNum* data;
} SoundFile;

struct AudioSim_s {
  int sampleRate;
  IRSFile* irsFile;
};

struct AudioStream_s {
  AudioSim* audioSim;
  IRSSource* source;
  double* leftovers;
  double max;
};

SoundFile* loadSound(char* fileName) {
  SoundFile* file = malloc(sizeof(SoundFile));
  file->info.format = 0;
  file->sf = sf_open(fileName,SFM_READ,&(file->info));
  if (file->sf == NULL) {
    printf("Failed to open the file.\n");
    exit(-1);
  }

  file->data = malloc(file->info.frames*sizeof(ComplexNum));

  int totalFrames = file->info.frames*file->info.channels;
  double* buf = malloc(totalFrames*sizeof(double));
  sf_readf_double(file->sf, buf, totalFrames);

  for (int i = 0; i < file->info.frames; i++) {
    if (file->info.channels > 0) {
      file->data[i].re = buf[i*file->info.channels];
    }
    file->data[i].im = 0;
  }

  free(buf);

  sf_close(file->sf);

  return file;
}

void freeSound(SoundFile* s) {
  if (s->data != NULL) {
    free(s->data);
  }
  free(s);
}

AudioSim* audioSim_init(char* irsFile) {
  // disregard irsFile and load our own echo file
  AudioSim* sim = malloc(sizeof(AudioSim));
  sim->irsFile = loadIRSFile(irsFile);
  if (!sim->irsFile) {
    printf("Failed to load IRS file!\n");
  }
  return sim;
}

void audioSim_destroy(AudioSim* a) {
  free(a->irsFile);
  free(a);
}

AudioStream* audioSim_initStream(AudioSim* a, float x, float y, float z) {
  AudioStream* stream = malloc(sizeof(AudioStream));
  stream->audioSim = a;
  stream->source = getClosestSource(a->irsFile, x, y, z);
  stream->leftovers = NULL;
  stream->max = 1.0;
  return stream;
}

void audioSim_destroyStream(AudioStream* s) {
  if (s->leftovers != NULL) {
    free(s->leftovers);
  }

  free(s);
}

void audioSim_resetStream(AudioStream* s) {
  if (s->leftovers != NULL) {
    free(s->leftovers);
  }
}

void audioSim_modifyStream(AudioStream* s, float x, float y, float z, double* dst, double* src, int len) {
  ComplexNum* srcComplex = malloc(len*sizeof(ComplexNum));

  for (int i = 0; i < len; i++) {
    srcComplex[i].re = src[i];
    srcComplex[i].im = 0;
  }

  ComplexNum* dstComplex;
  int dstLen;

  int irLen;
  double* irDataDoubles;
  getInterpolatedData(s->source, x, y, z, &irDataDoubles, &irLen);
  ComplexNum* irData = malloc(irLen*sizeof(ComplexNum));

  for (int i = 0; i < irLen; i++) {
    irData[i].re = irDataDoubles[i];
    irData[i].im = 0;
  }

  CONVOLVE(
    srcComplex,
    len,

    irData,
    irLen,

    &dstComplex,
    &dstLen
  );

  free(irData);
  free(irDataDoubles);

  int numLeftoversUsed = len;
  if (len > dstLen-len) {
    numLeftoversUsed = dstLen-len;
  }

  // record result
  for (int i = 0; i < len; i++) {
    dst[i] = dstComplex[i].re;
    if (s->leftovers != NULL && i < numLeftoversUsed) {
      dst[i] += s->leftovers[i];
      s->leftovers[i] = 0;
    }
    if (fabs(dst[i]) > s->max) {
      s->max = fabs(dst[i]);
    }
  }

  for (int i = 0; i < len; i++) {
    dst[i] = 0.99 * (dst[i] / s->max);
  }

  // record leftovers
  double* newLeftovers = malloc((dstLen - len)*sizeof(double));
  memset(newLeftovers, 0, (dstLen - len)*sizeof(double));
  if (s->leftovers != NULL && numLeftoversUsed < dstLen-len) {
    memcpy(newLeftovers, s->leftovers+len, (dstLen-len-numLeftoversUsed)*sizeof(double));
  }
  for (int i = len; i < dstLen; i++) {
    newLeftovers[i-len] += dstComplex[i].re;
  }
  if (s->leftovers != NULL) {
    free(s->leftovers);
  }
  s->leftovers = newLeftovers;
  free(dstComplex);
  free(srcComplex);
}
