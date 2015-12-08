#ifndef CUTOBJ
#define CUTOBJ

#include "comman.h"
#include "type_que.h"

void writeGCMask(const Mat &GCMask, const char *fileName, const int writeFlag, const int showFlag) {

	Mat resMap(GCMask.size(), CV_8UC3);
	for (int y = 0; y < GCMask.rows; y++) {
		for (int x = 0; x < GCMask.cols; x++) {
			switch (GCMask.ptr<uchar>(y)[x]) {
			case GC_FGD:
				resMap.ptr<Vec3b>(y)[x] = Vec3b(0,0,255);
				break;
			case GC_BGD:
				resMap.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
				break;
			case GC_PR_FGD:
				resMap.ptr<Vec3b>(y)[x] = Vec3b(0,0,100);
				break;
			case GC_PR_BGD:
				resMap.ptr<Vec3b>(y)[x] = Vec3b(100,0,0);
				break;
			}
		}
	}

	if (showFlag) imshow(fileName, resMap);
	if (writeFlag) imwrite(fileName, resMap);

}

void getMainRegionMask(Mat &regionMask, const Mat &saliencyMap) {

	Size imgSize = saliencyMap.size();
	TypeQue<Point> &que = *(new TypeQue<Point>);
	Mat pixelRegion(imgSize, CV_32SC1, Scalar(-1));
	int regionCount = 0;
	vector<long long> regionSaliency;

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			if (saliencyMap.ptr<uchar>(y)[x] == 0) continue;
			if (pixelRegion.ptr<int>(y)[x] > -1) continue;

			que.push(Point(x,y));
			pixelRegion.ptr<int>(y)[x] = regionCount;
			long long saliencyValue = 0;

			while (!que.empty()) {

				Point nowP = que.front();
				que.pop();
				saliencyValue += saliencyMap.at<uchar>(nowP);

				for (int k = 0; k < PIXEL_CONNECT; k++) {

					Point newP = nowP + dxdy[k];
					if (isOutside(newP.x, newP.y, imgSize.width, imgSize.height)) continue;
					if (saliencyMap.at<uchar>(newP) == 0) continue;
					if (pixelRegion.at<int>(newP) > -1) continue;

					pixelRegion.at<int>(newP) = regionCount;
					que.push(newP);
				}
			}

			regionCount++;
			regionSaliency.push_back(saliencyValue);

		}
	}

	delete &que;

	long long max_saliency = 0;
	int regionIdx = 0;
	for (size_t i = 0; i < regionSaliency.size(); i++) {
		if (max_saliency < regionSaliency[i]) {
			max_saliency = regionSaliency[i];
			regionIdx = i;
		}
	}

	compare(pixelRegion, regionIdx, regionMask, CMP_EQ);
	regionMask.convertTo(regionMask, CV_8UC1);

}

void getMainRegionRect(Rect &rect, const Mat &regionMask) {

	Size imgSize = regionMask.size();
	int min_x = INF, min_y = INF;
	int max_x = 0, max_y = 0;

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			if (regionMask.ptr<uchar>(y)[x] == 255) {
				min_x = min(min_x, x);
				max_x = max(max_x, x);
				min_y = min(min_y, y);
				max_y = max(max_y, y);
			}
		}
	}

	int ext = 20;
	max_x = max_x + ext + 1 < imgSize.width ? max_x + ext + 1: imgSize.width;
	max_y = max_y + ext + 1 < imgSize.height ? max_y + ext + 1: imgSize.height;
	min_x = min_x - ext > 0 ? min_x - ext : 0;
	min_y = min_y - ext > 0 ? min_y - ext : 0;

	rect = Rect(min_x, min_y, max_x - min_x, max_y - min_y);

}

void refineSalientObj(Mat &saliencyObj) {

	Size imgSize = saliencyObj.size();
	Mat pixelRegion(imgSize, CV_32SC1, Scalar(-1));
	int regionCount = 0;
	vector<int> regionSaliency;
	TypeQue<Point> &que = *(new TypeQue<Point>);

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			if (saliencyObj.ptr<uchar>(y)[x] < 255) continue;
			if (pixelRegion.ptr<int>(y)[x] != -1) continue;

			pixelRegion.ptr<int>(y)[x] = regionCount;
			regionSaliency.push_back(0);

			que.clear();
			que.push(Point(x,y));
			while (!que.empty()) {

				Point nowP = que.front();
				que.pop();
				regionSaliency[regionCount]++;

				for (int k = 0; k < PIXEL_CONNECT; k++) {

					Point newP = nowP + dxdy[k];
					if (isOutside(newP.x, newP.y, imgSize.width, imgSize.height)) continue;
					if (saliencyObj.at<uchar>(newP) < 255) continue;
					if (pixelRegion.at<int>(newP) == -1) {
						pixelRegion.at<int>(newP) = regionCount;
						que.push(newP);
					}
				}
			}

			regionCount++;

		}
	}

	delete &que;

	int max_regionSaliency = 0;

	for (int i = 0; i < regionCount; i++) {

		//cout << regionSaliency[i] << endl;

		if (max_regionSaliency < regionSaliency[i]) {
			max_regionSaliency = regionSaliency[i];
		}
	}

	max_regionSaliency *= MIN_REGION_SALIENCY;

	for (int i = 0; i < regionCount; i++) {
		if (regionSaliency[i] > max_regionSaliency) {
			regionSaliency[i] = 255;
		} else {
			regionSaliency[i] = 0;
		}
	}

	saliencyObj.setTo(0);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			if (regionIdx == -1) continue;
			saliencyObj.ptr<uchar>(y)[x] = regionSaliency[regionIdx];
		}
	}

}

