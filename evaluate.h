#ifndef EVALUATE
#define EVALUATE

#include "comman.h"
#include "type_file.h"

void getGroundTruth(map<string, Mat> &binaryMask, TypeFile &fileSet);

void evaluateMap(double &precision, double &recall, const Mat &mask, const Mat &saliencyMap);

void compResults_10K();

void benchMark(char *datasetName);

#endif // EVALUATE
