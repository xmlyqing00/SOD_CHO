#ifndef GRAPH_H
#define GRAPH_H

#include "comman.h"

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

int getCoveringValue(double overlap0, double overlap1) {

	if (max(overlap0, overlap1) < MIN_REGION_CONNECTED) {
		return -2;
	} else {

		double tmp0 = e / (e - 1);
		double tmp = -pow(e, -overlap0 / 1) + pow(e, -overlap1 / 1);
		tmp = tmp * tmp0;

		if (abs(tmp) < MIN_COVERING) return 0;
		return tmp > 0 ? 1 : -1;
	}
}

void buildRegionGraph(Mat &W, Mat &D, vector<int> &frontRegion,
					  const Mat *pyramidRegion, const vector< vector<int> > *pyramidMap,
					  const vector<Vec3b> &regionColor, const double GAMA) {

	int baseRegionCount = pyramidMap[0].size();
	W = Mat(baseRegionCount, baseRegionCount, CV_64FC1, Scalar(0));
	D = Mat(baseRegionCount, baseRegionCount, CV_64FC1, Scalar(0));
	Mat c(baseRegionCount, baseRegionCount, CV_64FC1, Scalar(0));

	// init W
	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = 0; j < baseRegionCount; j++) {
			double w = colorDiff(regionColor[i], regionColor[j]);
			W.ptr<double>(i)[j] = w;
		}
	}

	// update c
	frontRegion = vector<int>(baseRegionCount, 0);

	for (int pyramidIdx = 0; pyramidIdx < PYRAMID_SIZE; pyramidIdx++) {

		int regionCount = pyramidMap[pyramidIdx].size();
		int *regionElementCount = new int[regionCount];
		vector<Point> *regionElement = new vector<Point>[regionCount];
		for (int i = 0; i < regionCount; i++) {
			regionElementCount[i] = 0;
			regionElement[i].clear();
		}
		getRegionElement(regionElement, regionElementCount, pyramidRegion[pyramidIdx]);

		Mat regionOverlap(regionCount, regionCount, CV_32SC1, Scalar(0));

		for ( int i = 0; i < regionCount; i++ ) {

			vector<Point> regionBound;
			convexHull( regionElement[i], regionBound );

			vector<Point> horizontalBound;
			getHorizontalBound(horizontalBound, regionBound);

			getOverlap(regionOverlap, i, pyramidRegion[pyramidIdx], horizontalBound);
		}

		int *tmpFrontRegion = new int[regionCount];
		memset(tmpFrontRegion, 0, sizeof(int)*regionCount);
		Mat tmpc(regionCount, regionCount, CV_64FC1, Scalar(0));

		for (int i = 0; i < regionCount; i++) {

			for (int j = i + 1; j < regionCount; j++) {

				double overlap0 = (double)regionOverlap.ptr<int>(i)[j] / regionElementCount[j];
				double overlap1 = (double)regionOverlap.ptr<int>(j)[i] / regionElementCount[i];
				int regionRelation = getCoveringValue(overlap0, overlap1);

				switch (regionRelation) {
				case -1:
					tmpFrontRegion[i]++;
					tmpFrontRegion[j]--;
					tmpc.ptr<double>(i)[j]++;
					break;
				case 1:
					tmpFrontRegion[i]--;
					tmpFrontRegion[j]++;
					tmpc.ptr<double>(i)[j]++;
					break;
				case 0:
				case -2:
					break;
				}
			}
		}

		for (int i = 0; i < regionCount; i++) {
			for (size_t j = 0; j < pyramidMap[pyramidIdx][i].size(); j++) {

				frontRegion[pyramidMap[pyramidIdx][i][j]] += tmpFrontRegion[i];
				for (size_t k = 0; k < pyramidMap[pyramidIdx][i].size(); k++) {

					if (j == k) continue;

					c.ptr<double>(pyramidMap[pyramidIdx][i][j])[pyramidMap[pyramidIdx][i][k]]--;
				}
			}
		}

		delete[] tmpFrontRegion;

		for (int i = 0; i < regionCount; i++) {
			for (int j = i + 1; j < regionCount; j++) {

				if (tmpc.ptr<double>(i)[j] == 0) continue;

				for (size_t mapi_ele = 0; mapi_ele < pyramidMap[pyramidIdx][i].size(); mapi_ele++) {
					for (size_t mapj_ele = 0; mapj_ele < pyramidMap[pyramidIdx][j].size(); mapj_ele++) {
						c.ptr<double>(pyramidMap[pyramidIdx][i][mapi_ele])[pyramidMap[pyramidIdx][j][mapj_ele]]++;
					}
				}
			}
		}

		delete[] regionElement;
		delete[] regionElementCount;
	}

	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = i + 1; j < baseRegionCount; j++) {
			W.ptr<double>(i)[j] = W.ptr<double>(i)[j] * pow(1+GAMA, c.ptr<double>(i)[j]);
			W.ptr<double>(j)[i] = W.ptr<double>(i)[j];
		}
	}

	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = 0; j < baseRegionCount; j++) {
			D.ptr<double>(i)[i] = D.ptr<double>(i)[i] + W.ptr<double>(i)[j];
		}
	}
}

#endif // GRAPH_H

