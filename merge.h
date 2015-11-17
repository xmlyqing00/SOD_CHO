#ifndef MERGE
#define MERGE

#include "comman.h"

void getConvexHull(vector< vector<Point> > &regionBound, const Mat &pixelRegion,
				   const int regionCount) {

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

int getLineLength(const TypeLine &line) {
	return getPointDist(line.u, line.v);
}

int crossProduct(const Point &v0, const Point &v1) {
	return v0.x * v1.y - v0.y * v1.x;
}

int dotProduct(const Point &v0, const Point &v1) {
	return v0.x * v1.x + v0.y * v1.y;
}

double getVectorNorm(const Point &v) {
	return sqrt(v.x * v.x + v.y * v.y);
}

void fitBoundary(Point &p, const Size &boundSize) {

	p.x = min(max(p.x, 0), boundSize.width);
	p.y = min(max(p.y, 0), boundSize.height);
}

Point getIntersectP(TypeLine line0, TypeLine line1) {

	double t1, t2;
	Point intersectP(-1, -1);

	t1 = crossProduct(line0.u-line0.v, line1.u-line1.v);
	if (t1 == 0) {

		t2 = crossProduct(line1.u-line0.u, line1.v-line0.u);
		if (t2 == 0) { // overlap
			cout << "ERROR : OVERLAP";
		} else { // parallel
		}

	} else {

		t2 = crossProduct(line0.u-line1.u, line1.u-line1.v);
		t1 = t2 / t1;

		intersectP.x = cvRound(line0.u.x + (line0.v.x - line0.u.x) * t1);
		intersectP.y = cvRound(line0.u.y + (line0.v.y - line0.u.y) * t1);

	}

	return intersectP;

}

void getExtendPoly(Mat &extendPoly, const vector<Point> &poly) {

	if (poly.size() < 4) {

		if (poly.size() < 3) return;

		const Point *polyArray[1] = {&poly[0]};
		const int polyPts[1] = {(int)poly.size()};
		fillPoly(extendPoly, polyArray, polyPts, 1, Scalar(1), 8);
		return;
	}

	vector<Point> _poly;

	for (size_t i = 0; i < poly.size(); i++) {

		_poly.push_back(poly[i]);

		TypeLine line0, line1;
		line0.v = poly[i];
		line0.u = poly[(i-1+poly.size()) % poly.size()];
		line1.v = poly[(i+1) % poly.size()];
		line1.u = poly[(i+2) % poly.size()];

		//if (getLineLength(line0) < PARAM3_LINE_LENGTH || getLineLength(line1) < PARAM3_LINE_LENGTH) continue;

		Point intersectP = getIntersectP(line0, line1);
		bool intersectExist;

		if (intersectP.x == -1 && intersectP.y == -1) {
			intersectExist = 1;
		} else {
			int tmp1 = crossProduct(line1.u-line0.u, line1.v-line0.u);
			int tmp2 = crossProduct(line1.u-line0.u, intersectP-line0.u);

			if (tmp1 == 0 || tmp2 == 0) {
				intersectExist = 2;
			} else if (tmp1 * tmp2 < 0) {
				intersectExist = 1;
			} else {

				Point midP = line0.v + line1.v;
				midP.x /= 2;
				midP.y /= 2;
				tmp1 = getPointDist(line0.v, line1.v);
				tmp2 = getVectorNorm(intersectP - midP);
				float scale = float(tmp1) / tmp2;
				if (scale < 1) {
					intersectP.x = midP.x + cvRound(scale * (intersectP.x - midP.x));
					intersectP.y = midP.y + cvRound(scale * (intersectP.y - midP.y));
				}

				intersectExist = 0;
			}
		}

		switch (intersectExist) {
		case 0:
			fitBoundary(intersectP, extendPoly.size());
			_poly.push_back(intersectP);
			break;
		case 1:
			intersectP = line0.u + CONVEX_EXTENSION_SIZE * (line0.v-line0.u);
			fitBoundary(intersectP, extendPoly.size());
			_poly.push_back(intersectP);

			intersectP = line1.u + CONVEX_EXTENSION_SIZE * (line1.v-line1.u);
			fitBoundary(intersectP, extendPoly.size());
			_poly.push_back(intersectP);
			break;
		}
	}

	//Mat tmp = extendPoly.clone();
	const Point *_polyArray[1] = {&_poly[0]};
	const int _polyPts[1] = {(int)_poly.size()};
	fillPoly(extendPoly, _polyArray, _polyPts, 1, Scalar(1), 8);

//	const Point *polyArray[1] = {&poly[0]};
//	const int polyPts[1] = {(int)poly.size()};
//	fillPoly(tmp, polyArray, polyPts, 1, Scalar(1), 8);
//	for (size_t i = 0; i < poly.size(); i++) {
//		circle(tmp, poly[i], 5, Scalar(255));
//	}

//	imshow("poly", tmp);
//	imshow("extend poly", extendPoly);
//	waitKey(1);
}

bool checkCommanArea(const Mat &extendPoly0, const Mat &extendPoly1, const int area,
					   float MIN_COMMAN_AREA) {

	Mat maskMat = extendPoly0 & extendPoly1;
	Scalar overlapCount = sum(maskMat);
	//cout << overlapCount.val[0] << endl;
	float tmp = MIN_COMMAN_AREA * area;
	if (overlapCount.val[0] > tmp) {
		return true;
	} else {
		return false;
	}

}

void mergeRegion(Mat &pixelRegion, vector< vector<int> > &regionMap, vector<Vec3b> &regionColor,
				 const Mat &LABImg, const int COLOR_THRESHOLD) {

	int regionCount = regionMap.size();

	vector< vector<Point> > regionBound;
	getConvexHull(regionBound, pixelRegion, regionCount);

	vector<Mat> extendPoly(regionCount);
	for (int i = 0; i < regionCount; i++) {

		extendPoly[i] = Mat(pixelRegion.size(), CV_8UC1, Scalar(0));

		if (regionBound[i].size() < 4) {

			getExtendPoly(extendPoly[i], regionBound[i]);

		} else {

			vector<Point> _poly;
			_poly.push_back(regionBound[i][0]);
			for (size_t j = 1; j < regionBound[i].size()-1; j++) {
				if (getPointDist(_poly.back(), regionBound[i][j]) > MIN_LINE_LENGTH) {
					_poly.push_back(regionBound[i][j]);
				}
			}

			if (getPointDist(_poly.back(), regionBound[i].back()) > MIN_LINE_LENGTH &&
				getPointDist(_poly[0], regionBound[i].back()) > MIN_LINE_LENGTH) {
				_poly.push_back(regionBound[i].back());
			}

			vector<Point> poly;
			for (size_t j = 0; j < _poly.size(); j++) {

				int prevIdx = (j + 1) % _poly.size();
				int nextIdx = (j - 1 + _poly.size()) % _poly.size();
				float dotRes = dotProduct(_poly[prevIdx]-_poly[j], _poly[nextIdx]-_poly[j]);
				float norm1 = getVectorNorm(_poly[prevIdx]-_poly[j]);
				float norm2 = getVectorNorm(_poly[nextIdx]-_poly[j]);
				float cosRes = dotRes / (norm1 * norm2);
				if (cosRes > STRAIGHT_LINE_ANGLE) poly.push_back(_poly[j]);

			}

			getExtendPoly(extendPoly[i], poly);

		}
	}

	int *regionElementCount = new int[regionCount];
	vector<Point> *regionElement = new vector<Point>[regionCount];
	for (int i = 0; i < regionCount; i++) {
		regionElementCount[i] = 0;
		regionElement->clear();
	}
	getRegionElement(regionElement, regionElementCount, pixelRegion);

	int *replaceHead = new int[regionCount];
	for (int i = 0; i < regionCount; i++) replaceHead[i] = i;

	for (int i = 0; i < regionCount; i++) {
		for (int j = i + 1; j < regionCount; j++) {

			if (colorDiff(regionColor[i], regionColor[j]) > COLOR_THRESHOLD) continue;
			int area = 0.5 * (regionElementCount[i] + regionElementCount[j]);
			if (checkCommanArea(extendPoly[i], extendPoly[j], area, MIN_COMMAN_AREA) == false) continue;

			int pa0 = getElementHead(i, replaceHead);
			int pa1 = getElementHead(j, replaceHead);
			replaceHead[pa1] = pa0;
		}
	}

	delete[] regionElementCount;
	delete[] regionElement;

	int *regionBucket = new int[regionCount];
	memset(regionBucket, 0, sizeof(int)*(regionCount));
	for (int i = 0; i < regionCount; i++) {
		regionBucket[getElementHead(i, replaceHead)] = 1;
	}

	int _regionCount = 0;
	for (int i = 0; i < regionCount; i++) {
		if (regionBucket[i] == 1) regionBucket[i] = _regionCount++;
	}

	vector< vector<int> > _regionMap = vector< vector<int> >(_regionCount);
	for (int i = 0; i < regionCount; i++) {
		int paIdx = regionBucket[getElementHead(i, replaceHead)];
		for (size_t j = 0; j < regionMap[i].size(); j++) {
			_regionMap[paIdx].push_back(regionMap[i][j]);
		}
	}

	regionMap = _regionMap;

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			int newRegionIdx = regionBucket[getElementHead(regionIdx, replaceHead)];
			pixelRegion.ptr<int>(y)[x] = newRegionIdx;
		}
	}

	regionCount = _regionCount;
	getRegionColor(regionColor, regionCount, pixelRegion, LABImg);

	delete[] regionBucket;
	delete[] replaceHead;

	//cout << regionCount << endl;
	//char outputFile[100];
	//sprintf(outputFile, "param_test/%d/merge/%s", testNum, outputName);
	//writeRegionImageRepresent(regionCount, pixelRegion, regionColor, outputFile);
	//writeRegionImageRepresent(regionCount, pixelRegion, regionColor, "Merge Image.png");

}

