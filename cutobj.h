#ifndef CUTOBJ
#define CUTOBJ

#include "comman.h"
#include "type_que.h"

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

void getSaliencyObj(Mat &saliencyObj, const Mat &_saliencyMap, const Mat &LABImg, const int thres0) {

	Size imgSize = _saliencyMap.size();

	Mat borderMap(imgSize, CV_8UC1, Scalar(255));
	borderMap(Rect(BORDER_WIDTH,BORDER_WIDTH,imgSize.width-BORDER_WIDTH,imgSize.height-BORDER_WIDTH)).setTo(0);

	Mat saliencyMap = _saliencyMap.clone();
	saliencyMap.setTo(0, borderMap);
	threshold(saliencyMap, saliencyMap, thres0, 255, THRESH_TOZERO);

	Mat regionMask;
	getMainRegionMask(regionMask, saliencyMap);

	Rect regionRect;
	getMainRegionRect(regionRect, regionMask);
	saliencyMap = saliencyMap(regionRect);
	borderMap = borderMap(regionRect);
	regionMask = regionMask(regionRect);

	Mat colorImg = LABImg(regionRect).clone();
	cvtColor(colorImg, colorImg, COLOR_Lab2BGR);
	colorImg.convertTo(colorImg, CV_8UC3, 255);

	Mat GCmask(saliencyMap.size(), CV_8UC1, GC_PR_BGD);
	Mat tmpMask;
	threshold(saliencyMap, tmpMask, thres0, 255, THRESH_BINARY);
	GCmask.setTo(GC_PR_FGD, tmpMask);
	threshold(saliencyMap, tmpMask, HIGH_SALIENCY_THRESHOLD, 255, THRESH_BINARY);
	GCmask.setTo(GC_FGD, tmpMask);

	Mat bgdModel, fgdModel;
	grabCut(colorImg, GCmask, Rect(), bgdModel, fgdModel, 4, GC_INIT_WITH_MASK);

	GCmask = GCmask & 1;
	compare(GCmask, 1, GCmask, CMP_EQ);
	saliencyObj = Mat(imgSize, CV_8UC1, Scalar(0));
	GCmask.copyTo(saliencyObj(regionRect));

#ifdef SHOW_IMAGE
	imshow("Salient_Region.png", saliencyObj);
	imwrite("Salient_Region.png", saliencyObj);
#endif
}

#endif // CUTOBJ

