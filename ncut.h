#ifndef NCUT_H
#define NCUT_H

#include "comman.h"

void getNormalizedCut(vector<int> &regionTag, const Mat &W) {

	int n = W.rows;

	if (n == 1) {
		regionTag.push_back(0);
		return;
	}

	Mat D(W.size(), CV_64FC1, Scalar(0));
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			D.ptr<double>(i)[i] = D.ptr<double>(i)[i] + W.ptr<double>(i)[j];
		}
	}

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

	Mat y;
	solve(D_sqrt, eigenVec.row(n-2).t(), y);
	//cout << sum(abs(D_sqrt * y - eigenVec.row(n-2).t())) << endl;

	double best_partition = -1;
	double min_ncut_result = INF;

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

	regionTag = vector<int>(n);
	for (int i = 0; i < n; i++) {

		if (y.ptr<double>(0)[i] >= best_partition) {
			regionTag[i] = 1;
		} else {
			regionTag[i] = 0;
		}
	}
}

#endif // NCUT_H

