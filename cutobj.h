#ifndef CUTOBJ
#define CUTOBJ

#include "comman.h"
#include "type_que.h"

void writeGCMask(const Mat &GCMask, const char *fileName, const int writeFlag, const int showFlag);

void refineSalientObj(Mat &saliencyObj);

void getSaliencyObj(Mat &saliencyObj, const Mat &_saliencyMap, const Mat &LABImg, const int thres0);

#endif // CUTOBJ

