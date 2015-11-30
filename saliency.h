#ifndef SALIENCY_H
#define SALIENCY_H

#include "comman.h"
#include "type_que.h"

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

void writeSaliencyMap(Mat &saliencyMap, const vector<double> &regionSaliency, const Mat &pixelRegion,
					  const Size &imgSize, const char *fileName, const bool writeFlag) {

	if (saliencyMap.empty()) {
		saliencyMap = Mat(imgSize, CV_8UC1, Scalar(255));
	}
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			saliencyMap.ptr<uchar>(y)[x] *= regionSaliency[regionIdx];
		}
	}

	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);

	if (writeFlag) imwrite(fileName, saliencyMap);

}

void getBaseSaliencyMap(vector<double> &regionSaliency, const vector<int> &regionCount,
						const vector<Mat> &pyramidRegion) {

	int baseRegionCount = regionCount.back();
	regionSaliency = vector<double>(baseRegionCount, 0);
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

	delete[] regionElement;
	delete[] regionElementCount;

	normalizeVecd(regionSaliency);

	// update with same region
	vector<double> smoothedRegionSaliency(baseRegionCount, 0);

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
				smoothedRegionSaliency[j] = meanSaliency + smooth_ratio * (regionSaliency[j] - meanSaliency);
			}
		}
	}

	for (int i = 0; i < baseRegionCount; i++) {
		regionSaliency[i] = smoothedRegionSaliency[i] / PYRAMID_SIZE;
	}

	normalizeVecd(regionSaliency);
}

void updateMixContrast(Mat &contrastMap, const Mat &pixelRegion, const int regionCount, const Mat &LABImg) {

	vector<Vec3f> regionColor;
	const double sigma_color = 1;
	getRegionColor(regionColor, regionCount, pixelRegion, LABImg);

	Mat regionDist;
	const double sigma_width = 0.65;
	getRegionDist(regionDist, pixelRegion, regionCount);

	int *regionElementCount = new int[regionCount];
	vector<Point> *regionElement = new vector<Point>[regionCount];
	memset(regionElementCount, 0, sizeof(int)*regionCount);
	getRegionElement(regionElement, regionElementCount, pixelRegion);

	Size imgSize = pixelRegion.size();
	Point midP(imgSize.width/2, imgSize.height/2);

	vector<double> regionContrast(regionCount, 0);

	for (int i = 0; i < regionCount; i++) {

		for (int j = 0; j < regionCount; j++) {

			if (j == i) continue;

			double dist = exp(-regionDist.ptr<double>(i)[j] / sigma_width);
			double color = colorDiff(regionColor[i], regionColor[j]) / sigma_color;
			int size = regionElementCount[j];

			regionContrast[i] += size * dist * color;

			//cout << size << " " << dist << " " << color << " " << size * dist * color << endl;
		}

		double centerBias;
		getCenterBias(centerBias, regionElement[i], midP);
		regionContrast[i] *= centerBias;

	}

	delete[] regionElement;
	delete[] regionElementCount;

	normalizeVecd(regionContrast);

	contrastMap = Mat(imgSize, CV_8UC1);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			contrastMap.ptr<uchar>(y)[x] = regionContrast[regionIdx] * 255;
		}
	}

}

void updateRegionContrast(Mat &contrastMap, const Mat &pixelRegion, const int regionCount, const Mat &LABImg) {

	vector<Vec3f> regionColor;
	const double sigma_color = 1;
	getRegionColor(regionColor, regionCount, pixelRegion, LABImg);

	Mat regionDist;
	const double sigma_width = 0.65;
	getRegionDist(regionDist, pixelRegion, regionCount);

	int *regionElementCount = new int[regionCount];
	vector<Point> *regionElement = new vector<Point>[regionCount];
	memset(regionElementCount, 0, sizeof(int)*regionCount);
	getRegionElement(regionElement, regionElementCount, pixelRegion);

	Size imgSize = pixelRegion.size();
	vector<double> colorContrast(regionCount, 0);

	for (int i = 0; i < regionCount; i++) {

		for (int j = 0; j < regionCount; j++) {

			if (j == i) continue;

			double dist = exp(-regionDist.ptr<double>(i)[j] / sigma_width);
			double color = colorDiff(regionColor[i], regionColor[j]) / sigma_color;
			int size = regionElementCount[j];

			colorContrast[i] += size * dist * color;

			//cout << size << " " << dist << " " << color << " " << size * dist * color << endl;
		}
	}

	normalizeVecd(colorContrast);

	contrastMap = Mat(imgSize, CV_8UC1);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			contrastMap.ptr<uchar>(y)[x] = colorContrast[regionIdx] * 255;
		}
	}
}

