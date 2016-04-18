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

//void updateMixContrast(Mat &_saliencyMap, const Mat &pixelRegion, const int regionCount, const Mat &LABImg) {

//	vector<Vec3f> regionColor;
//	const double sigma_color = 1;
//	getRegionColor(regionColor, regionCount, pixelRegion, LABImg);

//	Mat regionDist;
//	const double sigma_width = 0.65;
//	getRegionDist(regionDist, pixelRegion, regionCount);

//	int *regionElementCount = new int[regionCount];
//	vector<Point> *regionElement = new vector<Point>[regionCount];
//	memset(regionElementCount, 0, sizeof(int)*regionCount);
//	getRegionElement(regionElement, regionElementCount, pixelRegion);

//	Size imgSize = pixelRegion.size();
//	Point midP(imgSize.width/2, imgSize.height/2);

//	vector<double> regionContrast(regionCount, 0);

//	for (int i = 0; i < regionCount; i++) {

//		for (int j = 0; j < regionCount; j++) {

//			if (j == i) continue;

//			double dist = exp(-regionDist.ptr<double>(i)[j] / sigma_width);
//			double color = colorDiff(regionColor[i], regionColor[j]) / sigma_color;
//			int size = regionElementCount[j];

//			regionContrast[i] += size * dist * color;

//			//cout << size << " " << dist << " " << color << " " << size * dist * color << endl;
//		}

//		double centerBias;
//		getCenterBias(centerBias, regionElement[i], midP);
//		regionContrast[i] *= centerBias;

//	}

//	delete[] regionElement;
//	delete[] regionElementCount;

//	Mat contrastMap(imgSize, CV_64FC1);
//	for (int y = 0; y < imgSize.height; y++) {
//		for (int x = 0; x < imgSize.width; x++) {
//			int regionIdx = pixelRegion.ptr<int>(y)[x];
//			contrastMap.ptr<double>(y)[x] = regionContrast[regionIdx];
//		}
//	}

//	_saliencyMap = _saliencyMap.mul(contrastMap);
//	normalize(_saliencyMap, _saliencyMap, 0, 255, CV_MINMAX);

//#ifdef SHOW_IMAGE
//	normalize(contrastMap, contrastMap, 0, 1, CV_MINMAX);
//	contrastMap.convertTo(contrastMap, CV_8UC1, 255);
//	imshow("contrast", contrastMap);
//	imwrite("debug_output/contrast.png", contrastMap);
//#endif


//}

//void updateborderMap(Mat &saliencyMap, Mat &borderMap, const Mat &pixelRegion, const int regionCount) {

//	if (!borderMap.empty()) {
//		saliencyMap.setTo(0, borderMap);
//		normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);
//		return;
//	}

//	Size imgSize = saliencyMap.size();
//	borderMap = Mat(imgSize, CV_8UC1, Scalar(255));
//	vector<double> regionVar_X(regionCount, 0);
//	vector<double> regionVar_Y(regionCount, 0);
//	vector<Point2i> regionMeanPos(regionCount, Point2i(0,0));
//	vector<int> regionSize(regionCount, 0);
//	vector<int> regionSaliency(regionCount, 0);

//	for (int y = 0; y < imgSize.height; y++) {
//		for (int x = 0; x < imgSize.width; x++) {

//			int regionIdx = pixelRegion.ptr<int>(y)[x];
//			regionMeanPos[regionIdx] += Point2i(x,y);
//			regionSize[regionIdx]++;
//			regionSaliency[regionIdx] += saliencyMap.ptr<uchar>(y)[x];
//		}
//	}

//	for (int i = 0; i < regionCount; i++) {
//		regionMeanPos[i].x /= regionSize[i];
//		regionMeanPos[i].y /= regionSize[i];
//		regionSaliency[i] /= regionSize[i];
//	}

//	for (int y = 0; y < imgSize.height; y++) {
//		for (int x = 0; x < imgSize.width; x++) {

//			int regionIdx = pixelRegion.ptr<int>(y)[x];
//			regionVar_X[regionIdx] += abs(x - regionMeanPos[regionIdx].x);
//			regionVar_Y[regionIdx] += abs(y - regionMeanPos[regionIdx].y);
//		}
//	}

//	for (int i = 0; i < regionCount; i++) {
//		regionVar_X[i] = regionVar_X[i] / regionSize[i] + FLOAT_EPS;
//		regionVar_Y[i] = regionVar_Y[i] / regionSize[i] + FLOAT_EPS;
//	}

