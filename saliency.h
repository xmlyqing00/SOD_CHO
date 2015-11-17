#ifndef SALIENCY_H
#define SALIENCY_H

#include "comman.h"

void getSaliencyMap(Mat &saliencyMap, Mat &W, Mat &D, const Mat &pixelRegion) {

	int n = W.rows;
	Mat D_sqrt;
	sqrt(D, D_sqrt);
	Mat D_inv = D_sqrt.inv();
	Mat D_W = D - W;
	Mat tmpMat = D_inv * D_W * D_inv;

	Mat eigenVec, eigenVal;
	bool eigenExist = eigen(tmpMat, eigenVal, eigenVec, -1, -1);
	if (!eigenExist) {
		cout << "Eigen does not exist !!" << endl;
		return;
	}

	//cout << sum(abs(tmpMat * eigenVec.row(n-2).t() - eigenVal.ptr<double>(0)[n-2] * eigenVec.row(n-2).t())) << endl;

	Mat y;
	solve(D_sqrt, eigenVec.row(n-2).t(), y);
	//cout << sum(abs(D_sqrt * y - eigenVec.row(n-2).t())) << endl;

	double best_partition = -1;
	double min_ncut_result = INF;

	//cout << y << endl;

	for (int i = 0; i < n; i++) {

		double sumA = 0, sumB = 0;

		for (int j = 0; j < n; j++) {
			if (float2sign(y.ptr<double>(0)[j]-y.ptr<double>(0)[i]) >= 0) {
				sumA += D.ptr<double>(j)[j];
			} else {
				sumB += D.ptr<double>(j)[j];
			}
		}

		if (sumA == 0 || sumB == 0) continue;

		double b = sumA / sumB;

		vector<double> tune_y(n);
		for (int j = 0; j < n; j++) {
			if (float2sign(y.ptr<double>(0)[j]-y.ptr<double>(0)[i]) >= 0) {
				tune_y[j] = 1;
			} else {
				tune_y[j] = -b;
			}
		}

		Mat vec_y(tune_y);

		Mat tmp1 = vec_y.t() * D_W * vec_y;
		Mat tmp2 = vec_y.t() * D * vec_y;
		double ncut_result = tmp1.ptr<double>(0)[0] / tmp2.ptr<double>(0)[0];

		if (min_ncut_result > ncut_result) {
			min_ncut_result = ncut_result;
			best_partition = y.ptr<double>(0)[i];
		}
	}

	int *regionTag = new int[n];
	for (int i = 0; i < n; i++) {

		if (y.ptr<double>(0)[i] >= best_partition) {
			regionTag[i] = 1;
		} else {
			regionTag[i] = 0;
		}
	}

	int partArea[2], hullArea[2];
	for (int regionFlag = 0; regionFlag < 2; regionFlag++) {

		vector<Point> pixelPart;
		for (int y = 0; y < pixelRegion.rows; y++) {
			for (int x = 0; x < pixelRegion.cols; x++) {
				if (regionTag[pixelRegion.ptr<int>(y)[x]] == regionFlag) {
					pixelPart.push_back(Point(x,y));
				}
			}
		}

		partArea[regionFlag] = pixelPart.size();

		vector<Point> pixelHull;
		convexHull(pixelPart, pixelHull);

		Mat hullAreaMap(pixelRegion.size(), CV_8UC1, Scalar(0));
		fillConvexPoly(hullAreaMap, pixelHull, Scalar(255));
		hullArea[regionFlag] = sum(hullAreaMap).val[0] - partArea[regionFlag];

	}

	double overlap[2];
	overlap[0] = (double)hullArea[0] / partArea[1];
	overlap[1] = (double)hullArea[1] / partArea[0];
	if (overlap[0] < overlap[1]) {
		for (int i = 0; i < n; i++) regionTag[i] = 1 - regionTag[i];
	}

	saliencyMap = Mat(pixelRegion.size(), CV_8UC1, Scalar(0));
	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			saliencyMap.ptr<uchar>(y)[x] = 255 * regionTag[regionIdx];
		}
	}

	delete[] regionTag;

	imshow("Saliency_Map", saliencyMap);
	imwrite("Saliency_Map.png", saliencyMap);

}

#endif // SALIENCY_H
