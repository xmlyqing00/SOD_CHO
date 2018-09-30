#ifndef PYRAMID_H
#define PYRAMID_H

#include "comman.h"
#include "ncut.h"

void recursivePyramidRegion(vector< vector<int> > &pyramidRegionTag, const vector<int> &WtoRegion, const Mat &W, const int depth, const int regionIdx);

void buildPyramidRegion(vector<Mat> &pyramidRegion, vector<int> &_regionCount, const Mat &pixelRegion, const Mat &W);

#endif // PYRAMID_H