void updateCenterBias(Mat &saliencyMap, const Mat &pixelRegion, const int regionCount) {

	int *regionElementCount = new int[regionCount];
	vector<Point> *regionElement = new vector<Point>[regionCount];
	memset(regionElementCount, 0, sizeof(int)*regionCount);
	getRegionElement(regionElement, regionElementCount, pixelRegion);

	Size imgSize = pixelRegion.size();
	Point midP(imgSize.width/2, imgSize.height/2);

	vector<double> centerBias(regionCount);

	for (int i = 0; i < regionCount; i++) {
		getCenterBias(centerBias[i], regionElement[i], midP);
	}

	normalizeVecd(centerBias);

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			saliencyMap.ptr<uchar>(y)[x] *= centerBias[regionIdx];
		}
	}

	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);

	delete[] regionElement;
	delete[] regionElementCount;
}

void updateborderMap(Mat &saliencyMap, Mat &borderMap, const Mat &pixelRegion, const int regionCount) {

	if (!borderMap.empty()) {
		saliencyMap.setTo(0, borderMap);
		normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);
		return;
	}

	Size imgSize = saliencyMap.size();
	borderMap = Mat(imgSize, CV_8UC1, Scalar(255));
	borderMap(Rect(BORDER_WIDTH, BORDER_WIDTH, imgSize.width-2*BORDER_WIDTH, imgSize.height-2*BORDER_WIDTH)).setTo(0);
	vector<int> regionSize(regionCount, 0);
	vector<int> regionBorderCount(regionCount, 0);

	TypeQue<Point> &que = *(new TypeQue<Point>);
	Mat visited(imgSize, CV_8UC1, Scalar(0));
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			if (borderMap.ptr<uchar>(y)[x] == 255) {
				regionBorderCount[regionIdx]++;
				que.push(Point(x,y));
				visited.ptr<uchar>(y)[x] = 1;
			}
			regionSize[regionIdx]++;
		}
	}

	vector<bool> borderRegion(regionCount, false);
	for (int i = 0; i < regionCount; i++) {
		if ((double)regionBorderCount[i] / regionSize[i] > BORDER_REGION) {
			borderRegion[i] = true;
		}
	}

	while (!que.empty()) {

		Point nowP = que.front();
		que.pop();

		for (int k = 0; k < PIXEL_CONNECT; k++) {

			Point newP = nowP + dxdy[k];
			if (isOutside(newP.x, newP.y, imgSize.width, imgSize.height)) continue;
			if (visited.ptr<uchar>(newP.y)[newP.x] == 1) continue;
			if (borderRegion[pixelRegion.ptr<int>(newP.y)[newP.x]]) {
				borderMap.ptr<uchar>(newP.y)[newP.x] = 255;
				visited.ptr<uchar>(newP.y)[newP.x] = 1;
				que.push(newP);
			}
		}
	}

	delete &que;

	saliencyMap.setTo(0, borderMap);

	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);
#ifdef SHOW_IMAGE
	imshow("Border_Map", borderMap);
	imwrite("Border_Map.png", borderMap);
#endif

}

