#ifndef MERGE
#define MERGE

#include "comman.h"
#include "convexhull.h"

void getSpatialCorrelation(Mat &regionSpatialCorrelation, const Mat &pixelRegion, const int regionCount) {

    Point *regionCenter = new Point[regionCount];
    int *regionRadius = new int[regionCount];

    int *regionElementCount = new int[regionCount];
    vector<Point> *regionElement = new vector<Point>[regionCount];
    for (int i = 0; i < regionCount; i++) {
        regionElementCount[i] = 0;
        regionElement->clear();
    }
    getRegionElement(regionElement, regionElementCount, pixelRegion);

    for (int i = 0; i < regionCount; i++) {

        regionCenter[i] = Point(0, 0);
        for (size_t j = 0; j < regionElement[i].size(); j++) {
            regionCenter[i] += regionElement[i][j];
        }
        regionCenter[i].x /= regionElementCount[i];
        regionCenter[i].y /= regionElementCount[i];
    }

    for (int i = 0; i < regionCount; i++) {

        regionRadius[i] = 0;
        for (size_t j = 0; j < regionElement[i].size(); j++) {
            int distTmp = getPointDist(regionCenter[i], regionElement[i][j]);
            regionRadius[i] = max(regionRadius[i], distTmp);
        }
    }

    for (int i = 0; i < regionCount; i++) {
        for (int j = i + 1; j < regionCount; j++) {

            int centerDist = getPointDist(regionCenter[i], regionCenter[j]);
			if (REGION_CORRELATION * (regionRadius[i] + regionRadius[j]) >  centerDist) {
                regionSpatialCorrelation.ptr<uchar>(i)[j] = 1;
                regionSpatialCorrelation.ptr<uchar>(j)[i] = 1;
            } else {
                regionSpatialCorrelation.ptr<uchar>(i)[j] = 0;
                regionSpatialCorrelation.ptr<uchar>(j)[i] = 0;
            }
        }
    }

    delete[] regionCenter;
    delete[] regionRadius;
    delete[] regionElementCount;
    for (int i = 0; i < regionCount; i++) regionElement->clear();
    delete[] regionElement;

}

int getLineLength(const TypeLine &line) {
	return getPointDist(line.u, line.v);
}

float crossProduct(const Point &v0, const Point &v1) {
	return v0.x * v1.y - v0.y * v1.x;
}

float dotProduct(const Point &v0, const Point &v1) {
	return v0.x * v1.x + v0.y * v1.y;
}

float getVectorNorm(const Point &v) {
	return sqrt(v.x * v.x + v.y * v.y);
}

int float2sign(const float &f) {
    return (f < -FLOAT_EPS) ? -1 : (f > FLOAT_EPS);
}

void fitBoundary(Point &p, const Size &boundSize) {

	p.x = min(max(p.x, 0), boundSize.width);
	p.y = min(max(p.y, 0), boundSize.height);
}

