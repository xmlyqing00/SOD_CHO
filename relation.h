#ifndef RELATION_H
#define RELATION_H

#include "comman.h"
#include "type_tarjan.h"

void rasterizeLine( vector<Point> &pixelBound, Point p0, Point p1 ) {

	int dx = p1.x - p0.x;
	int dy = p1.y - p0.y;

	if ( dx == 0 && dy == 0 ) {
		pixelBound.push_back( p0 );
		return;
	}

	if ( abs(dx) <= abs(dy) ) {

		if ( dy < 0 ) {
			swap( p0, p1 );
			dx = -dx;
			dy = -dy;
		}

		double slope = (double)dx / dy;
		for ( int step = 1; step < dy; step++ ) {
			int _y = p0.y + step;
			int _x = p0.x + cvRound( slope * step );
			pixelBound.push_back( Point( _x, _y ) );
		}

	} else {
		if ( dx < 0 ) {
			swap( p0, p1 );
			dx = -dx;
			dy = -dy;
		}

		double slope = (double)dy / dx;
		for ( int step = 1; step < dx; step++ ) {
			int _x = p0.x + step;
			int _y = p0.y + cvRound( slope * step );
			pixelBound.push_back( Point( _x, _y ) );
		}

	}
}

void getHorizontalBound(vector<Point> &horizontalBound, const vector<Point> &regionBound) {

	horizontalBound.push_back(regionBound[0]);
	for (size_t i = 1; i < regionBound.size(); i++) {

		horizontalBound.push_back(regionBound[i]);
		rasterizeLine(horizontalBound, regionBound[i - 1], regionBound[i]);
	}
	rasterizeLine(horizontalBound, regionBound[regionBound.size() - 1], regionBound[0]);

	sort( horizontalBound.begin(), horizontalBound.end(), cmpPoint);
}

void getOverlap(Mat &regionOverlap, const int regionIdx, const Mat &pixelRegion,
				const vector<Point> &horizontalBound) {

	for ( size_t i = 0; i < horizontalBound.size(); ) {

		size_t j;
		for ( j = i; j < horizontalBound.size(); j++ ) {
			if ( horizontalBound[j].y != horizontalBound[i].y ) break;
		}
		j--;
		int y = horizontalBound[i].y;
		for ( int x = horizontalBound[i].x; x <= horizontalBound[j].x; x++ ) {

			int neighbourIdx = pixelRegion.ptr<int>( y )[x];
			if (neighbourIdx == regionIdx) continue;
			regionOverlap.ptr<int>(regionIdx)[neighbourIdx]++;
		}
		i = j + 1;
	}
}

float getCoveringValue(float overlap0, float overlap1) {

	if (max(overlap0, overlap1) < REGION_CONNECTED) {
		return -INF;
	} else {
		if (overlap0 > overlap1) {

			if (overlap1 == 0) return 1;
			float tmp = (float)overlap0 / overlap1;
			tmp = 1 - pow(e, 1 - tmp);
			return (abs(tmp) < REGION_COVERING) ? 0 : tmp;
		} else {

			if (overlap0 == 0) return -1;
			float tmp = (float)overlap1 / overlap0;
			tmp = pow(e, 1 - tmp) - 1;
			return (abs(tmp) < REGION_COVERING) ? 0 : tmp;
		}
	}
}

void getInitRelation(Mat &regionRelation, Mat &regionRoute, const Mat &pixelRegion,
					 const vector<Point> *regionElement, const int *regionElementCount,
					 const int regionCount) {

	Mat regionOverlap(regionCount, regionCount, CV_32SC1, Scalar(0));

	for ( int i = 0; i < regionCount; i++ ) {

		vector<Point> regionBound;
		convexHull( regionElement[i], regionBound );

		vector<Point> horizontalBound;
		getHorizontalBound(horizontalBound, regionBound);

		getOverlap(regionOverlap, i, pixelRegion, horizontalBound);

//		if (i == 36 || i == 63 || i == 1) {
//			cout << i << endl;
//			Mat tmp(1300, 1300, CV_8UC1, Scalar(0));
//			for (size_t j = 0; j < horizontalBound.size(); j++) {
//				//cout << horizontalBound[j] << endl;
//				tmp.ptr<uchar>(horizontalBound[j].y)[horizontalBound[j].x] = 255;
//			}
//			resize(tmp, tmp, Size(), 0.5, 0.5);
//			imshow("tmp", tmp);
//			waitKey(0);
//		}
	}

	regionRelation = Mat(regionOverlap.size(), CV_32FC1, Scalar(0));
	regionRoute = Mat(regionOverlap.size(), CV_32SC1, Scalar(0));

	for (int i = 0; i < regionCount; i++) {

		for (int j = i + 1; j < regionCount; j++) {

			float overlap0 = (float)regionOverlap.ptr<int>(i)[j] / regionElementCount[j];
			float overlap1 = (float)regionOverlap.ptr<int>(j)[i] / regionElementCount[i];
			regionRelation.ptr<float>(i)[j] = getCoveringValue(overlap0, overlap1);
			if (regionRelation.ptr<float>(i)[j] == -INF ) {
				regionRoute.ptr<int>(i)[j] = 0;
				regionRoute.ptr<int>(j)[i] = 0;
			} else {
				regionRelation.ptr<float>(j)[i] = -regionRelation.ptr<float>(i)[j];
				regionRoute.ptr<int>(i)[j] = 1;
				regionRoute.ptr<int>(j)[i] = 1;
			}
		}
	}
}



void getRegionRelation(Mat &regionRelation, Mat &regionRoute, Mat &pixelRegion, int &regionCount) {

	int *regionElementCount = new int[regionCount];
	vector<Point> *regionElement = new vector<Point>[regionCount];
	for (int i = 0; i < regionCount; i++) {
		regionElementCount[i] = 0;
		regionElement[i].clear();
	}
	getRegionElement(regionElement, regionElementCount, pixelRegion);

	getInitRelation(regionRelation, regionRoute, pixelRegion, regionElement, regionElementCount, regionCount);
	for (int i = 0; i < regionCount; i++) regionElement->clear();
	delete[] regionElement;
	delete[] regionElementCount;

}

#endif // RELATION_H

