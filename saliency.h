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

void normalizeVecd(vector<double> &vec) {

	double max_data = 0;
	double min_data = INF;

	for (size_t i = 0; i < vec.size(); i++) {

		max_data = max(max_data, vec[i]);
		min_data = min(min_data, vec[i]);
	}

	for (size_t i = 0; i < vec.size(); i++) {
		vec[i] = (vec[i] - min_data) / (max_data - min_data);
	}
}

void getSaliencyMap(Mat &saliencyMap, const vector<int> &regionCount,
					const vector<Mat> &pyramidRegion, const Mat &LABImg) {

	int baseRegionCount = regionCount.back();
	vector<double> regionSaliency(baseRegionCount, 0);
	vector<int> regionOverlap(baseRegionCount, 0);
	Size imgSize = pyramidRegion[0].size();

	int overlapCount = 0;
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

			overlapCount++;
		}

		delete[] regionElement;
		delete[] regionElementCount;
	}

	int *regionElementCount = new int[baseRegionCount];
	vector<Point> *regionElement = new vector<Point>[baseRegionCount];
	memset(regionElementCount, 0, sizeof(int)*baseRegionCount);
	getRegionElement(regionElement, regionElementCount, pyramidRegion.back());

	// update with overlap
	for (int i = 0; i < baseRegionCount; i++) {
		regionSaliency[i] = (double)regionOverlap[i] / (overlapCount * regionElementCount[i]);
	}

	normalizeVecd(regionSaliency);

	// update with same region
	for (int pyramidIdx = PYRAMID_SIZE - 1; pyramidIdx >= 0; pyramidIdx--) {

		Mat regionComponent(regionCount[pyramidIdx], baseRegionCount, CV_8UC1, Scalar(0));
		for (int y = 0; y < imgSize.height; y++) {
			for (int x = 0; x < imgSize.width; x++) {

				int baseRegionIdx = pyramidRegion[PYRAMID_SIZE-1].ptr<int>(y)[x];
				int regionIdx = pyramidRegion[pyramidIdx].ptr<int>(y)[x];
				regionComponent.ptr<uchar>(regionIdx)[baseRegionIdx] = 1;
			}
		}

		double smooth_ratio = 0.5 + 0.5 * (double)(PYRAMID_SIZE - pyramidIdx - 1) / PYRAMID_SIZE;

		for (int i = 0; i < regionCount[pyramidIdx]; i++) {

			double meanSaliency = 0;
			int componentCount = 0;
			for (int j = 0; j < baseRegionCount; j++) {

				if (regionComponent.ptr<uchar>(i)[j] == 0) continue;
				meanSaliency += regionSaliency[j];
				componentCount++;
			}

			meanSaliency /= componentCount;

			for (int j = 0; j < baseRegionCount; j++) {

				if (regionComponent.ptr<uchar>(i)[j] == 0) continue;
				regionSaliency[j] = meanSaliency + smooth_ratio * (regionSaliency[j] - meanSaliency);
			}
		}
	}

	normalizeVecd(regionSaliency);

	// update saliency with region contrast
	vector<Vec3b> regionColor;
	const double sigma_color = 128.0;
	getRegionColor(regionColor, baseRegionCount, pyramidRegion.back(), LABImg);

	Mat regionDist;
	const double sigma_width = 0.4;
	getRegionDist(regionDist, pyramidRegion.back(), baseRegionCount);

	Point midP(imgSize.width/2, imgSize.height/2);

	vector<double> colorContrast(baseRegionCount, 0);
	for (int i = 0; i < baseRegionCount; i++) {

		for (int j = 0; j < baseRegionCount; j++) {

			if (j == i) continue;

			double dist = exp(-regionDist.ptr<double>(i)[j] / sigma_width);
			double color = colorDiff(regionColor[i], regionColor[j]) / sigma_color;
			int size = regionElementCount[j];

			colorContrast[i] += size * dist * color;

			//cout << size << " " << dist << " " << color << " " << size * dist * color << endl;
		}

		double centerBias;
		getCenterBias(centerBias, regionElement[i], midP);
		colorContrast[i] *= centerBias;
	}

	normalizeVecd(colorContrast);

	for (int i = 0; i < baseRegionCount; i++) {
		//cout << colorContrast[i] << " " << regionSaliency[i] << endl;
		regionSaliency[i] *= colorContrast[i];
	}

	normalizeVecd(regionSaliency);

	saliencyMap = Mat(imgSize, CV_8UC1, Scalar(0));
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pyramidRegion.back().ptr<int>(y)[x];
			saliencyMap.ptr<uchar>(y)[x] = regionSaliency[regionIdx] * 255;
		}
	}

	delete[] regionElement;
	delete[] regionElementCount;

}


#endif // SALIENCY_H
