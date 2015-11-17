#ifndef GRAPH_H
#define GRAPH_H

#include "comman.h"
#include "type_que.h"

void rasterizeLine( vector<Point> &pixelBound, Point p0, Point p1 ) {

	int dx = p1.x - p0.x;
	int dy = p1.y - p0.y;

	if ( dx == 0 && dy == 0 ) {
		pixelBound.push_back( p0 );
		return;
	}

	if ( abs(dx) <= abs(dy) ) {

		if ( dy < 0 ) {
			swap( p0, p1 );
			dx = -dx;
			dy = -dy;
		}

		double slope = (double)dx / dy;
		for ( int step = 1; step < dy; step++ ) {
			int _y = p0.y + step;
			int _x = p0.x + cvRound( slope * step );
			pixelBound.push_back( Point( _x, _y ) );
		}

	} else {
		if ( dx < 0 ) {
			swap( p0, p1 );
			dx = -dx;
			dy = -dy;
		}

		double slope = (double)dy / dx;
		for ( int step = 1; step < dx; step++ ) {
			int _x = p0.x + step;
			int _y = p0.y + cvRound( slope * step );
			pixelBound.push_back( Point( _x, _y ) );
		}

	}
}

void getHorizontalBound(vector<Point> &horizontalBound, const vector<Point> &regionBound) {

	horizontalBound.push_back(regionBound[0]);
	for (size_t i = 1; i < regionBound.size(); i++) {

		horizontalBound.push_back(regionBound[i]);
		rasterizeLine(horizontalBound, regionBound[i - 1], regionBound[i]);
	}
	rasterizeLine(horizontalBound, regionBound[regionBound.size() - 1], regionBound[0]);

	sort( horizontalBound.begin(), horizontalBound.end(), cmpPoint);
}

void getOverlap(Mat &regionOverlap, const int regionIdx, const Mat &pixelRegion,
				const vector<Point> &horizontalBound) {

	for ( size_t i = 0; i < horizontalBound.size(); ) {

		size_t j;
		for ( j = i; j < horizontalBound.size(); j++ ) {
			if ( horizontalBound[j].y != horizontalBound[i].y ) break;
		}
		j--;
		int y = horizontalBound[i].y;
		for ( int x = horizontalBound[i].x; x <= horizontalBound[j].x; x++ ) {

			int neighbourIdx = pixelRegion.ptr<int>( y )[x];
			if (neighbourIdx == regionIdx) continue;
			regionOverlap.ptr<int>(regionIdx)[neighbourIdx]++;
		}
		i = j + 1;
	}
}

int getCoveringValue(double overlap0, double overlap1) {

	if (max(overlap0, overlap1) < MIN_REGION_CONNECTED) {
		return -2;
	} else {

		double tmp0 = e / (e - 1);
		double tmp = -pow(e, -overlap0 / 1) + pow(e, -overlap1 / 1);
		tmp = tmp * tmp0;

		if (abs(tmp) < MIN_COVERING) return 0;
		return tmp > 0 ? 1 : -1;
	}
}

void getRegionNeighbour(Mat &regionNeighbour, const Mat &pixelRegion, const int regionCount) {

	regionNeighbour = Mat(regionCount, regionCount, CV_8UC1, Scalar(0));
	Mat neighbourPixelCount = Mat(regionCount, regionCount, CV_32SC1, Scalar(0));

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			Point nowP = Point(x,y);
			int regionIdx1 = pixelRegion.ptr<int>(y)[x];

			for (int k = 0; k < PIXEL_CONNECT; k++) {

				Point newP = nowP + dxdy[k];
				if (isOutside(newP.x, newP.y, pixelRegion.cols, pixelRegion.rows)) continue;
				int regionIdx2 = pixelRegion.ptr<int>(newP.y)[newP.x];

				if (regionIdx1 != regionIdx2) {
					int tmp1 = min(regionIdx1, regionIdx2);
					int tmp2 = max(regionIdx1, regionIdx2);
					neighbourPixelCount.ptr<int>(tmp1)[tmp2]++;
				}
			}
		}
	}

	for (int i = 0; i < regionCount; i++) {
		for (int j = i + 1; j < regionCount; j++) {
			if (neighbourPixelCount.ptr<int>(i)[j] > MIN_REGION_NEIGHBOUR) {
				regionNeighbour.ptr<uchar>(i)[j] = 1;
				regionNeighbour.ptr<uchar>(j)[i] = 1;
			} else {
				regionNeighbour.ptr<uchar>(i)[j] = 0;
				regionNeighbour.ptr<uchar>(j)[i] = 0;
			}
		}
	}

