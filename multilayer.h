#ifndef MULTILAYER_H
#define MULTILAYER_H

#include "comman.h"
#include "type_que.h"
#include "type_region.h"

float getMinIntDiff(int SEGMENT_THRESHOLD, int regionSize);

int Point2Index(Point u, int width);

void overSegmentation(Mat &pixelRegion, int &regionCount, const Mat &LABImg);

void buildMultiLayerModel(vector<TypeRegionSet> &multiLayerModel, const Mat &paletteMap, const Mat &colorImg);

#endif // MULTILAYER_H
