#ifndef EVALUATE
#define EVALUATE

#include "comman.h"

void getGroundTruth(map<string,Mat> &binaryMask, const char *dirName);

bool evaluateMap(double &precision, double &recall, const Mat &mask, const Mat &saliencyMap);

void compResults_10K();

void benchMark(char *datasetName);

#endif // EVALUATE