//	for (int i = 0; i < regionCount; i++) {
//		Mat tmp(pixelRegion.size(), CV_8UC3, Scalar(0));
//		for (int y = 0; y < tmp.rows; y++) {
//			for (int x = 0; x < tmp.cols; x++) {
//				int regionIdx = pixelRegion.ptr<int>(y)[x];
//				if (regionIdx == i) {
//					tmp.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
//				}
//				if (regionNeighbour.ptr<uchar>(i)[regionIdx]) {
//					tmp.ptr<Vec3b>(y)[x] = Vec3b(0, 255, 0);
//				}
//			}
//		}
//		imshow("region", tmp);
//		waitKey(0);
//	}

	vector<bool> unreachableRegion(regionCount, true);
	bool unreachExist = false;
	for (int i = 0; i < regionCount; i++) {

		for (int j = 0; j < regionCount; j++) {
			if (regionNeighbour.ptr<uchar>(i)[j] == 1) {
				unreachableRegion[i] = false;
				break;
			}
		}
		if (unreachableRegion[i]) unreachExist = true;
	}

	if (!unreachExist) return;

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			Point nowP = Point(x,y);
			int regionIdx1 = pixelRegion.ptr<int>(y)[x];

			if (!unreachableRegion[regionIdx1]) continue;

			for (int k = 0; k < PIXEL_CONNECT; k++) {

				Point newP = nowP + dxdy[k];
				if (isOutside(newP.x, newP.y, pixelRegion.cols, pixelRegion.rows)) continue;
				int regionIdx2 = pixelRegion.ptr<int>(newP.y)[newP.x];

				if (regionIdx1 != regionIdx2) {
					regionNeighbour.ptr<uchar>(regionIdx1)[regionIdx2] = 1;
					regionNeighbour.ptr<uchar>(regionIdx2)[regionIdx1] = 1;
				}
			}
		}
	}
}

void getRegionRelation(Mat &relation, const Mat &regionNeighbour, const Mat &tmpc,
					   const Mat &pixelRegion, const int regionCount) {

	TypeQue<int> &que = *(new TypeQue<int>);

	relation = Mat(regionCount, regionCount, CV_32SC1, Scalar(0));
	for (int i = 0; i < regionCount; i++) {

		int *reach = new int[regionCount];
		int *layer = new int[regionCount];
		int *max_layer = new int[regionCount];
		int *min_layer = new int[regionCount];
		memset(reach, 0, sizeof(int)*regionCount);
		memset(layer, 0, sizeof(int)*regionCount);
		memset(max_layer, 0, sizeof(int)*regionCount);
		memset(min_layer, 0, sizeof(int)*regionCount);

		que.clear();
		que.push(i);
		reach[i] = 1;

		while (!que.empty()) {

			int idx = que.front();
			que.pop();
			layer[idx] /= reach[idx];
			relation.ptr<int>(i)[idx] /= reach[idx];
			reach[idx] = -1;

			for (int j = 0; j < regionCount; j++) {

				if (reach[j] == -1) continue;
				if (regionNeighbour.ptr<uchar>(idx)[j] == 0) continue;

				int tmp = layer[idx] + tmpc.ptr<int>(idx)[j];
				layer[j] += tmp;
				max_layer[j] = max(max_layer[idx], tmp);
				min_layer[j] = min(min_layer[idx], tmp);
				relation.ptr<int>(i)[j] += abs(max_layer[j] - min_layer[j]);

				if (reach[j] == 0) que.push(j);
				reach[j]++;
			}
		}

		delete[] reach;
		delete[] layer;
		delete[] max_layer;
		delete[] min_layer;

	}

	delete &que;
}