void buildPyramidRegion(Mat *pyramidRegion, vector< vector<int> > *pyramidMap,
						const Mat &pixelRegion, const int &regionCount, const Mat &LABImg,
						const vector<Vec3b> &_regionColor) {

	int COLOR_THRESHOLD[PYRAMID_SIZE] = {0, 8, 16, 24, 30, 34, 32};
	char pyramidName[100];
	const int color_step = 1;

	pyramidRegion[0] = pixelRegion.clone();
	pyramidMap[0] = vector< vector<int> >(regionCount);
	for (int i = 0; i < regionCount; i++) pyramidMap[0][i].push_back(i);
	vector<Vec3b> regionColor = _regionColor;

	sprintf(pyramidName, "Pyramid_%d.png", 0);
	writeRegionImageRepresent(pyramidMap[0].size(), pyramidRegion[0], regionColor, pyramidName, 0, 1);

	for (int pyramidIdx = 1; pyramidIdx < PYRAMID_SIZE; pyramidIdx++) {

		pyramidRegion[pyramidIdx] = pyramidRegion[pyramidIdx-1].clone();
		pyramidMap[pyramidIdx] = pyramidMap[pyramidIdx-1];

		int color_threshold = COLOR_THRESHOLD[pyramidIdx-1] + color_step;
		while (color_threshold <= COLOR_THRESHOLD[pyramidIdx]) {

			//cout << MERGE_THRESHOLD << endl;
			mergeRegion(pyramidRegion[pyramidIdx], pyramidMap[pyramidIdx], regionColor, LABImg, color_threshold);
			color_threshold += color_step;
		}

		sprintf(pyramidName, "Pyramid_%d.png", pyramidIdx);
		writeRegionImageRepresent(pyramidMap[pyramidIdx].size(), pyramidRegion[pyramidIdx], regionColor, pyramidName, 0, 1);
	}
}

#endif // MERGE
