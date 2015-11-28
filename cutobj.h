#ifndef CUTOBJ
#define CUTOBJ

#include "comman.h"
#include "type_que.h"

void setBGBorder(Mat &mask, const int border) {

	Size imgSize = mask.size();
	(mask(Rect(0, 0, imgSize.width, border))).setTo(Scalar(GC_BGD));
	(mask(Rect(0, imgSize.height-border, imgSize.width, border))).setTo(Scalar(GC_BGD));
	(mask(Rect(0, 0, border, imgSize.height))).setTo(Scalar(GC_BGD));
	(mask(Rect(imgSize.width-border, 0, border, imgSize.height))).setTo(Scalar(GC_BGD));
}

void getSaliencyObj(Mat &saliencyObj, const Mat &saliencyMap) {

	Size imgSize = saliencyMap.size();

	Mat mask(imgSize, CV_8UC1, Scalar(GC_PR_BGD));
	mask.setTo(GC_BGD, 255 - saliencyMap);

	TypeQue<Point> &que = *(new TypeQue<Point>);
	Mat visited(imgSize, CV_8UC1, Scalar(0));
	vector< vector<Point> > candidateClusters;
	vector<long long> candidateSaliency;

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			if (saliencyMap.ptr<uchar>(y)[x] <= SALIENCY_THRESHOLD) continue;
			if (visited.ptr<uchar>(y)[x] == 1) continue;

			vector<Point> candidateCluster;
			que.push(Point(x,y));
			visited.ptr<uchar>(y)[x] = 1;
			long long saliencyValue = 0;

			while (!que.empty()) {

				Point nowP = que.front();
				que.pop();
				candidateCluster.push_back(nowP);
				saliencyValue += saliencyMap.at<uchar>(nowP);

				for (int k = 0; k < PIXEL_CONNECT; k++) {

					Point newP = nowP + dxdy[k];
					if (isOutside(newP.x, newP.y, imgSize.width, imgSize.height)) continue;
					if (saliencyMap.at<uchar>(newP) <= SALIENCY_THRESHOLD) continue;
					if (visited.at<uchar>(newP) == 1) continue;

					visited.at<uchar>(newP) = 1;
					que.push(newP);
				}
			}

			candidateClusters.push_back(candidateCluster);
			candidateSaliency.push_back(saliencyValue);

		}
	}

	long long max_saliency = 0;
	int regionsIdx = 0;
	for (size_t i = 0; i < candidateClusters.size(); i++) {
		if (max_saliency < candidateSaliency[i]) {
			max_saliency = candidateSaliency[i];
			regionsIdx = i;
		}
	}



	for (size_t i = 0; i < candidateClusters[regionsIdx].size(); i++) {
		mask.at<uchar>(candidateClusters[regionsIdx][i]) = GC_PR_FGD;
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

#ifdef SHOW_IMAGE
	imwrite("Salient_Region.png", saliencyObj);
#endif
}

#endif // CUTOBJ

