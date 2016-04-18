#ifndef EVALUATE
#define EVALUATE

#include "comman.h"
#include "type_file.h"

void getGroundTruth(map<string,Mat> &binaryMask, const TypeFile *fileSet);

bool evaluateMap(double &precision, double &recall, const Mat &mask, const Mat &saliencyMap);

void compMaskOthers_1K();

void compMaskOthers_10K();

void compResults_10K();

#endif // EVALUATE
