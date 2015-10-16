#ifndef CLUSTER_H
#define CLUSTER_H

#include "comman.h"
#include "overlap.h"

void getInitRelation(Mat &clusterRelation, Mat &clusterRoute, const Mat &pixelCluster,
					 const vector<Point> *clusterElement, const int *clusterElementCount,
					 const int clusterCount) {

	Mat clusterOverlap(clusterCount, clusterCount, CV_32SC1, Scalar(0));

	for ( int i = 0; i < clusterCount; i++ ) {

		vector<Point> clusterBound;
		convexHull( clusterElement[i], clusterBound );

		vector<Point> horizontalBound;
		getHorizontalBound(horizontalBound, clusterBound);

		getOverlap(clusterOverlap, i, pixelCluster, horizontalBound);

//		if (i == 36 || i == 63 || i == 1) {
//			cout << i << endl;
//			Mat tmp(1300, 1300, CV_8UC1, Scalar(0));
//			for (size_t j = 0; j < horizontalBound.size(); j++) {
//				//cout << horizontalBound[j] << endl;
//				tmp.ptr<uchar>(horizontalBound[j].y)[horizontalBound[j].x] = 255;
//			}
//			resize(tmp, tmp, Size(), 0.5, 0.5);
//			imshow("tmp", tmp);
//			waitKey(0);
//		}
	}

	clusterRelation = Mat(clusterOverlap.size(), CV_32FC1, Scalar(0));
	clusterRoute = Mat(clusterOverlap.size(), CV_32SC1, Scalar(0));

	for (int i = 0; i < clusterCount; i++) {

		for (int j = 0; j < clusterCount; j++) {

			float overlap0 = (float)clusterOverlap.ptr<int>(i)[j] / clusterElementCount[j];
			float overlap1 = (float)clusterOverlap.ptr<int>(j)[i] / clusterElementCount[i];
			clusterRelation.ptr<float>(i)[j] = getCoveringValue(overlap0, overlap1);
			if (abs(clusterRelation.ptr<float>(i)[j]) < COVERING_RATE ) {
				clusterRelation.ptr<float>(i)[j] = 0;
			}
			clusterRoute.ptr<int>(i)[j] = 1;

		}
	}
}

void mergeClusterOverlapCycle(Mat &clusterRelation, Mat &clusterRoute,
							  Mat &pixelCluster, int &clusterCount) {

	TypeTarjan &tarjan = *(new TypeTarjan);

	tarjan.init(clusterCount, clusterRelation, clusterRoute);
	tarjan.getComponent();

//	for (int i = 0; i < tarjan.componentCount; i++) {
//		if (tarjan.component[i].size() > 2) {
//			Mat tmp(pixelCluster.size(), CV_8UC1, Scalar(0));
//			for (size_t j = tarjan.component[i].size()-1; j > 0 ; j--) {
//				cout << tarjan.component[i][j] << " ";
//				if (j < tarjan.component[i].size()-1) {
//					cout << clusterRelation.ptr<float>(tarjan.component[i][j+1])[tarjan.component[i][j]] << " ";
//				}
//				for (int y = 0; y < pixelCluster.rows; y++) {
//					for (int x = 0; x < pixelCluster.cols; x++) {
//						if (tarjan.component[i][j] == pixelCluster.ptr<int>(y)[x]) {
//							tmp.ptr<uchar>(y)[x] = j * 255 / tarjan.component[i].size();
//						}
//					}
//				}
//				if (tarjan.component[i][0] == 39) {
//					imshow("tmp1", tmp);
//					cout << endl;
//					waitKey();
//				}
//			}
//			imshow("tmp", tmp);
//			waitKey(0);
//			cout << endl;
//		}
//	}

	int _clusterCount = tarjan.componentCount;
	Mat _relation(_clusterCount, _clusterCount, CV_32FC1, Scalar(0));
	Mat _route(_clusterCount, _clusterCount, CV_32SC1, Scalar(0));

	for (int y = 0; y < pixelCluster.rows; y++) {
		for (int x = 0; x < pixelCluster.cols; x++) {
			int replaceIdx = tarjan.componentIndex[pixelCluster.ptr<int>(y)[x]];
			pixelCluster.ptr<int>(y)[x] = replaceIdx;
		}
	}

	for (int i = 0; i < clusterCount; i++) {

		int replace_i = tarjan.componentIndex[i];
		for (int j = 0; j < clusterCount; j++) {
			int replace_j = tarjan.componentIndex[j];
			_relation.ptr<float>(replace_i)[replace_j] += clusterRelation.ptr<float>(i)[j];
			_route.ptr<int>(replace_i)[replace_j]++;
		}
	}

	for (int i = 0; i < _clusterCount; i++) {
		for (int j = 0; j < _clusterCount; j++) {
			if (_route.ptr<int>(i)[j] > 0) {
				_relation.ptr<float>(i)[j] /= _route.ptr<int>(i)[j];
				_route.ptr<int>(i)[j] = 1;
			}
		}
	}

	for (int i = 0; i < _clusterCount; i++) _route.ptr<int>(i)[i] = 0;

	clusterRelation = _relation.clone();
	clusterRoute = _route.clone();
	clusterCount = _clusterCount;

	tarjan.clear();
	delete &tarjan;
}

void getLocalRelation(Mat &clusterRelation, Mat &clusterRoute) {

	for (int y = 0; y < clusterRelation.rows; y++) {
		for (int x = y + 1; x < clusterRelation.cols; x++) {

			if (clusterRoute.ptr<int>(y)[x] == 0 && clusterRoute.ptr<int>(x)[y] == 0) continue;

			clusterRoute.ptr<int>(y)[x] = 1;
			clusterRoute.ptr<int>(x)[y] = 1;
			float w0 = clusterRelation.ptr<float>(y)[x];
			float w1 = clusterRelation.ptr<float>(x)[y];
			if (abs(w0) > abs(w1)) {
				clusterRelation.ptr<float>(x)[y] = -w0;
			} else {
				clusterRelation.ptr<float>(y)[x] = -w1;
			}
		}
	}

}

void getClusterRelation(Mat &clusterRelation, Mat &clusterRoute, Mat &pixelCluster, int &clusterCount, Mat &smoothImg) {

	int *clusterElementCount = new int[clusterCount];
	vector<Point> *clusterElement = new vector<Point>[clusterCount];
	for (int i = 0; i < clusterCount; i++) {
		clusterElementCount[i] = 0;
		clusterElement[i].clear();
	}
	getClusterElement(clusterElement, clusterElementCount, pixelCluster);

	getInitRelation(clusterRelation, clusterRoute, pixelCluster, clusterElement, clusterElementCount, clusterCount);
	delete[] clusterElement;
	delete[] clusterElementCount;

	mergeClusterOverlapCycle(clusterRelation, clusterRoute, pixelCluster, clusterCount);

	getLocalRelation(clusterRelation, clusterRoute);

}

#endif // CLUSTER_H

