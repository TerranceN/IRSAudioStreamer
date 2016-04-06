#include "irs.h"

#include "stdlib.h"
#include "stdint.h"
#include "stdio.h"
#include <string.h>
#include <stdbool.h>
#include <math.h>

#pragma pack(push,1)
typedef struct {
  // Should always contain the text "iSim"
  char format[4];
  // File format version
  int32_t version;
  // Total size of the header in bytes
  int32_t headerSize;
  // The length of the scene in voxels
  int32_t sizeX;
  // The height of the scene in voxels
  int32_t sizeY;
  // The depth of the scene in voxels
  int32_t sizeZ;
  // Number of samples per second
  int32_t samplingRate;
  // Speed of sound (voxels/sample)
  float speedOfSound;
  // Voxels per metre
  float scale;
  // The number of sources in the file
  int32_t nSources;
  // The number of listeners in the file
  int32_t nListeners;
} IRSHeader;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
  int32_t id;
  int32_t xPos;
  int32_t yPos;
  int32_t zPos;
} IRSListenerDataChunk;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
  int32_t size;
  int32_t nEntries;
} IRSListenerHeaderChunk;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
  int32_t id;
  int32_t xPos;
  int32_t yPos;
  int32_t zPos;
  int32_t type;
  int32_t nSamples;
} IRSSourceDataChunk;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
  int32_t size;
  int32_t nEntries;
} IRSSourceHeaderChunk;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct {
  int32_t size;
  int32_t sourceId;
  int32_t listenerId;
} IRSDataHeaderChunk;
#pragma pack(pop)

struct IRSListener_s {
  int id;
  IRSSource* source;
  float x;
  float y;
  float z;
  double* data;
};

struct IRSSource_s {
  int id;
  float x;
  float y;
  float z;
  int nListeners;
  int listenerAxisSize;
  int dataLen;
  IRSListener** listeners;
  IRSFile* file;
};

struct IRSFile_s {
  IRSHeader header;
  int nSources;
  int nListeners;
  IRSSource* sources;
  IRSListener* listeners;
};

// Resamples a 22050 Hz input to 44100 Hz using linear interpolation
float* resample_22050_to_44100(float* input, int n) {
  float* resampled = (float*)malloc(sizeof(float) * n * 2);
  for (int i = 0; i < n-1; i++) {
    resampled[2 * i] = input[i];
    resampled[2 * i + 1] = (input[i] + input[i+1])/2.0f;
  }
  resampled[2*n - 1] = resampled[2*n - 2] / 2.0f;
  return resampled;
}

IRSFile* loadIRSFile(char* filename) {
  IRSFile* irsFile = malloc(sizeof(IRSFile));

  FILE* file;
  if (file = fopen(filename, "rb")) {
    IRSHeader header;
    fread(&header, sizeof(IRSHeader), 1, file);

    irsFile->header = header;
    irsFile->nSources = header.nSources;

    {
      IRSSourceHeaderChunk sourceHeaderChunk;
      fread(&sourceHeaderChunk, sizeof(IRSSourceHeaderChunk), 1, file);

      irsFile->sources = malloc(sourceHeaderChunk.nEntries*sizeof(IRSSource));
      for (int i = 0; i < sourceHeaderChunk.nEntries; i++) {
        IRSSourceDataChunk sourceDataChunk;
        fread(&sourceDataChunk, sizeof(IRSSourceDataChunk), 1, file);
        irsFile->sources[i].id = sourceDataChunk.id;
        irsFile->sources[i].x = (float)sourceDataChunk.xPos/header.scale;
        irsFile->sources[i].y = (float)sourceDataChunk.yPos/header.scale;
        irsFile->sources[i].z = (float)sourceDataChunk.zPos/header.scale;
        irsFile->sources[i].dataLen = 22050;
        irsFile->sources[i].nListeners = header.nListeners;
        irsFile->sources[i].listenerAxisSize = sqrt(header.nListeners);
        irsFile->sources[i].file = irsFile;
      }
    }
    {
      IRSListenerHeaderChunk listenerHeaderChunk;
      fread(&listenerHeaderChunk, sizeof(IRSListenerHeaderChunk), 1, file);

      irsFile->listeners = malloc(listenerHeaderChunk.nEntries*sizeof(IRSListener));
      irsFile->nListeners = listenerHeaderChunk.nEntries;
      for (int i = 0; i < listenerHeaderChunk.nEntries; i++) {
        IRSListenerDataChunk listenerDataChunk;
        int r = fread(&listenerDataChunk, sizeof(IRSListenerDataChunk), 1, file);
        irsFile->listeners[i].id = listenerDataChunk.id;
        irsFile->listeners[i].x = (float)listenerDataChunk.xPos/header.scale;
        irsFile->listeners[i].y = (float)listenerDataChunk.yPos/header.scale;
        irsFile->listeners[i].z = (float)listenerDataChunk.zPos/header.scale;
      }
    }
    {
      for (int i =0; i < header.nSources; i++) {
        float* buffer = malloc(irsFile->sources[i].dataLen*sizeof(float));
        irsFile->sources[i].listeners = malloc(irsFile->sources[i].nListeners*sizeof(IRSListener*));

        for (int j = 0; j < irsFile->sources[i].nListeners; j++) {
          IRSListener* listener = NULL;

          IRSDataHeaderChunk dataHeaderChunk;
          fread(&dataHeaderChunk, sizeof(IRSDataHeaderChunk), 1, file);

          for (int k = 0; k < irsFile->nListeners; k++) {
            if (irsFile->listeners[k].id == dataHeaderChunk.listenerId) {
              listener = &irsFile->listeners[k];
            }
          }
          irsFile->sources[i].listeners[j] = listener;


          listener->source = &irsFile->sources[i];
          listener->data = malloc(irsFile->sources[i].dataLen*2*sizeof(double));
          fread(buffer, sizeof(float), irsFile->sources[i].dataLen, file);
          float* resampled = resample_22050_to_44100(buffer, irsFile->sources[i].dataLen);
          for (int k = 0; k < irsFile->sources[i].dataLen*2; k++) {
            listener->data[k] = resampled[k];
          }
          free(resampled);
        }

        irsFile->sources[i].dataLen *= 2;
        free(buffer);
      }
    }
  } else {
    free(irsFile);
    return NULL;
  }fclose(file);

  return irsFile;
}

