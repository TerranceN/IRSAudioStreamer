#include <stdlib.h>
#include <stdio.h>
#include <sndfile.h>
#include <string.h>
#include "convolve.h"
#include "AudioSim.h"

typedef struct {
  SNDFILE* sf;
  SF_INFO info;
  ComplexNum* data;
} SoundFile;

SoundFile* myLoadSound(char* fileName) {
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

  printf("Loaded %s: channels: %d, sRate: %d, frames: %d\n", fileName, file->info.channels, file->info.samplerate, file->info.frames);

  free(buf);

  sf_close(file->sf);

  return file;
}

void myFreeSound(SoundFile* s) {
  if (s->data != NULL) {
    free(s->data);
  }
  free(s);
}

void writeWav(char* wavName, SF_INFO info, double* left) {
  int doubles = info.frames*info.channels;
  double* buf = left;
  SNDFILE* file = sf_open(wavName,SFM_WRITE,&(info));
  printf("writing %p %d\n", file, doubles);
  sf_writef_double(file, buf, doubles);
  sf_close(file);
}

int main(int argc, char** argv) {
  SNDFILE *sf;
  SF_INFO info;
  int num_channels;
  int num, num_items;
  int *buf;
  int f,sr,c;
  int i,j;
  FILE *out;

  int argCount = 1;

  AudioSim* sim = audioSim_init(argv[argCount]);
  argCount += 1;

  AudioStream* stream = audioSim_initStream(sim, 0, 0, 0);

  SoundFile* speechFile = myLoadSound(argv[argCount]);
  argCount += 1;

  {
    const int BUFSIZE = 44100;
    // I'm setting this as 2x the speech file so that you can hear the echo
    // In a real-time scenario, you would just feed 0s to the modify function
    const int amountToModify = 2*speechFile->info.frames;
    double* buf = malloc(amountToModify*sizeof(double));
    int count = 0;
    double* dstTmp = malloc(BUFSIZE*sizeof(double));
    double* srcTmp = malloc(BUFSIZE*sizeof(double));

    while (count < amountToModify) {
      int samples = BUFSIZE;
      if (count > amountToModify - BUFSIZE) {
        samples = amountToModify - count;
      }
      for (int i = 0; i < BUFSIZE; i++) {
        if (count + i < speechFile->info.frames) {
          srcTmp[i] = speechFile->data[count+i].re;
        } else {
          srcTmp[i] = 0;
        }
      }
      audioSim_modifyStream(stream, atof(argv[argCount]), atof(argv[argCount+1]), atof(argv[argCount+2]), buf+count, srcTmp, samples);

      count += BUFSIZE;
    }

    free(srcTmp);
    free(dstTmp);

    SF_INFO info = speechFile->info;
    info.frames = amountToModify;
    writeWav("output.wav", info, buf);
    free(buf);
  }

  audioSim_destroyStream(stream);
  audioSim_destroy(sim);

  myFreeSound(speechFile);

  return 0;
}