void updateborderMap2(Mat &saliencyMap, Mat &borderMap, const Mat &pixelRegion, const int regionCount) {

	if (!borderMap.empty()) {
		saliencyMap.setTo(0, borderMap);
		normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);
		return;
	}

	Size imgSize = saliencyMap.size();
	borderMap = Mat(imgSize, CV_8UC1, Scalar(255));
	borderMap(Rect(BORDER_WIDTH, BORDER_WIDTH, imgSize.width-2*BORDER_WIDTH, imgSize.height-2*BORDER_WIDTH)).setTo(0);
	vector<double> regionVar_X(regionCount, 0);
	vector<double> regionVar_Y(regionCount, 0);
	vector<Point2i> regionMeanPos(regionCount, Point2i(0,0));
	vector<int> regionSize(regionCount, 0);
	vector<int> regionSaliency(regionCount, 0);

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			regionMeanPos[regionIdx] += Point2i(x,y);
			regionSize[regionIdx]++;
			regionSaliency[regionIdx] += saliencyMap.ptr<uchar>(y)[x];
		}
	}

	for (int i = 0; i < regionCount; i++) {
		regionMeanPos[i].x /= regionSize[i];
		regionMeanPos[i].y /= regionSize[i];
		regionSaliency[i] /= regionSize[i];
	}

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			regionVar_X[regionIdx] += abs(x - regionMeanPos[regionIdx].x);
			regionVar_Y[regionIdx] += abs(y - regionMeanPos[regionIdx].y);
		}
	}

	for (int i = 0; i < regionCount; i++) {
		regionVar_X[i] = regionVar_X[i] / regionSize[i] + FLOAT_EPS;
		regionVar_Y[i] = regionVar_Y[i] / regionSize[i] + FLOAT_EPS;
	}

	vector<int> regionBorderNum_X(regionCount, 0);
	vector<int> regionBorderNum_Y(regionCount, 0);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			if (borderMap.ptr<uchar>(y)[x] != 255) continue;

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			if (x < CROP_WIDTH || x >= imgSize.width-CROP_WIDTH) {
				regionBorderNum_X[regionIdx]++;
			}
			if (y < CROP_WIDTH || y >= imgSize.height-CROP_WIDTH) {
				regionBorderNum_Y[regionIdx]++;
			}
		}
	}

	vector<bool> regionBorder(regionCount, false);
	for (int i = 0; i < regionCount; i++) {
		if (regionSaliency[i] > 180) continue;
		double lk = regionBorderNum_X[i] / (4*CROP_WIDTH*regionVar_Y[i]) + regionBorderNum_Y[i] / (4*CROP_WIDTH*regionVar_X[i]);
		regionBorder[i] = lk > 0.4 ? true : false;
	}

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			if (regionBorder[regionIdx]) borderMap.ptr<uchar>(y)[x] = 255;
		}
	}

	saliencyMap.setTo(0, borderMap);
	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);

#ifdef SHOW_IMAGE
	imshow("Border_Map", borderMap);
	imwrite("Border_Map.png", borderMap);
#endif

}

