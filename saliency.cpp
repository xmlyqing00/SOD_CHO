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

double calcCenterW(const Point &regionCenter, const Size &imgSize) {

	Point imgCenter(imgSize.width >> 1, imgSize.height >> 1);
	Point2d bias = regionCenter - imgCenter;
	bias.x /= imgSize.width;
	bias.y /= imgSize.height;
	return exp(-9.0 * (sqr(bias.x) + sqr(bias.y)));

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

void calcCHOMap(vector<TypeRegionSet> &multiLayerModel, const Size &imgSize) {

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

		for (int regionId = 0; regionId < multiLayerModel[i].regionCount; regionId++) {

			int overlapCount = 0;
			for (int j = 0; j < multiLayerModel[i].regions[regionId].ptsCount; j++) {
				overlapCount += overlapAccumulate.at<int>(multiLayerModel[i].regions[regionId].pts[j]);
			}

			multiLayerModel[i].CHO[regionId] = overlapCount / multiLayerModel[i].regions[regionId].ptsCount;

		}

		for (int regionId = 0; regionId < multiLayerModel[i].regionCount; regionId++) {
			for (int j = 0; j < multiLayerModel[i].regions[regionId].ptsCount; j++) {
				multiLayerModel[i].CHOMap.at<float>(multiLayerModel[i].regions[regionId].pts[j]) = multiLayerModel[i].CHO[regionId];
			}
		}

	}
}

void calcContrastMap(vector<TypeRegionSet> &multiLayerModel, const Size &imgSize) {

	for (size_t i = 0; i < multiLayerModel.size(); i++) {

		for (int regionId0 = 0; regionId0 < multiLayerModel[i].regionCount; regionId0++) {

			for (int regionId1 = 0; regionId1 < multiLayerModel[i].regionCount; regionId1++) {

				if (regionId0 == regionId1) continue;

				double dist = calcRegionsDist(multiLayerModel[i].regions[regionId0], multiLayerModel[i].regions[regionId1]);
				double colorDiff = 1 - calcRegionsColorDiff(multiLayerModel[i].regions[regionId0], multiLayerModel[i].regions[regionId1]);

				multiLayerModel[i].contrast[regionId0] += dist * colorDiff * multiLayerModel[i].regions[regionId1].ptsCount;

			}

			//multiLayerModel[i].contrast[regionId0] *= multiLayerModel[i].regions[regionId0].ptsCount;
			multiLayerModel[i].contrast[regionId0] *= calcCenterW(multiLayerModel[i].regions[regionId0].centerPos, imgSize);

		}

	}

	for (size_t i = 0; i < multiLayerModel.size(); i++) {

		for (int regionId = 0; regionId < multiLayerModel[i].regionCount; regionId++) {

			for (int j = 0; j < multiLayerModel[i].regions[regionId].ptsCount; j++) {

				multiLayerModel[i].contrastMap.at<float>(multiLayerModel[i].regions[regionId].pts[j]) = multiLayerModel[i].contrast[regionId];
			}
		}

	}
}

void calcSaliencyMap(Mat &saliencyMap, Mat &CHOMap, Mat &contrastMap, const vector<TypeRegionSet> &multiLayerModel) {

	for (size_t i = 0; i < multiLayerModel.size(); i++) {

		double w = pow(1, multiLayerModel.size()-i-1);
		CHOMap += w * multiLayerModel[i].CHOMap;
		contrastMap += w * multiLayerModel[i].contrastMap;

		multiLayerModel[i].writeCHOMap();
		multiLayerModel[i].writeContrastMap();
	}

	normalize(CHOMap, CHOMap, 0, 1, CV_MINMAX);
	normalize(contrastMap, contrastMap, 0, 1, CV_MINMAX);

	//saliencyMap = CHOMap.mul(contrastMap);
	saliencyMap = CHOMap + contrastMap;

	normalize(saliencyMap, saliencyMap, 0, 1, CV_MINMAX);

	imwrite("CHO_Map.png", CHOMap);
	imwrite("Contrast_Map.png", contrastMap);
	imwrite("Saliency_Map.png", saliencyMap);

}

