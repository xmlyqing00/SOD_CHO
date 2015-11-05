#ifndef SEGMENT_H
#define SEGMENT_H

#include "comman.h"
#include "type_que.h"

float getMinIntDiff(int SEGMENT_THRESHOLD, int regionSize) {
	return (float)SEGMENT_THRESHOLD / regionSize;
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
	const int SEGMENT_THRESHOLD = 5 * max(inputImg.rows, inputImg.cols);

	for (int y = 0; y < inputImg.rows; y++) {
		for (int x = 0; x < inputImg.cols; x++) {

//			if (y == 304 && x == 55) {
//				cout << endl;
//			}
			if (cannyImg.ptr<uchar>(y)[x] == 255) continue;

			Point nowP = Point(x, y);

			for (int k = 0; k < 4; k++) {

				Point newP = nowP + dxdy[leftside[k]];
				if (isOutside(newP.x, newP.y, inputImg.cols, inputImg.rows)) continue;

				if (cannyImg.ptr<uchar>(newP.y)[newP.x] == 255) continue;

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
	float *minIntDiff = new float[pixelCount];
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

		//if (edges[i].w > 1) break;

		if (edges[i].w <= minIntDiff[pu] && edges[i].w <= minIntDiff[pv]) {

			//cout << edges[i].u << " " << edges[i].v << " " << edges[i].w << endl;
			regionHead[pv] = pu;
			regionSize[pu] += regionSize[pv];
			minIntDiff[pu] = edges[i].w + getMinIntDiff(SEGMENT_THRESHOLD, regionSize[pu]);
		}
	}

	delete[] minIntDiff;

	// get main region
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
	delete[] regionIndex;
	delete[] regionSize;

	//writeRegionImage(regionCount, pixelRegion, "Segment_Init.png");

	// get region represent color
	Vec3i *_regionColor = new Vec3i[regionCount];
	regionSize = new int[regionCount];
	for (int i = 0; i < regionCount; i++) {
		_regionColor[i] = Vec3i(0, 0, 0);
		regionSize[i] = 0;
	}

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			if (regionIdx != -1) {
				_regionColor[regionIdx] += inputImg.ptr<Vec3b>(y)[x];
				regionSize[regionIdx]++;
			}
		}
	}

	regionColor = vector<Vec3b>(regionCount);

	for (int i = 0; i < regionCount; i++) {
		for (int k = 0; k < 3; k++) {
			regionColor[i].val[k] = _regionColor[i].val[k] / regionSize[i];
		}
	}

	delete[] _regionColor;
	delete[] regionSize;

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
