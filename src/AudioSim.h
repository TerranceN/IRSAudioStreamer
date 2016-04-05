#ifndef AUDIOSIM_H
#define AUDIOSIM_H

typedef struct AudioSim_s AudioSim;

AudioSim* audioSim_init(char* irsFile);
void audioSim_destroy(AudioSim* a);


/* Defines a single stream of audio in the simulation
 */
typedef struct AudioStream_s AudioStream;

AudioStream* audioSim_initStream(AudioSim* a, float x, float y, float z);
void audioSim_destroyStream(AudioStream* a);

/* Resets the playback on a stream
 */
void audioSim_resetStream(AudioStream* a);

/* Interpolates the stored data and convoludes it with the given audio samples
 * len must be a power of 2 (needed for the fft)
 */
void audioSim_modifyStream(AudioStream* a, float x, float y, float z, double* dst, double* src, int len);

#endif