void buildRegionGraph(Mat &W, Mat &D, const Mat *pyramidRegion, const vector< vector<int> > *pyramidMap,
					  const vector<Vec3b> &regionColor, const double GAMA, const double PARAM1, const double PARAM2) {

	int baseRegionCount = pyramidMap[0].size();
	W = Mat(baseRegionCount, baseRegionCount, CV_64FC1, Scalar(0));
	D = Mat(baseRegionCount, baseRegionCount, CV_64FC1, Scalar(0));
	Mat c(baseRegionCount, baseRegionCount, CV_32SC1, Scalar(0));

	// update c
	for (int pyramidIdx = 0; pyramidIdx < PYRAMID_SIZE; pyramidIdx++) {

		int regionCount = pyramidMap[pyramidIdx].size();

		int *regionElementCount = new int[regionCount];
		vector<Point> *regionElement = new vector<Point>[regionCount];
		for (int i = 0; i < regionCount; i++) {
			regionElementCount[i] = 0;
			regionElement[i].clear();
		}
		getRegionElement(regionElement, regionElementCount, pyramidRegion[pyramidIdx]);

		Mat regionOverlap(regionCount, regionCount, CV_32SC1, Scalar(0));

		for ( int i = 0; i < regionCount; i++ ) {

			vector<Point> regionBound;
			convexHull( regionElement[i], regionBound );

			vector<Point> horizontalBound;
			getHorizontalBound(horizontalBound, regionBound);

			getOverlap(regionOverlap, i, pyramidRegion[pyramidIdx], horizontalBound);
		}

//		for (int i = 0; i < regionCount; i++) {
//			Mat tmp(pyramidRegion[0].size(), CV_8UC1, Scalar(0));
//			for (int y = 0; y < tmp.rows; y++) {
//				for (int x = 0; x < tmp.cols; x++) {
//					if (pyramidRegion[pyramidIdx].ptr<int>(y)[x] == i) {
//						tmp.ptr<uchar>(y)[x] = 255;
//					}
//				}
//			}

//			imshow("region", tmp);
//			waitKey(0);
//		}

		Mat tmpc(regionCount, regionCount, CV_32SC1, Scalar(0));

		for (int i = 0; i < regionCount; i++) {

			for (int j = i + 1; j < regionCount; j++) {

				double overlap0 = (double)regionOverlap.ptr<int>(i)[j] / regionElementCount[j];
				double overlap1 = (double)regionOverlap.ptr<int>(j)[i] / regionElementCount[i];
				int regionRelation = getCoveringValue(overlap0, overlap1);

				switch (regionRelation) {
				case -1:
					tmpc.ptr<int>(i)[j] = -1;
					break;
				case 1:
					tmpc.ptr<int>(i)[j] = 1;
					break;
				case 0:
				case -2:
					break;
				}
			}
		}

		Mat regionNeighbour;
		getRegionNeighbour(regionNeighbour, pyramidRegion[pyramidIdx], regionCount);

		// increase neighbour region c
		for (int i = 0; i < regionCount; i++) {

			vector<int> convexhullRegion;
			for (int j = 0; j < regionCount; j++) {

				if (j == i) continue;
				if (tmpc.ptr<int>(i)[j] == 1) convexhullRegion.push_back(j);
			}
			for (size_t j = 0; j < convexhullRegion.size(); j++) {
				for (size_t k = j + 1; k < convexhullRegion.size(); k++) {

					int regionIdx1 = convexhullRegion[j];
					int regionIdx2 = convexhullRegion[k];
					if (regionNeighbour.ptr<uchar>(regionIdx1)[regionIdx2] == 0) continue;

					for (size_t j_ele = 0; j_ele < pyramidMap[pyramidIdx][regionIdx1].size(); j_ele++) {
						for (size_t k_ele = 0; k_ele < pyramidMap[pyramidIdx][regionIdx2].size(); k_ele++) {
							c.ptr<int>(pyramidMap[pyramidIdx][regionIdx1][j_ele])[pyramidMap[pyramidIdx][regionIdx2][k_ele]]++;
						}
					}
				}
			}
		}

		// increase inside regions c
		for (int i = 0; i < regionCount; i++) {
			for (size_t j = 0; j < pyramidMap[pyramidIdx][i].size(); j++) {
				for (size_t k = 0; k < pyramidMap[pyramidIdx][i].size(); k++) {

					if (j == k) continue;
					c.ptr<int>(pyramidMap[pyramidIdx][i][j])[pyramidMap[pyramidIdx][i][k]]++;
				}
			}
		}
		//cout << c << endl;

		// get region relation
		Mat regionRelation;
		getRegionRelation(regionRelation, regionNeighbour, tmpc, pyramidRegion[pyramidIdx], regionCount);
		//cout << regionRelation << endl;

		// decrease outside regions c
		for (int i = 0; i < regionCount; i++) {
			for (int j = i + 1; j < regionCount; j++) {

				int r = regionRelation.ptr<int>(i)[j];
				if (r == 0) continue;

				//cout << r << endl;

				for (size_t mapi_ele = 0; mapi_ele < pyramidMap[pyramidIdx][i].size(); mapi_ele++) {
					for (size_t mapj_ele = 0; mapj_ele < pyramidMap[pyramidIdx][j].size(); mapj_ele++) {
						c.ptr<int>(pyramidMap[pyramidIdx][i][mapi_ele])[pyramidMap[pyramidIdx][j][mapj_ele]] -= r;
					}
				}
			}
		}

		//cout << c << endl;

		delete[] regionElement;
		delete[] regionElementCount;


//		for (int i = 0; i < regionCount; i++) {
//			Mat tmp(pyramidRegion[pyramidIdx].size(), CV_8UC3, Scalar(0));
//			int max_c = 0;
//			for (int j = 0; j < regionCount; j++) {
//				max_c = max(max_c, -c.ptr<int>(i)[j]);
//			}
//			cout << max_c << endl;
//			for (int y = 0; y < tmp.rows; y++) {
//				for (int x = 0; x < tmp.cols; x++) {
//					int regionIdx = pyramidRegion[pyramidIdx].ptr<int>(y)[x];
//					if (regionIdx == i) {
//						tmp.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
//					} else {
//						tmp.ptr<Vec3b>(y)[x] = Vec3b(0, -(double)c.ptr<int>(i)[regionIdx] / max_c * 255, 0);
//					}
//				}
//			}
//			imshow("region", tmp);
//			waitKey(0);
//		}
	}

	// init W
	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = i + 1; j < baseRegionCount; j++) {
			double w = pow(e, -(double)colorDiff(regionColor[i], regionColor[j]) / 20);
			//cout << w << " " << colorDiff(regionColor[i], regionColor[j]) << endl;
			W.ptr<double>(i)[j] = w;
		}
	}

	//cout << W << endl;

	// update W with d
	Mat regionDist;
	getRegionDist(regionDist, pyramidRegion[0], baseRegionCount);

	int width = pyramidRegion[0].cols;
	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = i + 1; j < baseRegionCount; j++) {
			double d = pow(e, -(double)regionDist.ptr<int>(i)[j] / width);
			d = PARAM1 + (1-PARAM1)*d;
			//cout << d << endl;
			W.ptr<double>(i)[j] *= d;
		}
	}

	// update W with size
	int *regionElementCount = new int[baseRegionCount];
	vector<Point> *regionElement = new vector<Point>[baseRegionCount];
	for (int i = 0; i < baseRegionCount; i++) {
		regionElementCount[i] = 0;
		regionElement[i].clear();
	}
	getRegionElement(regionElement, regionElementCount, pyramidRegion[0]);

	double sizeSigma = 0;
	for (int i = 0; i < baseRegionCount; i++) sizeSigma += regionElementCount[i];
	sizeSigma /= baseRegionCount;
	sizeSigma = sizeSigma * sizeSigma;
	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = i + 1; j < baseRegionCount; j++) {
			double size = pow(e, -(double)regionElementCount[i]*regionElementCount[j]/sizeSigma);
			size = PARAM2 + (1-PARAM2)*size;
			//cout << size << endl;
			W.ptr<double>(i)[j] *= size;
		}
	}
	delete[] regionElement;
	delete[] regionElementCount;

	// update W with c
	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = i + 1; j < baseRegionCount; j++) {
			W.ptr<double>(i)[j] = W.ptr<double>(i)[j] * pow(GAMA, c.ptr<int>(i)[j]);
			W.ptr<double>(j)[i] = W.ptr<double>(i)[j];
		}
	}

	//cout << W << endl;