IRSSource* getClosestSource(IRSFile* irsFile, float x, float y, float z) {
  return &irsFile->sources[0];
}

IRSListener* getClosestListener(IRSSource* source, float x, float y, float z) {
  return source->listeners[267];
}

void getInterpolatedData(IRSSource* source, float x, float y, float z, double** dst, int* dstLen) {
  int numSignals = 1;

  IRSListener** listeners = malloc(4*sizeof(IRSListener*));

  bool offXNeg = x < -0.5*source->file->header.sizeX/source->file->header.scale;
  bool offXPos = x > 0.5*source->file->header.sizeX/source->file->header.scale;
  bool offYNeg = y < -0.5*source->file->header.sizeY/source->file->header.scale;
  bool offYPos = y > 0.5*source->file->header.sizeY/source->file->header.scale;

  if (offXNeg || offXPos) {
    int xIndex = 0;
    if (offXPos) {
      xIndex = source->listenerAxisSize-1;
    }
    if (offYNeg || offYPos) {
      // only grab appropriate corner
      int yIndex = 0;
      if (offYPos) {
        yIndex = source->listenerAxisSize-1;
      }
      numSignals = 1;
      listeners[0] = source->listeners[xIndex*source->listenerAxisSize+yIndex];
    } else {
      // grab two closest searching along y
      int yUpper;
      for (yUpper = 0; yUpper < source->listenerAxisSize; yUpper++) {
        if (source->listeners[yUpper]->y > y) {
          break;
        }
      }
      numSignals = 2;
      listeners[0] = source->listeners[xIndex*source->listenerAxisSize+yUpper-1];
      listeners[1] = source->listeners[xIndex*source->listenerAxisSize+yUpper];
    }
  } else if (offYNeg || offYPos) {
    int yIndex = 0;
    if (offYPos) {
      yIndex = source->listenerAxisSize-1;
    }
    // grab two closest searching along x
    int xUpper;
    for (xUpper = 0; xUpper < source->listenerAxisSize; xUpper++) {
      if (source->listeners[xUpper*source->listenerAxisSize]->x > x) {
        break;
      }
    }
    numSignals = 2;
    listeners[0] = source->listeners[(xUpper-1)*source->listenerAxisSize+yIndex];
    listeners[1] = source->listeners[xUpper*source->listenerAxisSize+yIndex];
  } else {
    // grab four closest searching both x and y
    numSignals = 4;
    int xUpper;
    for (xUpper = 0; xUpper < source->listenerAxisSize; xUpper++) {
      if (source->listeners[xUpper*source->listenerAxisSize]->x > x) {
        break;
      }
    }
    int yUpper;
    for (yUpper = 0; yUpper < source->listenerAxisSize; yUpper++) {
      if (source->listeners[yUpper]->y > y) {
        break;
      }
    }
    listeners[0] = source->listeners[(xUpper-1)*source->listenerAxisSize+yUpper-1];
    listeners[1] = source->listeners[xUpper*source->listenerAxisSize+yUpper-1];
    listeners[2] = source->listeners[(xUpper-1)*source->listenerAxisSize+yUpper];
    listeners[3] = source->listeners[xUpper*source->listenerAxisSize+yUpper];
  }

  double** signals = malloc(numSignals*sizeof(double*));
  int* lengths = malloc(numSignals*sizeof(int*));
  float* weights = malloc(numSignals*sizeof(float*));

  for (int i = 0; i < numSignals; i++) {
    getListenerData(listeners[i], &signals[i], &lengths[i]);
  }

  weights[0] = 1.0;
  if (numSignals == 2) {
    float interp;
    if (offXNeg || offYNeg) {
      interp = (y - listeners[0]->y) / (listeners[1]->y - listeners[0]->y);
    } else {
      interp = (x - listeners[0]->x) / (listeners[1]->x - listeners[0]->x);
    }
    weights[0] = 1.0 - interp;
    weights[1] = interp;
  } else if (numSignals == 4) {
    float xInterp = (x - listeners[0]->x) / (listeners[1]->x - listeners[0]->x);
    float yInterp = (y - listeners[0]->y) / (listeners[2]->y - listeners[0]->y);
    weights[0] = (1-xInterp)*(1-yInterp);
    weights[1] = xInterp*(1-yInterp);
    weights[2] = (1-xInterp)*yInterp;
    weights[3] = xInterp * yInterp;
  }

  free(listeners);

  {
    int maxLen = 0;
    for (int i = 0; i < numSignals; i++) {
      if (lengths[i] > maxLen) {
        maxLen = lengths[i];
      }
    }

    *dstLen = maxLen;
    double* newBuf = malloc((*dstLen)*sizeof(double));
    memset(newBuf, 0, (*dstLen)*sizeof(double));
    for (int i = 0; i < numSignals; i++) {
      for (int j = 0; j < lengths[i]; j++) {
        newBuf[j] += weights[i]*signals[i][j];
      }
    }
    *dst = newBuf;
  }

  free(signals);
  free(lengths);
  free(weights);
}

void getListenerData(IRSListener* listener, double** data, int* dataLen) {
  *dataLen = listener->source->dataLen;
  *data = listener->data;
}