void getSaliencyObj(Mat &saliencyObj, const Mat &_saliencyMap, const Mat &LABImg, const int thres0) {

	Size imgSize = _saliencyMap.size();

	Mat borderMap(imgSize, CV_8UC1, Scalar(255));
	borderMap(Rect(BORDER_WIDTH,BORDER_WIDTH,imgSize.width-BORDER_WIDTH,imgSize.height-BORDER_WIDTH)).setTo(0);

	//Mat saliencyMap;
	//GaussianBlur(_saliencyMap, saliencyMap, Size(9, 9), 0);
	Mat saliencyMap = _saliencyMap.clone();
	//normalize(saliencyMap, saliencyMap, 0, 255, CV_MINMAX);
	saliencyMap.setTo(0, borderMap);
	threshold(saliencyMap, saliencyMap, thres0, 255, THRESH_TOZERO);

	Mat regionMask;
	getMainRegionMask(regionMask, saliencyMap);

	Rect regionRect;
	getMainRegionRect(regionRect, regionMask);
	saliencyMap = saliencyMap(regionRect);
	regionMask = regionMask(regionRect);

	Mat colorImg = LABImg(regionRect).clone();
	cvtColor(colorImg, colorImg, COLOR_Lab2BGR);
	colorImg.convertTo(colorImg, CV_8UC3, 255);

	Mat GCmask(saliencyMap.size(), CV_8UC1, GC_PR_BGD);
	Mat tmpMask;
	threshold(saliencyMap, tmpMask, thres0, 255, THRESH_BINARY);
	GCmask.setTo(GC_PR_FGD, tmpMask);
	threshold(saliencyMap, tmpMask, HIGH_SALIENCY_THRESHOLD, 255, THRESH_BINARY);
	GCmask.setTo(GC_FGD, regionMask);
	threshold(saliencyMap, tmpMask, LOW_SALIENCY_THRESHOLD, 255, THRESH_BINARY_INV);
	GCmask.setTo(GC_BGD, tmpMask);

	//writeGCMask(GCmask, "Pre_GC.png", 0, 1);
	//imshow("saliency", saliencyMap);
	//imshow("color", colorImg);

	Mat bgdModel, fgdModel;
	grabCut(colorImg, GCmask, Rect(), bgdModel, fgdModel, 3, GC_INIT_WITH_MASK);

	//writeGCMask(GCmask, "After_GC.png", 0, 1);

	GCmask = GCmask & 1;
	compare(GCmask, 1, tmpMask, CMP_EQ);
	Mat globalMask(imgSize, CV_8UC1, GC_PR_BGD);
	(globalMask(regionRect)).setTo(GC_FGD, tmpMask);

	//writeGCMask(globalMask, "Before_Global.png", 0, 1);
	colorImg = LABImg.clone();
	cvtColor(colorImg, colorImg, COLOR_Lab2BGR);
	colorImg.convertTo(colorImg, CV_8UC3, 255);

	bgdModel.release();
	fgdModel.release();
	grabCut(colorImg, globalMask, Rect(), bgdModel, fgdModel, 3, GC_INIT_WITH_MASK);

	//writeGCMask(globalMask, "After_Global.png", 0, 1);
	globalMask = globalMask & 1;
	compare(globalMask, 1, saliencyObj, CMP_EQ);
	//imshow("obj", saliencyObj);
	refineSalientObj(saliencyObj);
	//imshow("_obj", saliencyObj);
	//waitKey();

	if (sum(saliencyObj).val[0] == 0) {
		threshold(_saliencyMap, saliencyObj, 130, 255, THRESH_BINARY);
		cout << "Salient Obj is None !!" << endl;
	}

#ifdef SHOW_IMAGE
	imshow("Salient_Region.png", saliencyObj);
	imwrite("Salient_Region.png", saliencyObj);
#endif
}

#endif // CUTOBJ