Point getIntersectP(TypeLine line0, TypeLine line1) {

    float t1, t2;
    Point intersectP(-1, -1);

	t1 = crossProduct(line0.u-line0.v, line1.u-line1.v);
    if (float2sign(t1) == 0) {

		t2 = crossProduct(line1.u-line0.u, line1.v-line0.u);
        if (float2sign(t2) == 0) { // overlap
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

		const Point *polyArray[1] = {&poly[0]};
		const int polyPts[1] = {(int)poly.size()};
		fillPoly(extendPoly, polyArray, polyPts, 1, Scalar(255), 8);
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

		//if (getLineLength(line0) < LINE_LENGTH || getLineLength(line1) < LINE_LENGTH) continue;

        Point intersectP = getIntersectP(line0, line1);
		bool intersectExist;

        if (intersectP.x == -1 && intersectP.y == -1) {
			intersectExist = 1;
		} else {
			int tmp1 = float2sign(crossProduct(line1.u-line0.u, line1.v-line0.u));
			int tmp2 = float2sign(crossProduct(line1.u-line0.u, intersectP-line0.u));

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
			intersectP = line0.u + REGION_CORRELATION * (line0.v-line0.u);
			fitBoundary(intersectP, extendPoly.size());
			_poly.push_back(intersectP);

			intersectP = line1.u + REGION_CORRELATION * (line1.v-line1.u);
			fitBoundary(intersectP, extendPoly.size());
			_poly.push_back(intersectP);
			break;
        }
    }

//	Mat tmp = extendPoly.clone();
	const Point *_polyArray[1] = {&_poly[0]};
	const int _polyPts[1] = {(int)_poly.size()};
	fillPoly(extendPoly, _polyArray, _polyPts, 1, Scalar(255), 8);

//	const Point *polyArray[1] = {&poly[0]};
//	const int polyPts[1] = {(int)poly.size()};
//	fillPoly(tmp, polyArray, polyPts, 1, Scalar(255), 8);
//	for (size_t i = 0; i < poly.size(); i++) {
//		circle(tmp, poly[i], 5, Scalar(255));
//	}

//	imshow("poly", tmp);
//	imshow("extend poly", extendPoly);
//	waitKey(0);
}

bool contourCompletion(const Mat &extendPoly0, const Mat &extendPoly1) {

	Mat maskMat = extendPoly0 & extendPoly1;
	Scalar overlapCount = sum(maskMat);

	float tmp = (float)overlapCount.val[0] / (maskMat.rows * maskMat.cols);
    if (tmp > CONTOUR_COMPLETION) {
        return true;
    } else {
        return false;
    }

}

void mergeRegion(Mat &pixelRegion, int &regionCount, vector<Vec3b> &regionColor) {

	vector< vector<Point> > regionBound;
	getConvexHull(regionBound, pixelRegion, regionCount, regionColor);

	vector<Mat> extendPoly(regionCount);
	for (int i = 0; i < regionCount; i++) {

		extendPoly[i] = Mat(pixelRegion.size(), CV_8UC1, Scalar(0));

		if (regionBound[i].size() < 4) {

			getExtendPoly(extendPoly[i], regionBound[i]);

		} else {

			vector<Point> _poly;
			_poly.push_back(regionBound[i][0]);
			for (size_t j = 1; j < regionBound[i].size()-1; j++) {
				if (getPointDist(_poly.back(), regionBound[i][j]) > LINE_LENGTH) {
					_poly.push_back(regionBound[i][j]);
				}
			}

			if (getPointDist(_poly.back(), regionBound[i].back()) > LINE_LENGTH &&
				getPointDist(_poly[0], regionBound[i].back()) > LINE_LENGTH) {
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
				if (cosRes > LINE_ANGLE) poly.push_back(_poly[j]);

			}

			if (poly.size() < 3) {
				poly = _poly;
			}

			getExtendPoly(extendPoly[i], poly);
		}
	}

	Mat regionSpatialCorrelation(regionCount, regionCount, CV_8UC1);
	getSpatialCorrelation(regionSpatialCorrelation, pixelRegion, regionCount);

	int *replaceHead = new int[regionCount];
	int *replaceCount = new int[regionCount];
	Vec3b *replaceColor = new Vec3b[regionCount];
	for (int i = 0; i < regionCount; i++) {
		replaceHead[i] = i;
		replaceCount[i] = 1;
		replaceColor[i] = regionColor[i];
	}

    for (int i = 0; i < regionCount; i++) {
        for (int j = i + 1; j < regionCount; j++) {

            if (regionSpatialCorrelation.ptr<uchar>(i)[j] == 0) continue;
			if (colorDiff(regionColor[i], regionColor[j]) > COLOR_DIFF) continue;
			if (contourCompletion(extendPoly[i], extendPoly[j]) == false) continue;

            int pa0 = getElementHead(i, replaceHead);
            int pa1 = getElementHead(j, replaceHead);
            Vec3i color = (Vec3i)replaceColor[pa0] * replaceCount[pa0] +
                          (Vec3i)replaceColor[pa1] * replaceCount[pa1];
            replaceCount[pa0] += replaceCount[pa1];
            replaceColor[pa0] = color / replaceCount[pa0];
            replaceHead[pa1] = pa0;

        }
    }

    int *regionBucket = new int[regionCount];
    memset(regionBucket, 0, sizeof(int)*(regionCount));
    for (int i = 0; i < regionCount; i++) {
        regionBucket[getElementHead(i, replaceHead)] = 1;
    }

    int _regionCount = 0;
    for (int i = 0; i < regionCount; i++) {
        if (regionBucket[i] == 1) regionBucket[i] = _regionCount++;
    }

    for (int y = 0; y < pixelRegion.rows; y++) {
        for (int x = 0; x < pixelRegion.cols; x++) {

            int regionIdx = pixelRegion.ptr<int>(y)[x];
            pixelRegion.ptr<int>(y)[x] = regionBucket[replaceHead[regionIdx]];
        }
    }

    //for (int i = 1; i <= regionCount; i++) cout << i << " " << replaceByIdx[i] << " " << regionBucket[i] << " " << regionColor[i] << endl;

    vector<Vec3b> _regionColor(_regionCount);
    for (int i = 0; i < regionCount; i++) {
        int regionIdx = regionBucket[replaceHead[i]];
        _regionColor[regionIdx] = regionColor[i];
    }
    regionColor = _regionColor;
    regionCount = _regionCount;

    delete[] regionBucket;
    delete[] replaceHead;
    delete[] replaceCount;
    delete[] replaceColor;

	cout << regionCount << endl;
	writeRegionImage(regionCount, pixelRegion, "Merge_Region.png");
}

#endif // MERGE

