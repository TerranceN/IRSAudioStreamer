#ifndef IRS_H
#define IRS_H

typedef struct IRSFile_s IRSFile;
typedef struct IRSListener_s IRSListener;
typedef struct IRSSource_s IRSSource;

IRSFile* loadIRSFile(char* filename);
IRSSource* getClosestSource(IRSFile* irsFile, float x, float y, float z);
IRSListener* getClosestListener(IRSSource* source, float x, float y, float z);
void getInterpolatedData(IRSSource* source, float x, float y, float z, double** dst, int* dstLen);
void getListenerData(IRSListener* listener, double** data, int* dataLen);

#endif