void quantizeColorSpace(Mat &colorMap, vector<Vec3f> &platte, const Mat &colorImg) {

	Size imgSize = colorImg.size();
	vector<TypeColorSpace> colorSet;
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			colorSet.push_back(TypeColorSpace(Point(x,y), colorImg.ptr<Vec3f>(y)[x]));
		}
	}

	vector< vector<TypeColorSpace> > medianCutQue;
	medianCutQue.push_back(colorSet);

	for (int level = 0; level < QUANTIZE_LEVEL; level++) {

		vector< vector<TypeColorSpace> > tmpQue;

		for (size_t i = 0; i < medianCutQue.size(); i++) {

			Vec3f minColor(255, 255, 255);
			Vec3f maxColor(0, 0, 0);
			for (size_t j = 0; j < medianCutQue[i].size(); j++) {
				for (int k = 0; k < 3; k++) {
					minColor.val[k] = min(minColor.val[k], medianCutQue[i][j].color[k]);
					maxColor.val[k] = max(maxColor.val[k], medianCutQue[i][j].color[k]);
				}
			}

			int cut_dimension = 0;
			double max_range = 0;
			for (int k = 0; k < 3; k++) {
				if (maxColor.val[k] - minColor.val[k] > max_range) {
					max_range = maxColor.val[k] - minColor.val[k];
					cut_dimension = k;
				}
			}

			switch (cut_dimension) {
			case 0:
				sort(medianCutQue[i].begin(), medianCutQue[i].end(), cmpColor0);
				break;
			case 1:
				sort(medianCutQue[i].begin(), medianCutQue[i].end(), cmpColor1);
				break;
			case 2:
				sort(medianCutQue[i].begin(), medianCutQue[i].end(), cmpColor2);
				break;
			default:
				cout << "error in cut" << endl;
				exit(0);
			}

			int mid_pos = medianCutQue[i].size() / 2;
			vector<TypeColorSpace> part0(medianCutQue[i].begin(), medianCutQue[i].begin() + mid_pos);
			vector<TypeColorSpace> part1(medianCutQue[i].begin() + mid_pos, medianCutQue[i].end());

			tmpQue.push_back(part0);
			tmpQue.push_back(part1);
		}

		medianCutQue = tmpQue;
	}

	for (size_t i = 0; i < medianCutQue.size(); i++) {

		Vec3f meanColor = Vec3f(medianCutQue[i][medianCutQue[i].size()>>1].color);
		platte.push_back(meanColor);
	}

	int range = int(0.1 * platte.size());
	for (size_t i = 0; i < medianCutQue.size(); i++) {

		for (size_t j = 0; j < medianCutQue[i].size(); j++) {

			Vec3f c = Vec3f(medianCutQue[i][j].color);

			size_t best_fit = i;
			double min_diff = INF;
			for (int k = 0; k < range; k++) {

				int tmpIdx = i + k;
				if (tmpIdx < 0 || tmpIdx >= (int)medianCutQue.size()) continue;
				double tmp = colorDiff(c, platte[tmpIdx]);
				if (tmp < min_diff) {
					min_diff = tmp;
					best_fit = tmpIdx;
				}

				tmpIdx = i - k;
				if (tmpIdx < 0 || tmpIdx >= (int)medianCutQue.size()) continue;
				tmp = colorDiff(c, platte[tmpIdx]);
				if (tmp < min_diff) {
					min_diff = tmp;
					best_fit = tmpIdx;
				}
			}
			colorMap.at<uchar>(medianCutQue[i][j].pos) = best_fit;
		}
	}


}

void updateColorSmooth(Mat &saliencyMap, const Mat &LABImg) {

	Size imgSize = saliencyMap.size();
	Mat colorMap(imgSize, CV_8UC1);
	vector<Vec3f> platte;
	//Mat BGRImg;
	//cvtColor(LABImg, BGRImg, COLOR_Lab2BGR);
	quantizeColorSpace(colorMap, platte, LABImg);

	vector<int> colorCount(platte.size(), 0);
	vector<int> colorSaliency(platte.size(), 0);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			int colorIdx = colorMap.ptr<uchar>(y)[x];
			colorCount[colorIdx]++;
			colorSaliency[colorIdx] += saliencyMap.ptr<uchar>(y)[x];
		}
	}

	for (size_t i = 0; i < platte.size(); i++) colorSaliency[i] /= colorCount[i];
	//for (size_t i = 0; i < platte.size(); i++) cout << platte[i] << " " << colorSaliency[i] << endl;

	int smooth_range = int(0.1 * platte.size());
	vector<double> _colorSaliency(platte.size(), 0);
	for (size_t i = 0; i < platte.size(); i++) {

		double sigma_color = 1;
		vector< pair<int, double> > similarColor;
		for (size_t j = 0; j < platte.size(); j++) {
			double diff = exp(-colorDiff(platte[i],platte[j]) / sigma_color);
			//cout << colorDiff(platte[i],platte[j]) << " " << diff << endl;
			//double diff = -colorDiff(platte[i],platte[j]) / 3;
			similarColor.push_back(make_pair(j,diff));
		}

		sort(similarColor.begin(), similarColor.end(), cmpSimilarColor);

		double sum_diff = 0;
		//double sum_pts = 0;
		for (int j = 0; j < smooth_range; j++) {
			sum_diff += similarColor[j].second * colorCount[similarColor[j].first];
			//sum_pts += colorCount[similarColor[j].first];
			//sum_diff += similarColor[j].second;
		}

		_colorSaliency[i] = 0;
		for (int j = 0; j < smooth_range; j++) {
			int k = similarColor[j].first;
			_colorSaliency[i] += similarColor[j].second * colorCount[k] * colorSaliency[k];
			//_colorSaliency[i] += (similarColor[j].second - sum_diff) * colorCount[k] * colorSaliency[k];
		}

		_colorSaliency[i] /= sum_diff;
		//_colorSaliency[i] /= -sum_diff * sum_pts;
	}

	normalizeVecd(_colorSaliency);

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			saliencyMap.ptr<uchar>(y)[x] = _colorSaliency[colorMap.ptr<uchar>(y)[x]] * 255;
		}
	}