//	vector<int> regionBorderNum_X(regionCount, 0);
//	vector<int> regionBorderNum_Y(regionCount, 0);
//	for (int y = 0; y < imgSize.height; y++) {
//		for (int x = 0; x < imgSize.width; x++) {

//			if (borderMap.ptr<uchar>(y)[x] != 255) continue;

//			int regionIdx = pixelRegion.ptr<int>(y)[x];
//		}
//	}

//	vector<bool> regionBorder(regionCount, false);
//	for (int i = 0; i < regionCount; i++) {
//		if (regionSaliency[i] > 180) continue;
//		double lk = regionBorderNum_X[i] / (4*CROP_WIDTH*regionVar_Y[i]) + regionBorderNum_Y[i] / (4*CROP_WIDTH*regionVar_X[i]);
//		regionBorder[i] = lk > 0.4 ? true : false;
//	}

//	for (int y = 0; y < imgSize.height; y++) {
//		for (int x = 0; x < imgSize.width; x++) {

//			int regionIdx = pixelRegion.ptr<int>(y)[x];
//			if (regionBorder[regionIdx]) borderMap.ptr<uchar>(y)[x] = 255;
//		}
//	}

//	saliencyMap.setTo(0, borderMap);
//	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);

//#ifdef SHOW_IMAGE
//	imshow("Border_Map", borderMap);
//	imwrite("debug_output/Border_Map.png", borderMap);
//#endif

//}

//void updateColorSmooth(Mat &saliencyMap, const Mat &LABImg) {



//	vector<int> colorCount(platte.size(), 0);
//	vector<int> colorSaliency(platte.size(), 0);
//	for (int y = 0; y < imgSize.height; y++) {
//		for (int x = 0; x < imgSize.width; x++) {

//			int colorIdx = colorMap.ptr<uchar>(y)[x];
//			colorCount[colorIdx]++;
//			colorSaliency[colorIdx] += saliencyMap.ptr<uchar>(y)[x];
//		}
//	}

//	for (size_t i = 0; i < platte.size(); i++) colorSaliency[i] /= colorCount[i];
//	//for (size_t i = 0; i < platte.size(); i++) cout << platte[i] << " " << colorSaliency[i] << endl;

//	int smooth_range = int(0.1 * platte.size());
//	vector<double> _colorSaliency(platte.size(), 0);
//	for (size_t i = 0; i < platte.size(); i++) {

//		double sigma_color = 1;
//		vector< pair<int, double> > similarColor;
//		for (size_t j = 0; j < platte.size(); j++) {
//			double diff = exp(-colorDiff(platte[i],platte[j]) / sigma_color);
//			//cout << colorDiff(platte[i],platte[j]) << " " << diff << endl;
//			//double diff = -colorDiff(platte[i],platte[j]) / 3;
//			similarColor.push_back(make_pair(j,diff));
//		}

//		sort(similarColor.begin(), similarColor.end(), cmpSimilarColor);

//		double sum_diff = 0;
//		//double sum_pts = 0;
//		for (int j = 0; j < smooth_range; j++) {
//			sum_diff += similarColor[j].second * colorCount[similarColor[j].first];
//			//sum_pts += colorCount[similarColor[j].first];
//			//sum_diff += similarColor[j].second;
//		}

//		_colorSaliency[i] = 0;
//		for (int j = 0; j < smooth_range; j++) {
//			int k = similarColor[j].first;
//			_colorSaliency[i] += similarColor[j].second * colorCount[k] * colorSaliency[k];
//			//_colorSaliency[i] += (similarColor[j].second - sum_diff) * colorCount[k] * colorSaliency[k];
//		}

//		_colorSaliency[i] /= sum_diff;
//		//_colorSaliency[i] /= -sum_diff * sum_pts;
//	}

//	normalizeVecd(_colorSaliency);

//	for (int y = 0; y < imgSize.height; y++) {
//		for (int x = 0; x < imgSize.width; x++) {
//			saliencyMap.ptr<uchar>(y)[x] = _colorSaliency[colorMap.ptr<uchar>(y)[x]] * 255;
//		}
//	}

//#ifdef SHOW_IMAGE
//	Mat quantizeImg(imgSize, CV_32FC3);
//	for (int y = 0; y < imgSize.height; y++) {
//		for (int x = 0; x < imgSize.width; x++) {
//			quantizeImg.ptr<Vec3f>(y)[x] = platte[colorMap.ptr<uchar>(y)[x]];
//		}
//	}
//	cvtColor(quantizeImg, quantizeImg, COLOR_Lab2BGR);
//	imshow("Quantize_Image.png", quantizeImg);
//	quantizeImg.convertTo(quantizeImg, CV_8UC3, 255);
//	imwrite("debug_output/Quantize_Image.png", quantizeImg);
//#endif
//}

