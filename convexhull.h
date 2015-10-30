#ifndef CONVEXHULL_H
#define CONVEXHULL_H

#include "comman.h"

void getConvexHull(vector< vector<Point> > &regionBound, const Mat &pixelRegion,
                   const int regionCount, const vector<Vec3b> &regionColor) {

    int *regionElementCount = new int[regionCount];
    vector<Point> *regionElement = new vector<Point>[regionCount];
    for (int i = 0; i < regionCount; i++) {
        regionElementCount[i] = 0;
        regionElement[i].clear();
    }
	getRegionElement(regionElement, regionElementCount, pixelRegion);

	regionBound = vector< vector<Point> >(regionCount);
	for ( int i = 0; i < regionCount; i++ ) convexHull( regionElement[i], regionBound[i] );

    for (int i = 0; i < regionCount; i++) regionElement->clear();
    delete[] regionElement;
    delete[] regionElementCount;
}

#endif // CONVEXHULL_H

