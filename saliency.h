#ifndef SALIENCY_H
#define SALIENCY_H

#include "comman.h"
#include "type_que.h"

void getOverlap(vector<int> &regionOverlap, const Mat &curRegionMap, const int &curIdx, const Mat &baseRegionMap, const Mat &convexMap);

void getCenterBias(double &centerBias, const vector<Point> &pts, const Point &midP);

void normalizeVecd(vector<double> &vec);

void getCHODetail(Mat &CHODetailMap, const int &objIdx, const Mat &objPixelRegion, const int &bgIdx, const Mat &bgPixelRegion, const vector<Point> &regionBound, const Mat &LABImg);

void getCHOSaliencyMap(Mat &saliencyMap, const vector<int> &regionCount, const vector<Mat> &pyramidRegion, const Mat &LABImg);

void updateMixContrast(Mat &_saliencyMap, const Mat &pixelRegion, const int regionCount, const Mat &LABImg);

void updateborderMap(Mat &saliencyMap, Mat &borderMap, const Mat &pixelRegion, const int regionCount);

void quantizeColorSpace(Mat &colorMap, vector<Vec3f> &platte, const Mat &colorImg);

void updateColorSmooth(Mat &saliencyMap, const Mat &LABImg);

void updateRegionSmooth(Mat &saliencyMap, const Mat &pixelRegion, const int regionCount);

void getSaliencyMap(Mat &saliencyMap, const vector<int> &regionCount, const vector<Mat> &pyramidRegion, const Mat &over_pixelRegion, const int &over_regionCount, const Mat &LABImg);

#endif // SALIENCY_H