//void updateRegionSmooth(Mat &saliencyMap, const Mat &pixelRegion, const int regionCount) {

//	vector<int> regionSaliency(regionCount, 0);
//	vector<int> regionSize(regionCount, 0);
//	Size imgSize = saliencyMap.size();

//	for (int y = 0; y < imgSize.height; y++) {
//		for (int x = 0; x < imgSize.width; x++) {
//			int regionIdx = pixelRegion.ptr<int>(y)[x];
//			regionSaliency[regionIdx] += saliencyMap.ptr<uchar>(y)[x];
//			regionSize[regionIdx]++;
//		}
//	}

//	for (int i = 0; i < regionCount; i++) {
//		regionSaliency[i] /= regionSize[i];
//	}

//	for (int y = 0; y < imgSize.height; y++) {
//		for (int x = 0; x < imgSize.width; x++) {

//			int regionIdx = pixelRegion.ptr<int>(y)[x];
//			saliencyMap.ptr<uchar>(y)[x] = regionSaliency[regionIdx];
//		}
//	}

//	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);

//}

void calcSaliencyMap(Mat &saliencyMap, vector<TypeRegionSet> &multiLayerModel, const Mat &LABImg) {

	Size imgSize = LABImg.size();
	Mat overlapAccumulate(imgSize, CV_32SC1, Scalar(0));

	for (size_t i = 0; i < multiLayerModel.size(); i++) {

		for (int regionId = 0; regionId < multiLayerModel[i].regionCount; regionId++) {

			vector<Point> regionBound;
			convexHull(multiLayerModel[i].regions[regionId].pts, regionBound );

			Mat convexMap(imgSize, CV_32SC1, Scalar(0));
			fillConvexPoly(convexMap, regionBound, Scalar(1));

			for (int j = 0; j < multiLayerModel[i].regions[regionId].ptsCount; j++) {
				convexMap.at<int>(multiLayerModel[i].regions[regionId].pts[j]) = 0;
			}

			overlapAccumulate += convexMap;

		}
	}

	for (size_t i = 0; i < multiLayerModel.size(); i++) {

		Mat CHOMap(imgSize, CV_64FC1, Scalar(0));

		for (int regionId = 0; regionId < multiLayerModel[i].regionCount; regionId++) {

			int overlapCount = 0;
			for (int j = 0; j < multiLayerModel[i].regions[regionId].ptsCount; j++) {
				overlapCount += overlapAccumulate.at<int>(multiLayerModel[i].regions[regionId].pts[j]);
			}

			multiLayerModel[i].CHO[regionId] = overlapCount / multiLayerModel[i].regions[regionId].ptsCount;

		}

		normalizeVecd(multiLayerModel[i].CHO);

		for (int regionId = 0; regionId < multiLayerModel[i].regionCount; regionId++) {
			for (int j = 0; j < multiLayerModel[i].regions[regionId].ptsCount; j++) {
				CHOMap.at<double>(multiLayerModel[i].regions[regionId].pts[j]) = multiLayerModel[i].CHO[regionId];
			}
		}

		imshow("CHO Map", CHOMap);
		waitKey();

	}

//	Mat borderMap;
//	updateborderMap(saliencyMap, borderMap, over_pixelRegion, over_regionCount);
//#ifdef SHOW_IMAGE
//	imshow("debug_output/S_border1", saliencyMap);
//#endif

//	updateColorSmooth(saliencyMap, LABImg);
//#ifdef SHOW_IMAGE
//	imshow("debug_output/S_color", saliencyMap);
//#endif

//	saliencyMap = 0.5 * saliencyMap + 0.5 * saliencyMap_base;
//	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);

//	updateRegionSmooth(saliencyMap, over_pixelRegion, over_regionCount);
//#ifdef SHOW_IMAGE
//	imshow("debug_output/S_space", saliencyMap);
//#endif

//	updateborderMap(saliencyMap, borderMap, over_pixelRegion, over_regionCount);
//#ifdef SHOW_IMAGE
//	imshow("debug_output/S_border2", saliencyMap);
//#endif

//	GaussianBlur(saliencyMap, saliencyMap, Size(3,3), 0);
//	normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);
//#ifdef SHOW_IMAGE
//	imwrite("debug_output/Saliency_Map.png", saliencyMap);
//#endif

}