//	for (int i = 0; i < baseRegionCount; i++) {
//		Mat tmp(pyramidRegion[0].size(), CV_8UC3, Scalar(0));
//		Mat cc(pyramidRegion[0].size(), CV_8UC3, Scalar(0));
//		double max_w = 0;
//		int max_c = 0;
//		for (int j = 0; j < baseRegionCount; j++) {
//			max_w = max(max_w, W.ptr<double>(i)[j]);
//			max_c = max(max_c, -c.ptr<int>(i)[j]);
//		}

//		for (int y = 0; y < tmp.rows; y++) {
//			for (int x = 0; x < tmp.cols; x++) {
//				int regionIdx = pyramidRegion[0].ptr<int>(y)[x];
//				if (regionIdx == i) {
//					tmp.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
//					cc.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
//				} else {
//					tmp.ptr<Vec3b>(y)[x] = Vec3b(0, W.ptr<double>(i)[regionIdx] / max_w * 255, 0);
//					cc.ptr<Vec3b>(y)[x] = Vec3b(0, -(double)c.ptr<int>(i)[regionIdx] / max_c * 255, 0);
//				}
//			}
//		}
//		imshow("W", tmp);
//		imshow("c", cc);
//		waitKey(0);
//	}

	for (int i = 0; i < baseRegionCount; i++) {
		D.ptr<double>(i)[i] = 0;
		for (int j = 0; j < baseRegionCount; j++) {
			D.ptr<double>(i)[i] = D.ptr<double>(i)[i] + W.ptr<double>(i)[j];
		}
	}
}

#endif // GRAPH_H
