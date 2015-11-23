#ifndef SALIENCY_H
#define SALIENCY_H

#include "comman.h"

void getOverlap(vector<int> &regionOverlap, const Mat &curRegionMap, const int &curIdx,
				const Mat &baseRegionMap, const Mat &convexMap) {

	for (int y = 0; y < curRegionMap.rows; y++) {
		for (int x = 0; x < curRegionMap.cols; x++) {

			if (convexMap.ptr<uchar>(y)[x] != 255) continue;
			if (curRegionMap.ptr<int>(y)[x] == curIdx) continue;
			regionOverlap[baseRegionMap.ptr<int>(y)[x]]++;
		}
	}
}

void getCenterBias(double &centerBias, const vector<Point> &pts, const Point &midP) {

	Point2d bias(0, 0);
	int width = midP.x << 1;
	int height = midP.y << 1;
	for (size_t i = 0; i < pts.size(); i++) {
		bias += Point2d(abs(pts[i].x - midP.x), abs(pts[i].y - midP.y));
	}

	bias.x /= width * pts.size();
	bias.y /= height * pts.size();
	centerBias = exp(-9.0 * (sqr(bias.x) + sqr(bias.y)));

}

void getSaliencyMap(Mat &saliencyMap, const vector<int> &regionCount, const vector<Mat> &pyramidRegion) {

	vector<double> regionSaliency(regionCount.back(), 0);
	vector<int> regionOverlap(regionCount.back(), 0);
	Size imgSize = pyramidRegion[0].size();

	int count = 0;
	for (int pyramidIdx = 0; pyramidIdx < PYRAMID_SIZE; pyramidIdx++) {

		int *regionElementCount = new int[regionCount[pyramidIdx]];
		vector<Point> *regionElement = new vector<Point>[regionCount[pyramidIdx]];
		memset(regionElementCount, 0, sizeof(int)*regionCount[pyramidIdx]);
		getRegionElement(regionElement, regionElementCount, pyramidRegion[pyramidIdx]);

		for (int i = 0; i < regionCount[pyramidIdx]; i++) {

			vector<Point> regionBound;
			convexHull(regionElement[i], regionBound );

			Mat convexMap(imgSize, CV_8UC1, Scalar(0));
			fillConvexPoly(convexMap, regionBound, Scalar(255));
			getOverlap(regionOverlap, pyramidRegion[pyramidIdx], i, pyramidRegion.back(), convexMap);

			count++;
		}

		delete[] regionElement;
		delete[] regionElementCount;
	}

	int *regionElementCount = new int[regionCount.back()];
	vector<Point> *regionElement = new vector<Point>[regionCount.back()];
	memset(regionElementCount, 0, sizeof(int)*regionCount.back());
	getRegionElement(regionElement, regionElementCount, pyramidRegion.back());

	double max_saliency = 0;
	double min_saliency = INF;
	for (int i = 0; i < regionCount.back(); i++) {

		regionSaliency[i] = (double)regionOverlap[i] / (count * regionElementCount[i]);
		max_saliency = max(max_saliency, regionSaliency[i]);
		min_saliency = min(min_saliency, regionSaliency[i]);
		//cout << regionSaliency[i] << " ";
	}
	//cout << endl;

	Point midP(imgSize.width/2, imgSize.height/2);
	for (int i = 0; i < regionCount.back(); i++) {

		regionSaliency[i] = (regionSaliency[i] - min_saliency) / (max_saliency - min_saliency);

		double centerBias;
		getCenterBias(centerBias, regionElement[i], midP);
		//cout << centerBias << " " << regionSaliency[i] << " ";
		regionSaliency[i] *= centerBias;
		//cout << regionSaliency[i] << endl;
	}

	max_saliency = 0;
	min_saliency = INF;
	for (int i = 0; i < regionCount.back(); i++) {
		max_saliency = max(max_saliency, regionSaliency[i]);
		min_saliency = min(min_saliency, regionSaliency[i]);
	}
	for (int i = 0; i < regionCount.back(); i++) {
		regionSaliency[i] = (regionSaliency[i] - min_saliency) / (max_saliency - min_saliency);
	}

	saliencyMap = Mat(imgSize, CV_8UC1, Scalar(0));
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pyramidRegion.back().ptr<int>(y)[x];
			saliencyMap.ptr<uchar>(y)[x] = regionSaliency[regionIdx] * 255;
		}
	}

#ifdef SHOW_IMAGE
	imshow("Saliency_Image", saliencyMap);
#endif

	delete[] regionElement;
	delete[] regionElementCount;

}


#endif // SALIENCY_H
