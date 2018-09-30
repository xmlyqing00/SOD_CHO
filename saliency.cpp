#include "saliency.h"

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

void getCHODetail(Mat &CHODetailMap, const int &objIdx, const Mat &objPixelRegion, const int &bgIdx, const Mat &bgPixelRegion,
				  const vector<Point> &regionBound, const Mat &LABImg) {

	Size imgSize = LABImg.size();
	Mat colorImg;
	cvtColor(LABImg, colorImg, COLOR_Lab2BGR);
	colorImg.convertTo(colorImg, CV_8UC3, 255);
	CHODetailMap = Mat(imgSize, CV_8UC3);

	initTransparentImage(CHODetailMap);

	Vec3i objColor(0, 0, 0), bgColor(0, 0, 0);
	int objCount = 0, bgCount = 0;
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			if (objPixelRegion.ptr<int>(y)[x] == objIdx) {
				objColor += colorImg.ptr<Vec3b>(y)[x];
				objCount++;
			}
			if (bgPixelRegion.ptr<int>(y)[x] == bgIdx) {
				bgColor += colorImg.ptr<Vec3b>(y)[x];
				bgCount++;
			}
		}
	}

	for (int k = 0; k < 3; k++) {
		if (objCount > 0) objColor.val[k] /= objCount;
		if (objCount > 0) bgColor.val[k] /= bgCount;
	}

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			if (objPixelRegion.ptr<int>(y)[x] == objIdx) {
				CHODetailMap.ptr<Vec3b>(y)[x] = objColor;
			}
			if (bgPixelRegion.ptr<int>(y)[x] == bgIdx) {
				CHODetailMap.ptr<Vec3b>(y)[x] = bgColor;
			}
		}
	}

	for (size_t i = 1; i < regionBound.size(); i++) {
		line(CHODetailMap, regionBound[i-1], regionBound[i], Scalar(0, 0, 255), 3);
	}
	line(CHODetailMap, regionBound.back(), regionBound[0], Scalar(0, 0, 255), 3);

}

void getCHOSaliencyMap(Mat &saliencyMap, const vector<int> &regionCount, const vector<Mat> &pyramidRegion, const Mat &LABImg) {

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

#ifdef SHOW_IMAGE
//			Mat CHODetailMap;
//			getCHODetail(CHODetailMap, 6, pyramidRegion.back(), i, pyramidRegion[pyramidIdx], regionBound, LABImg);
//			char CHODetailMapName[100];
//			sprintf(CHODetailMapName, "CHO_Details/pyramid%d_bg%d.png", pyramidIdx, i);
//			imwrite(CHODetailMapName, CHODetailMap);
//			imshow(CHODetailMapName, CHODetailMap);
#endif

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

	saliencyMap = Mat(imgSize, CV_64FC1);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pyramidRegion.back().ptr<int>(y)[x];
			saliencyMap.ptr<double>(y)[x] = regionSaliency[regionIdx];
		}
	}
}

void updateMixContrast(Mat &_saliencyMap, const Mat &pixelRegion, const int regionCount, const Mat &LABImg) {

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

	Mat contrastMap(imgSize, CV_64FC1);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			contrastMap.ptr<double>(y)[x] = regionContrast[regionIdx];
		}
	}

	_saliencyMap = _saliencyMap.mul(contrastMap);
	normalize(_saliencyMap, _saliencyMap, 0, 255, CV_MINMAX);

#ifdef SHOW_IMAGE
	normalize(contrastMap, contrastMap, 0, 1, CV_MINMAX);
	contrastMap.convertTo(contrastMap, CV_8UC1, 255);
	imshow("contrast", contrastMap);
	imwrite("debug_output/contrast.png", contrastMap);
#endif


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
	imwrite("debug_output/Border_Map.png", borderMap);
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
	quantizeImg.convertTo(quantizeImg, CV_8UC3, 255);
	imwrite("debug_output/Quantize_Image.png", quantizeImg);
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

	Mat _saliencyMap;
	getCHOSaliencyMap(_saliencyMap, regionCount, pyramidRegion, LABImg);

#ifdef SHOW_IMAGE
	Mat tmp_saliencyMap;
	normalize(_saliencyMap, tmp_saliencyMap, 0, 1, CV_MINMAX);
	tmp_saliencyMap.convertTo(tmp_saliencyMap, CV_8UC1, 255);
	imshow("CHO", tmp_saliencyMap);
	imwrite("debug_output/CHO.png", tmp_saliencyMap);
#endif

	updateMixContrast(_saliencyMap, over_pixelRegion, over_regionCount, LABImg);

	_saliencyMap.convertTo(saliencyMap, CV_8UC1);

	Mat saliencyMap_base = saliencyMap.clone();
#ifdef SHOW_IMAGE
	imshow("MIX", saliencyMap);
	imwrite("debug_output/MIX.png", saliencyMap);
#endif

	Mat borderMap;
	updateborderMap(saliencyMap, borderMap, over_pixelRegion, over_regionCount);
#ifdef SHOW_IMAGE
	imshow("debug_output/S_border1", saliencyMap);
#endif

	updateColorSmooth(saliencyMap, LABImg);
#ifdef SHOW_IMAGE
	imshow("debug_output/S_color", saliencyMap);
#endif

//	Mat tmpMap, tmpMap1;
//	saliencyMap.convertTo(tmpMap, CV_32SC1);
//	saliencyMap_base.convertTo(tmpMap1, CV_32SC1);
//	tmpMap = tmpMap.mul(tmpMap1);
//	normalize(tmpMap, tmpMap, 0, 255, CV_MINMAX);
//	tmpMap.convertTo(saliencyMap, CV_8UC1);
	saliencyMap = 0.5 * saliencyMap + 0.5 * saliencyMap_base;
	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);

	updateRegionSmooth(saliencyMap, over_pixelRegion, over_regionCount);
#ifdef SHOW_IMAGE
	imshow("debug_output/S_space", saliencyMap);
#endif

	updateborderMap(saliencyMap, borderMap, over_pixelRegion, over_regionCount);
#ifdef SHOW_IMAGE
	imshow("debug_output/S_border2", saliencyMap);
#endif

	GaussianBlur(saliencyMap, saliencyMap, Size(3,3), 0);
	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);
#ifdef SHOW_IMAGE
	imwrite("debug_output/Saliency_Map.png", saliencyMap);
#endif

}