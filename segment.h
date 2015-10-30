#ifndef SEGMENT_H
#define SEGMENT_H

#include "comman.h"
#include "type_que.h"

int getMinIntDiff(int SEGMENT_THRESHOLD, int regionSize) {
	return SEGMENT_THRESHOLD / regionSize;
}

int Point2Index(Point u, int width) {
	return u.y * width + u.x;
}

void segmentImage( Mat &pixelRegion, int &regionCount, vector<Vec3b> &regionColor,
				   const Mat &cannyImg, const Mat &inputImg ) {

	// segment image
	vector<TypeEdge> edges;
	pixelRegion = Mat::zeros( inputImg.size(), CV_32SC1 );
	const int leftside[4] = {2, 3, 5, 7};
	//const int SEGMENT_THRESHOLD = inputImg.rows * inputImg.cols / 50;
	const int SEGMENT_THRESHOLD = 2 * max(inputImg.rows, inputImg.cols);

	for (int y = 0; y < inputImg.rows; y++) {
		for (int x = 0; x < inputImg.cols; x++) {

			Point nowP = Point(x, y);

			for (int k = 0; k < 4; k++) {

				Point newP = nowP + dxdy[leftside[k]];
				if ( isOutside( newP.x, newP.y, inputImg.cols, inputImg.rows ) ) continue;
				Vec3b nowColor = inputImg.ptr<Vec3b>(nowP.y)[nowP.x];
				Vec3b newColor = inputImg.ptr<Vec3b>(newP.y)[newP.x];
				int diff = colorDiff(nowColor, newColor);
				edges.push_back(TypeEdge(nowP, newP, diff));
			}
		}
	}

	sort(edges.begin(), edges.end(), cmpTypeEdge);

	int pixelCount = inputImg.rows * inputImg.cols;
	int *regionHead = new int[pixelCount];
	int *regionSize = new int[pixelCount];
	int *minIntDiff = new int[pixelCount];
	for (int i = 0; i < pixelCount; i++) {
		regionHead[i] = i;
		regionSize[i] = 1;
		minIntDiff[i] = getMinIntDiff(SEGMENT_THRESHOLD, 1);
	}

	for (size_t i = 0; i < edges.size(); i++) {

		int uIdx = Point2Index(edges[i].u, inputImg.cols);
		int vIdx = Point2Index(edges[i].v, inputImg.cols);
		int pu = getElementHead(uIdx, regionHead);
		int pv = getElementHead(vIdx, regionHead);
		if (pu == pv) continue;

		if (edges[i].w <= minIntDiff[pu] && edges[i].w <= minIntDiff[pv]) {

			regionHead[pv] = pu;
			regionSize[pu] += regionSize[pv];
			minIntDiff[pu] = edges[i].w + getMinIntDiff(SEGMENT_THRESHOLD, regionSize[pu]);
		} else {
			//cout << endl;
		}
	}

	delete[] minIntDiff;

	int idx = 0;
	regionCount = 0;
	int minRegionSize = REGION_SIZE * (inputImg.rows * inputImg.cols);
	int *regionIndex = new int[pixelCount];
	for (int i = 0; i < pixelCount; i++) regionIndex[i] = -1;

	for (int y = 0; y < inputImg.rows; y++) {
		for (int x = 0; x < inputImg.cols; x++) {

			int pIdx = getElementHead(idx, regionHead);
			if (regionSize[pIdx] < minRegionSize) {
				pixelRegion.ptr<int>(y)[x] = -1;
			} else {
				if (regionIndex[pIdx] == -1) {
					pixelRegion.ptr<int>(y)[x] = regionCount;
					regionIndex[pIdx] = regionCount++;
				} else {
					pixelRegion.ptr<int>(y)[x] = regionIndex[pIdx];
				}
			}
			idx++;
		}
	}
	delete[] regionHead;
	delete[] regionSize;
	delete[] regionIndex;

	// get region typical color
	map<int, int> *colorBucket = new map<int, int>[regionCount];

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			if (regionIdx == -1) continue;
			colorBucket[regionIdx][hashVec3b(inputImg.ptr<Vec3b>(y)[x])]++;
		}
	}

	regionColor = vector<Vec3b>(regionCount);

	for (int i = 0; i < regionCount; i++) {

		map<int, int>::iterator it;

		int mostAppearColorHash = 0;
		int mostAppearTimes = 0;

		for (it = colorBucket[i].begin(); it != colorBucket[i].end(); ++it) {

			//cout << it->first << " " << it->second << endl;
			if (it->second > mostAppearTimes) {
				mostAppearTimes = it->second;
				mostAppearColorHash = it->first;
			}
		}

		regionColor[i] = deHashVec3b(mostAppearColorHash);
	}

	delete[] colorBucket;

	// cluster small regions
	TypeQue<Point> &que = *(new TypeQue<Point>);
	for ( int y = 0; y < pixelRegion.rows; y++ ) {
		for ( int x = 0; x < pixelRegion.cols; x++ ) {
			if ( pixelRegion.ptr<int>( y )[x] != -1 ) que.push( Point( x, y ) );
		}
	}

	while ( !que.empty() ) {

		Point nowP = que.front();
		que.pop();

		int regionIdx = pixelRegion.ptr<int>(nowP.y)[nowP.x];

			for ( int k = 0; k < PIXEL_CONNECT; k++ ) {

			Point newP = nowP + dxdy[k];

			if ( isOutside( newP.x, newP.y, pixelRegion.cols, pixelRegion.rows ) ) continue;
			if ( pixelRegion.ptr<int>( newP.y )[newP.x] != -1 ) continue;

			pixelRegion.ptr<int>( newP.y )[newP.x] = regionIdx;
			que.push( Point( newP.x, newP.y ) );
		}
	}

	delete &que;



	cout << regionCount << endl;
	writeRegionImage(regionCount, pixelRegion, "Segment_Region.png");

}

#endif // SEGMENT_H
