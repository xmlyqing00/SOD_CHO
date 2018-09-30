#ifndef SEGMENT_H
#define SEGMENT_H

#include "comman.h"
#include "type_que.h"

float getMinIntDiff(int SEGMENT_THRESHOLD, int regionSize);

int Point2Index(Point u, int width);

void overSegmentation(Mat &pixelRegion, int &regionCount, const Mat &LABImg);

void segmentImage(Mat &W, Mat &pixelRegion, int &regionCount, const Mat &LABImg);

#endif // SEGMENT_H