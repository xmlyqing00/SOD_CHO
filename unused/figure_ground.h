#ifndef FIGURE_GROUND_H
#define FIGURE_GROUND_H

#include "comman.h"
#include "type_que.h"



double getCoveringValue(double overlap0, double overlap1) {

	/*if (max(overlap0, overlap1) < MIN_REGION_CONNECTED) {
		return 0;
	} else */{

		double tmp0 = e / (e - 1);
		double tmp = -pow(e, -overlap0 / 10) + pow(e, -overlap1 / 10);
		tmp = tmp * tmp0;

		if (abs(tmp) < MIN_COVERING) return 0;
		return tmp ;
	}
}

void getCenterBias(double &centerBias, const vector<Point> &pts, const Point &midP) {

	centerBias = 0;

	for (size_t i = 0; i < pts.size(); i++) {
		centerBias += abs(pts[i].x - midP.x) + abs(pts[i].y - midP.y);
	}

	centerBias /= pts.size();

	centerBias = pow(e, -(double)centerBias / (midP.x+midP.y));

}

double checkFigureGround(const Mat &saliencyMap) {

	Size imgSize = saliencyMap.size();

	vector< vector<Point> > ptsComponent;
	Mat visited(imgSize, CV_8UC1, Scalar(0));
	TypeQue<Point> &que = *(new TypeQue<Point>);

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			if (saliencyMap.ptr<uchar>(y)[x] == 0) {
				continue;
			}

			if (visited.ptr<uchar>(y)[x] == 1) continue;

			que.clear();
			que.push(Point(x,y));
			visited.ptr<uchar>(y)[x] = 1;
			vector<Point> pts;

			while (!que.empty()) {

				Point nowP = que.front();
				que.pop();
				pts.push_back(nowP);

				for (int k = 0; k < PIXEL_CONNECT; k++) {

					Point newP = nowP + dxdy[k];
					if (isOutside(newP.x, newP.y, imgSize.width, imgSize.height)) continue;
					if (visited.ptr<uchar>(newP.y)[newP.x] == 1) continue;
					if (saliencyMap.ptr<uchar>(newP.y)[newP.x] == 0) continue;

					visited.ptr<uchar>(newP.y)[newP.x] = 1;
					que.push(newP);
				}
			}

			ptsComponent.push_back(pts);

		}
	}

	vector<Point> pts;
	for (size_t i = 0; i < ptsComponent.size(); i++) {
		for (size_t j = 0; j < ptsComponent[i].size(); j++) {
			pts.push_back(ptsComponent[i][j]);
		}
	}
	ptsComponent.push_back(pts);

	double avgSaliency = 0, mainSaliency = 0;
	long long bgSize = (long long)imgSize.width * imgSize.height - pts.size();
	Point midP = Point(imgSize.width/2, imgSize.height/2);

	for (size_t i = 0; i < ptsComponent.size(); i++) {

		vector<Point> regionBound;
		convexHull( ptsComponent[i], regionBound );

		vector<Point> horizontalBound;
		getHorizontalBound(horizontalBound, regionBound);

		long long overlap;
		getOverlap(overlap, saliencyMap, horizontalBound, 0);
		double overlapRate = (double)(overlap) / (bgSize);

		if (i < ptsComponent.size() - 1) {
			avgSaliency += overlapRate;
		} else {
			mainSaliency = overlapRate;
		}

	}

	//avgSaliency /= (ptsComponent.size()-1);
	//mainSaliency = avgSaliency * 0.5 + mainSaliency * 0.5;

	pts.clear();
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			if (saliencyMap.ptr<uchar>(y)[x] == 0) pts.push_back(Point(x,y));
		}
	}
	vector<Point> regionBound;
	convexHull( pts, regionBound );

	vector<Point> horizontalBound;
	getHorizontalBound(horizontalBound, regionBound);

	long long overlap;
	getOverlap(overlap, saliencyMap, horizontalBound, 1);
	double overlapRate = (double)(overlap) / (ptsComponent.back().size());

	cout << overlapRate << " " << mainSaliency << " ";

	double coveringRate = getCoveringValue(overlapRate, mainSaliency);

	double centerBias;
	getCenterBias(centerBias, ptsComponent.back(), midP);

	cout << coveringRate << " " << centerBias << " " << coveringRate*centerBias << endl;
	mainSaliency = coveringRate * centerBias;

	return mainSaliency;
}

#endif // FIGURE_GROUND_H

