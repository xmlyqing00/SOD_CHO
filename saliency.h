#ifndef SALIENCY_H
#define SALIENCY_H

#include "comman.h"

void getSaliencyMap(Mat &saliencyMap, const Mat &W, const Mat &D,
					const vector<int> &frontRegion, const Mat &pixelRegion) {

	int n = frontRegion.size();

	Mat D_sqrt;
	sqrt(D, D_sqrt);
	Mat D_inv = D.inv();
	Mat tmpMat = D_inv * (D-W) * D_inv;

	Mat eigenVec, eigenVal;
	bool eigenExist = eigen(tmpMat, eigenVal, eigenVec, -1, -1);
	if (!eigenExist) {
		cout << "Eigen does not exist !!" << endl;
		return;
	}
	//cout << sum(abs(tmpMat * eigenVec.row(n-2).t() - eigenVal.ptr<double>(0)[n-2] * eigenVec.row(n-2).t())) << endl;
}

#endif // SALIENCY_H


