#ifndef PYRAMID_H
#define PYRAMID_H

#include "comman.h"
#include "ncut.h"

void recursivePyramidRegion(vector< vector<int> > &pyramidRegionTag, const vector<int> &WtoRegion,
							const Mat &W, const int depth, const int regionIdx) {

	if (W.cols == 0) return;

	vector<int> regionTag;
	getNormalizedCut(regionTag, W);

	int n = W.rows;
	int regionCountPart1 = 0, regionCountPart2 = 0;
	vector<int> regionMap(n);
	vector<int> WtoRegion1, WtoRegion2;

	for (int i = 0; i < n; i++) {
		if (regionTag[i] == 0) {
			regionMap[i] = regionCountPart1++;
			pyramidRegionTag[depth][WtoRegion[i]] = regionIdx << 1;
			WtoRegion1.push_back(WtoRegion[i]);
		} else {
			regionMap[i] = regionCountPart2++;
			pyramidRegionTag[depth][WtoRegion[i]] = (regionIdx << 1) + 1;
			WtoRegion2.push_back(WtoRegion[i]);
		}
	}

	if (depth == PYRAMID_SIZE - 1) return;

	Mat WPart1(regionCountPart1, regionCountPart1, CV_64FC1);
	Mat WPart2(regionCountPart2, regionCountPart2, CV_64FC1);

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {

			if (regionTag[i] != regionTag[j]) continue;
			if (regionTag[i] == 0) {
				WPart1.ptr<double>(regionMap[i])[regionMap[j]] = W.ptr<double>(i)[j];
			} else {
				WPart2.ptr<double>(regionMap[i])[regionMap[j]] = W.ptr<double>(i)[j];
			}
		}
	}

	recursivePyramidRegion(pyramidRegionTag, WtoRegion1, WPart1, depth+1, regionIdx << 1);
	recursivePyramidRegion(pyramidRegionTag, WtoRegion2, WPart2, depth+1, (regionIdx << 1) + 1);

}

void buildPyramidRegion(vector<Mat> &pyramidRegion, vector<int> &_regionCount,
						const Mat &pixelRegion, const Mat &W) {

	vector< vector<int> > pyramidRegionTag(PYRAMID_SIZE, vector<int>(W.cols, 0));
	vector<int> WtoRegion(W.cols);
	for (size_t i = 0; i < WtoRegion.size(); i++) WtoRegion[i] = i;

	recursivePyramidRegion(pyramidRegionTag, WtoRegion, W, 0, 0);

	Size imgSize = pixelRegion.size();

	for (int pyramidIdx = 0; pyramidIdx < PYRAMID_SIZE; pyramidIdx++) {

		vector<int> regionTag(pow(2,pyramidIdx+1), 0);
		for (size_t i = 0; i < pyramidRegionTag[pyramidIdx].size(); i++) {
			regionTag[pyramidRegionTag[pyramidIdx][i]] = 1;
		}

		int regionCount = 0;
		for (size_t i = 0; i < regionTag.size(); i++) {
			if (regionTag[i] == 1) regionTag[i] = regionCount++;
		}

		for (size_t i = 0; i < pyramidRegionTag[pyramidIdx].size(); i++) {
			pyramidRegionTag[pyramidIdx][i] = regionTag[pyramidRegionTag[pyramidIdx][i]];
		}

		Mat newPixelRegion(imgSize, CV_32SC1);
		for (int y = 0; y < imgSize.height; y++) {
			for (int x = 0; x < imgSize.width; x++) {
				int regionIdx = pixelRegion.ptr<int>(y)[x];
				newPixelRegion.ptr<int>(y)[x] = pyramidRegionTag[pyramidIdx][regionIdx];
			}
		}

		pyramidRegion.push_back(newPixelRegion);

		_regionCount.push_back(regionCount);

#ifdef SHOW_IMAGE
		char fileName[100];
		sprintf(fileName, "Pyramid_%d.png", pyramidIdx);
		writeRegionImageRandom(regionCount, newPixelRegion, fileName, 0, 1);
#endif
	}
}

#endif // PYRAMID_H

