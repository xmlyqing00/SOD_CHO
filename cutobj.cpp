#include "cutobj.h"

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

	Mat saliencyMap;
	GaussianBlur(_saliencyMap, saliencyMap, Size(9,9), 0);

	Size imgSize = saliencyMap.size();
	//Rect borderRect(BORDER_WIDTH, BORDER_WIDTH, imgSize.width - BORDER_WIDTH*2, imgSize.height - BORDER_WIDTH*2);

	Mat GCMask(imgSize, CV_8UC1, GC_PR_FGD);
	//GCMask(borderRect).setTo(GC_PR_FGD);

	Mat tmpMask;
	threshold(saliencyMap, tmpMask, thres0, 255, THRESH_BINARY_INV);
	GCMask.setTo(GC_PR_BGD, tmpMask);
	threshold(saliencyMap, tmpMask, HIGH_SALIENCY_THRESHOLD, 255, THRESH_BINARY);
	GCMask.setTo(GC_FGD, tmpMask);
	threshold(saliencyMap, tmpMask, LOW_SALIENCY_THRESHOLD, 255, THRESH_BINARY_INV);
	GCMask.setTo(GC_BGD, tmpMask);
#ifdef SHOW_IMAGE
	writeGCMask(GCMask, "debug_output/GCMask_pre.png", 0, 1);
#endif
	Mat colorImg = LABImg.clone();
	cvtColor(colorImg, colorImg, COLOR_Lab2BGR);
	colorImg.convertTo(colorImg, CV_8UC3, 255);
	Mat bgdModel, fgdModel;
	//cvtColor(saliencyMap, tmpMask, COLOR_GRAY2BGR);
	grabCut(colorImg, GCMask, Rect(), bgdModel, fgdModel, 6, GC_INIT_WITH_MASK);

#ifdef SHOW_IMAGE
	writeGCMask(GCMask, "debug_output/GCMask_after.png", 0, 1);
#endif

	GCMask = GCMask & 1;
	compare(GCMask, 1, saliencyObj, CMP_EQ);

	refineSalientObj(saliencyObj);

	if (sum(saliencyObj).val[0] == 0) {
		threshold(saliencyMap, saliencyObj, 130, 255, THRESH_BINARY);
		cout << "Salient Obj is None !!" << endl;
	}

#ifdef SHOW_IMAGE
	imshow("Salient_Region.png", saliencyObj);
	imwrite("debug_output/Salient_Region.png", saliencyObj);
#endif
}