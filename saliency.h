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

void getSaliencyMap(Mat &saliencyMap, const vector<int> &regionCount, const vector<Mat> &pyramidRegion) {

	vector<double> regionSaliency(regionCount.back(), 0);
	vector<int> regionOverlap(regionCount.back(), 0);
	Size imgSize = pyramidRegion[0].size();

	int count = 0;
	for (int pyramidIdx = 0; pyramidIdx < PYRAMID_SIZE; pyramidIdx++) {

		int *regionElementCount = new int[regionCount[pyramidIdx]];
		vector<Point> *regionElement = new vector<Point>[regionCount[pyramidIdx]];
		for (int i = 0; i < regionCount[pyramidIdx]; i++) {
			regionElementCount[i] = 0;
			regionElement[i].clear();
		}
		getRegionElement(regionElement, regionElementCount, pyramidRegion[pyramidIdx]);

		for (int i = 0; i < regionCount[pyramidIdx]; i++) {

			vector<Point> regionBound;
			convexHull(regionElement[i], regionBound );

			Mat convexMap(imgSize, CV_8UC1, Scalar(0));
			fillConvexPoly(convexMap, regionBound, Scalar(255));
			imshow("convexMap", convexMap);
			waitKey(1);
			getOverlap(regionOverlap, pyramidRegion[pyramidIdx], i, pyramidRegion.back(), convexMap);

			count++;
		}

		if (pyramidIdx == PYRAMID_SIZE - 1) {

			for (int i = 0; i < regionCount.back(); i++) {
				regionSaliency[i] = (double)regionOverlap[i] / (count * regionElementCount[i]);
				cout << regionSaliency[i] << " ";
			}
			cout << endl;
		}

		for (int i = 0; i < regionCount.back(); i++) {
			cout << regionOverlap[i] << " ";
		}
		cout << endl;

		delete[] regionElement;
		delete[] regionElementCount;
	}

	double max_saliency = 0;
	double min_saliency = INF;
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

	imshow("Saliency_Image", saliencyMap);

}


#endif // SALIENCY_H
