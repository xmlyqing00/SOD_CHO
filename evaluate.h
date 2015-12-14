#ifndef EVALUATE
#define EVALUATE

#include "comman.h"

void getGroundTruth(map<string,Mat> &binaryMask, const char *dirName);

bool evaluateMap(double &precision, double &recall, const Mat &mask, const Mat &saliencyMap);

void compMaskOthers_1K();

void compMaskOthers_10K();

void compResults_10K();

#endif // EVALUATE
