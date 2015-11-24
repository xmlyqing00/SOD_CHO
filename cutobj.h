#ifndef CUTOBJ
#define CUTOBJ

#include "comman.h"

void setBGBorder(Mat &mask, const int border) {

	Size imgSize = mask.size();
	(mask(Rect(0, 0, imgSize.width, border))).setTo(Scalar(GC_BGD));
	(mask(Rect(0, imgSize.height-border, imgSize.width, border))).setTo(Scalar(GC_BGD));
	(mask(Rect(0, 0, border, imgSize.height))).setTo(Scalar(GC_BGD));
	(mask(Rect(imgSize.width-border, 0, border, imgSize.height))).setTo(Scalar(GC_BGD));
}

void getSaliencyObj(Mat &saliencyObj, const Mat &saliencyMap) {

	Size imgSize = saliencyMap.size();
	Mat mask(imgSize, CV_8UC1);

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			if (saliencyMap.ptr<uchar>(y)[x] <= 30) {

				if (saliencyMap.ptr<uchar>(y)[x] < 10) {
					mask.ptr<uchar>(y)[x] = GC_BGD;
				} else {
					mask.ptr<uchar>(y)[x] = GC_PR_BGD;
				}

			} else {
				if (saliencyMap.ptr<uchar>(y)[x] >= 240) {
					mask.ptr<uchar>(y)[x] = GC_PR_FGD;
				} else {
					mask.ptr<uchar>(y)[x] = GC_PR_FGD;
				}
			}
		}
	}

	int border = 15;
	Rect rect(border, border, imgSize.width-2*border, imgSize.height-2*border);
	setBGBorder(mask, border);
	Mat bgdModel, fgdModel;
	Mat _saliencyMap;
	cvtColor(saliencyMap, _saliencyMap, COLOR_GRAY2RGB);
	grabCut(_saliencyMap, mask, rect, bgdModel, fgdModel, 2, GC_INIT_WITH_MASK);

	int ITER_COUNT = 4;
	while (ITER_COUNT--) {

		Mat binMask, tmpMat;

		//compare(mask, GC_FGD, binMask, CMP_EQ);
		//mask.setTo(GC_PR_FGD, binMask);
		compare(mask, GC_PR_FGD, binMask, CMP_EQ);
		erode(binMask, tmpMat, Mat(), Point(-1, -1), 5);
		mask.setTo(GC_PR_FGD, binMask);
		mask.setTo(GC_FGD, tmpMat);

		//imshow("bin", binMask);
		//imshow("dilate", tmpMat);
		//waitKey(0);

		compare(mask, GC_BGD, binMask, CMP_EQ);
		erode(binMask, tmpMat, Mat(), Point(-1, -1), 5);
		mask.setTo(GC_PR_BGD, binMask);
		mask.setTo(GC_BGD, tmpMat);

//		imshow("bin", binMask);
//		imshow("dilate", tmpMat);
//		waitKey(0);

		setBGBorder(mask, border);
		grabCut(_saliencyMap, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_MASK);

	}
	saliencyObj = Mat(imgSize, CV_8UC1, Scalar(0));
	saliencyObj.setTo(Scalar(255), mask & 1);
}

#endif // CUTOBJ

