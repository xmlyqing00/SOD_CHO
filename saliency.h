#ifndef SALIENCY_H
#define SALIENCY_H

#include "comman.h"
#include "type_que.h"
#include "type_region.h"

double calcCenterW(const Point &regionCenter, const Size &imgSize);

void calcCHOMap(vector<TypeRegionSet> &multiLayerModel, const Size &imgSize);

void calcContrastMap(vector<TypeRegionSet> &multiLayerModel, const Size &imgSize);

void calcSaliencyMap(Mat &saliencyMap, Mat &CHOMap, Mat &contrastMap, const vector<TypeRegionSet> &multiLayerModel);

void getCHODetail(Mat &CHODetailMap, const int &objIdx, const Mat &objPixelRegion, const int &bgIdx, const Mat &bgPixelRegion, const vector<Point> &regionBound, const Mat &LABImg);

void calcBorderMap(Mat &borderMap, const vector<TypeRegionSet> &multiLayerModel);

void updateColorSmooth(Mat &saliencyMap, const Mat &paletteMap);

void updateRegionSmooth(Mat &saliencyMap, const vector<TypeRegionSet> &multiLayerModel);

void getSaliencyMap(Mat &saliencyMap, vector<TypeRegionSet> &multiLayerModel, const Mat &LABImg);

#endif // SALIENCY_H
