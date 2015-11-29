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

void updateMixContrast(Mat &saliencyMap, const Mat &pixelRegion, const int regionCount, const Mat &LABImg) {

	vector<Vec3b> regionColor;
	const double sigma_color = 1;
	getRegionColor(regionColor, regionCount, pixelRegion, LABImg);

	Mat regionDist;
	const double sigma_width = 0.4;
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

	Mat _saliencyMap(imgSize, CV_64FC1, Scalar(0));
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			_saliencyMap.ptr<double>(y)[x] = (double)saliencyMap.ptr<uchar>(y)[x] / 255.0 * regionContrast[regionIdx];
		}
	}

	normalize(_saliencyMap, _saliencyMap, 0, 255, CV_MINMAX);

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			saliencyMap.ptr<uchar>(y)[x] = _saliencyMap.ptr<double>(y)[x];
		}
	}

}

void updateRegionContrast(Mat &saliencyMap, const Mat &pixelRegion, const int regionCount, const Mat &LABImg) {

	vector<Vec3b> regionColor;
	const double sigma_color = 1;
	getRegionColor(regionColor, regionCount, pixelRegion, LABImg);

	Mat regionDist;
	const double sigma_width = 0.4;
	getRegionDist(regionDist, pixelRegion, regionCount);

	int *regionElementCount = new int[regionCount];
	vector<Point> *regionElement = new vector<Point>[regionCount];
	memset(regionElementCount, 0, sizeof(int)*regionCount);
	getRegionElement(regionElement, regionElementCount, pixelRegion);

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

	//normalizeVecd(colorContrast);

	Size imgSize = saliencyMap.size();
	Mat _saliencyMap(imgSize, CV_32SC1);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			_saliencyMap.ptr<int>(y)[x] = (int)saliencyMap.ptr<uchar>(y)[x] * colorContrast[regionIdx];
		}
	}

	normalize(_saliencyMap, _saliencyMap, 0, 255, CV_MINMAX);

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			saliencyMap.ptr<uchar>(y)[x] = _saliencyMap.ptr<int>(y)[x];
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

void quantizeColorSpace(Mat &colorMap, vector<Vec3b> &platte, const Mat &LABImg) {

	Size imgSize = LABImg.size();
	vector<TypeColorSpace> colorSet;
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			colorSet.push_back(TypeColorSpace(Point(x,y), LABImg.ptr<Vec3b>(y)[x]));
		}
	}

	vector< vector<TypeColorSpace> > medianCutQue;
	medianCutQue.push_back(colorSet);

	for (int level = 0; level < QUANTIZE_LEVEL; level++) {

		vector< vector<TypeColorSpace> > tmpQue;

		for (size_t i = 0; i < medianCutQue.size(); i++) {

			Vec3b minColor(255, 255, 255);
			Vec3b maxColor(0, 0, 0);
			for (size_t j = 0; j < medianCutQue[i].size(); j++) {
				for (int k = 0; k < 3; k++) {
					minColor.val[k] = min(minColor.val[k], medianCutQue[i][j].color[k]);
					maxColor.val[k] = max(maxColor.val[k], medianCutQue[i][j].color[k]);
				}
			}

			int cut_dimension = 0;
			int max_range = 0;
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

		Vec3b meanColor = Vec3b(medianCutQue[i][medianCutQue[i].size()>>1].color);
		platte.push_back(meanColor);
	}

	int range = int(0.1 * platte.size());
	for (size_t i = 0; i < medianCutQue.size(); i++) {

		for (size_t j = 0; j < medianCutQue[i].size(); j++) {

			Vec3b c = Vec3b(medianCutQue[i][j].color);

			size_t best_fit = i;
			int min_diff = INF;
			for (int k = 0; k < range; k++) {

				int tmpIdx = i + k;
				if (tmpIdx < 0 || tmpIdx >= (int)medianCutQue.size()) continue;
				int tmp = colorDiff(c, platte[tmpIdx]);
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
	vector<Vec3b> platte;
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

	//Mat tmp(platte);
	//cvtColor(tmp, tmp, COLOR_Lab2RGB);
	//cout << CV_8UC3 << tmp.type() << endl;
	//for (size_t i = 0; i < platte.size(); i++) platte[i] = tmp.ptr<Vec3b>(0)[i];

	int smooth_range = int(0.1 * platte.size());
	vector<int> _colorSaliency(platte.size(), 0);
	for (size_t i = 0; i < platte.size(); i++) {

		double sigma_color = 32;
		vector< pair<int, double> > similarColor;
		for (size_t j = 0; j < platte.size(); j++) {
			double diff = exp(-(double)colorDiff(platte[i],platte[j]) / sigma_color);
			//cout << colorDiff(platte[i],platte[j]) << " " << diff << endl;
			//double diff = -colorDiff(platte[i],platte[j]);
			similarColor.push_back(make_pair(colorSaliency[j],diff));
		}

		sort(similarColor.begin(), similarColor.end(), cmpSimilarColor);

		double sum_diff = 0;
		for (int j = 0; j < smooth_range; j++) sum_diff += similarColor[j].second;

		double new_saliency = 0;
		for (int j = 0; j < smooth_range; j++) {
			new_saliency += similarColor[j].second / sum_diff * similarColor[j].first;
		}

		_colorSaliency[i] = min(255, cvRound(new_saliency));
	}

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			saliencyMap.ptr<uchar>(y)[x] = _colorSaliency[colorMap.ptr<uchar>(y)[x]];
		}
	}

	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);

#ifdef SHOW_IMAGE
	Mat quantizeImg(imgSize, CV_8UC3);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			quantizeImg.ptr<Vec3b>(y)[x] = platte[colorMap.ptr<uchar>(y)[x]];
		}
	}
	cvtColor(quantizeImg, quantizeImg, COLOR_Lab2RGB);
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
					const Mat &over_pixelRegion, const int &over_regionCount, const Mat &LABImg) {

	vector<double> regionSaliency;
	getBaseSaliencyMap(regionSaliency, regionCount, pyramidRegion);

	writeSaliencyMap(saliencyMap, regionSaliency, pyramidRegion.back(), LABImg.size(), "Saliency_Map_Base.png", 1);

#ifdef SHOW_IMAGE
	imshow("base", saliencyMap);
#endif

	//updateMixContrast(saliencyMap, over_pixelRegion, over_regionCount, LABImg);

	//updateRegionContrast(saliencyMap, over_pixelRegion, over_regionCount, LABImg);
#ifdef SHOW_IMAGE
	imshow("contrast", saliencyMap);
#endif
	updateCenterBias(saliencyMap, over_pixelRegion, over_regionCount);
#ifdef SHOW_IMAGE
	//imshow("center", saliencyMap);
#endif
	Mat borderMap;
	//updateborderMap(saliencyMap, borderMap, pyramidRegion.back(), regionCount.back());
	updateborderMap(saliencyMap, borderMap, over_pixelRegion, over_regionCount);
#ifdef SHOW_IMAGE
	imshow("S_border1", saliencyMap);
#endif
	updateColorSmooth(saliencyMap, LABImg);
#ifdef SHOW_IMAGE
	imshow("S_color", saliencyMap);
#endif
	updateRegionSmooth(saliencyMap, over_pixelRegion, over_regionCount);
#ifdef SHOW_IMAGE
	imshow("S_space", saliencyMap);
#endif
	updateborderMap(saliencyMap, borderMap, pyramidRegion.back(), regionCount.back());
#ifdef SHOW_IMAGE

	imshow("S_border2", saliencyMap);
	imwrite("Saliency_Map.png", saliencyMap);
#endif

}


#endif // SALIENCY_H