void calcBorderMap(Mat &borderMap, const vector<TypeRegionSet> &multiLayerModel) {

	Size imgSize = borderMap.size();
	int layerId = 1;

	Mat tmpMap = borderMap(Rect(BORDER_WIDTH, BORDER_WIDTH, imgSize.width-2*BORDER_WIDTH, imgSize.height-2*BORDER_WIDTH));
	tmpMap.setTo(0);

	for (int i = 0; i < multiLayerModel[layerId].regionCount; i++) {

		int borderCount = 0;
		for (int j = 0; j < multiLayerModel[layerId].regions[i].ptsCount; j++) {
			if (borderMap.at<uchar>(multiLayerModel[layerId].regions[i].pts[j]) == 255) borderCount++;
		}

		if (borderCount < multiLayerModel[layerId].regions[i].ptsCount * 0.2) continue;

		for (int j = 0; j < multiLayerModel[layerId].regions[i].ptsCount; j++) {
			borderMap.at<uchar>(multiLayerModel[layerId].regions[i].pts[j]) = 255;
		}

	}

	imwrite("Border_Map.png", borderMap);

}

void updateColorSmooth(Mat &saliencyMap, const Mat &paletteMap) {

	Size imgSize = paletteMap.size();
	int paletteSize = paletteDist.rows;
	vector<int> colorCount(paletteSize, 0);
	vector<float> colorSaliency(paletteSize, 0);

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			int colorIdx = paletteMap.ptr<uchar>(y)[x];
			colorCount[colorIdx]++;
			colorSaliency[colorIdx] += saliencyMap.ptr<float>(y)[x];
		}
	}

	for (int i = 0; i < paletteSize; i++) colorSaliency[i] /= colorCount[i];
	//for (size_t i = 0; i < paletteSize; i++) cout << palette[i] << " " << colorSaliency[i] << endl;

	int smooth_range = int(0.1 * paletteSize);
	vector<float> _colorSaliency(paletteSize, 0);
	for (int i = 0; i < paletteSize; i++) {

		vector< pair<int, double> > similarColor;
		for (int j = 0; j < paletteSize; j++) {
			double diff = exp(-paletteDist.ptr<float>(i)[j] / SIGMA_COLOR);
			//double diff = -colorDiff(palette[i],palette[j]) / 3;
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

	normalizeVecf(_colorSaliency);

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			saliencyMap.ptr<float>(y)[x] = _colorSaliency[paletteMap.ptr<uchar>(y)[x]];
		}
	}
}

void updateRegionSmooth(Mat &saliencyMap, const vector<TypeRegionSet> &multiLayerModel) {

	int layerId = 1;

	for (int i = 0; i < multiLayerModel[layerId].regionCount; i++) {

		double accumulateSaliency = 0;

		for (int j = 0; j < multiLayerModel[layerId].regions[i].ptsCount; j++) {
			accumulateSaliency += saliencyMap.at<float>(multiLayerModel[layerId].regions[i].pts[j]);
		}

		accumulateSaliency /= multiLayerModel[layerId].regions[i].ptsCount;

		for (int j = 0; j < multiLayerModel[layerId].regions[i].ptsCount; j++) {
			saliencyMap.at<float>(multiLayerModel[layerId].regions[i].pts[j]) = accumulateSaliency;
		}

	}

	normalize(saliencyMap, saliencyMap, 0, 1, CV_MINMAX);

}

void getSaliencyMap(Mat &saliencyMap, vector<TypeRegionSet> &multiLayerModel, const Mat &paletteMap) {

	Size imgSize = paletteMap.size();

	calcCHOMap(multiLayerModel, imgSize);
	calcContrastMap(multiLayerModel, imgSize);

	Mat CHOMap(imgSize, CV_32FC1, Scalar(0));
	Mat contrastMap(imgSize, CV_32FC1, Scalar(0));
	calcSaliencyMap(saliencyMap, CHOMap, contrastMap, multiLayerModel);
	//imshow("Saliency_Map.png", saliencyMap);

	Mat borderMap(imgSize, CV_8UC1, Scalar(255));
	calcBorderMap(borderMap, multiLayerModel);
	saliencyMap.setTo(0, borderMap);
	//imshow("Border_Map.png", saliencyMap);

	updateColorSmooth(saliencyMap, paletteMap);
	//imshow("Color_Smooth.png", saliencyMap);

	updateRegionSmooth(saliencyMap, multiLayerModel);
	//imshow("Region_Smooth.png", saliencyMap);

	saliencyMap.setTo(0, borderMap);
	//imshow("Border_Map.png", saliencyMap);

	//waitKey(0);

}