#ifdef SHOW_IMAGE
	Mat quantizeImg(imgSize, CV_32FC3);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			quantizeImg.ptr<Vec3f>(y)[x] = platte[colorMap.ptr<uchar>(y)[x]];
		}
	}
	cvtColor(quantizeImg, quantizeImg, COLOR_Lab2BGR);
	imshow("Quantize_Image.png", quantizeImg);
	imwrite("Quantize_Image.png", quantizeImg);
#endif
}

void updateRegionSmooth(Mat &saliencyMap, const Mat &pixelRegion, const int regionCount) {

	vector<int> regionSaliency(regionCount, 0);
	vector<int> regionSize(regionCount, 0);
	Size imgSize = saliencyMap.size();

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			regionSaliency[regionIdx] += saliencyMap.ptr<uchar>(y)[x];
			regionSize[regionIdx]++;
		}
	}

	for (int i = 0; i < regionCount; i++) {
		regionSaliency[i] /= regionSize[i];
	}

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			saliencyMap.ptr<uchar>(y)[x] = regionSaliency[regionIdx];
		}
	}

	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);

}

void getSaliencyMap(Mat &saliencyMap, const vector<int> &regionCount, const vector<Mat> &pyramidRegion,
					const Mat &over_pixelRegion, const int &over_regionCount, const Mat &LABImg, const double alpha) {

	vector<double> regionSaliency;
	getBaseSaliencyMap(regionSaliency, regionCount, pyramidRegion);

	writeSaliencyMap(saliencyMap, regionSaliency, pyramidRegion.back(), LABImg.size(), "Saliency_Map_Base.png", 1);

#ifdef SHOW_IMAGE
	imshow("base", saliencyMap);
#endif
	Mat contrastMap;
	updateMixContrast(contrastMap, over_pixelRegion, over_regionCount, LABImg);
	//updateRegionContrast(contrastMap, over_pixelRegion, over_regionCount, LABImg);
#ifdef SHOW_IMAGE
	imshow("contrast", contrastMap);
#endif
	saliencyMap = 0.5 * saliencyMap + 0.5 * contrastMap;
	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);
	Mat saliencyMap0 = saliencyMap.clone();
#ifdef SHOW_IMAGE
	imshow("combine", saliencyMap);
#endif
	//updateCenterBias(saliencyMap, over_pixelRegion, over_regionCount);
#ifdef SHOW_IMAGE
	imshow("center", saliencyMap);
#endif
	Mat borderMap;
	//updateborderMap(saliencyMap, borderMap, pyramidRegion.back(), regionCount.back());
	updateborderMap2(saliencyMap, borderMap, over_pixelRegion, over_regionCount);
#ifdef SHOW_IMAGE
	imshow("S_border1", saliencyMap);
#endif
	updateColorSmooth(saliencyMap, LABImg);
#ifdef SHOW_IMAGE
	imshow("S_color", saliencyMap);
#endif
	saliencyMap = 0.5 * saliencyMap + 0.5 * saliencyMap0;
	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);

	updateRegionSmooth(saliencyMap, over_pixelRegion, over_regionCount);
	//updateRegionSmooth(saliencyMap, pyramidRegion.back(), regionCount.back());
#ifdef SHOW_IMAGE
	imshow("S_space", saliencyMap);
#endif
	updateborderMap2(saliencyMap, borderMap, over_pixelRegion, over_regionCount);
#ifdef SHOW_IMAGE
	imshow("S_border2", saliencyMap);
#endif

#ifdef SHOW_IMAGE
	imwrite("Saliency_Map.png", saliencyMap);
#endif

}


#endif // SALIENCY_H
