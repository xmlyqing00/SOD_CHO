#ifndef LAYERS_H
#define LAYERS_H

#include "comman.h"

void mergeRegionOverlapCycle(Mat &regionRelation, Mat &regionRoute,
							  Mat &pixelRegion, int &regionCount) {

	TypeTarjan &tarjan = *(new TypeTarjan);

	tarjan.init(regionCount, regionRelation, regionRoute);
	tarjan.getComponent();

	int _regionCount = tarjan.componentCount;
	Mat _relation(_regionCount, _regionCount, CV_32FC1, Scalar(0));
	Mat _route(_regionCount, _regionCount, CV_32SC1, Scalar(0));

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {
			int replaceIdx = tarjan.componentIndex[pixelRegion.ptr<int>(y)[x]];
			pixelRegion.ptr<int>(y)[x] = replaceIdx;
		}
	}

	for (int i = 0; i < regionCount; i++) {

		int replace_i = tarjan.componentIndex[i];
		for (int j = 0; j < regionCount; j++) {

			if (regionRoute.ptr<int>(i)[j] == 0) continue;

			int replace_j = tarjan.componentIndex[j];
			_relation.ptr<float>(replace_i)[replace_j] += regionRelation.ptr<float>(i)[j];
			_route.ptr<int>(replace_i)[replace_j]++;
		}
	}

	for (int i = 0; i < _regionCount; i++) {
		for (int j = 0; j < _regionCount; j++) {
			if (_route.ptr<int>(i)[j] > 0) {
				_relation.ptr<float>(i)[j] /= _route.ptr<int>(i)[j];
				_route.ptr<int>(i)[j] = 1;
			}
		}
	}

	for (int i = 0; i < _regionCount; i++) _route.ptr<int>(i)[i] = 0;

	regionRelation = _relation.clone();
	regionRoute = _route.clone();
	regionCount = _regionCount;

	tarjan.clear();
	delete &tarjan;
}

void getLocalRelation(Mat &regionRelation, Mat &regionRoute) {

	for (int y = 0; y < regionRelation.rows; y++) {
		for (int x = y + 1; x < regionRelation.cols; x++) {

			if (regionRoute.ptr<int>(y)[x] == 0 && regionRoute.ptr<int>(x)[y] == 0) continue;

			regionRoute.ptr<int>(y)[x] = 1;
			regionRoute.ptr<int>(x)[y] = 1;
			float w0 = regionRelation.ptr<float>(y)[x];
			float w1 = regionRelation.ptr<float>(x)[y];
			if (abs(w0) > abs(w1)) {
				regionRelation.ptr<float>(x)[y] = -w0;
			} else {
				regionRelation.ptr<float>(y)[x] = -w1;
			}
		}
	}

}

void getRegionLayer(int *regionLayer, Mat &regionRelation, Mat &regionRoute, Mat &pixelRegion,
					int &regionCount) {

	mergeRegionOverlapCycle(regionRelation, regionRoute, pixelRegion, regionCount);

	getLocalRelation(regionRelation, regionRoute);

    TypeQue<int> &que = *(new TypeQue<int>);

    int *low_layer = new int[regionCount];
    int *below = new int[regionCount];
    for (int i = 0; i < regionCount; i++) {
        low_layer[i] = -1;
        below[i] = 0;
    }
    for (int i = 0; i < regionCount; i++) {

        for (int j = 0; j < regionCount; j++) {

            if (regionRoute.ptr<int>(i)[j] == 0) continue;
			if (regionRelation.ptr<float>(i)[j] <= 0) below[i]++;
        }
        if (below[i] == 0) {
            low_layer[i] = 0;
            que.push(i);
        }
    }

	//for (int i = 0; i < regionCount; i++) cout << below[i] << endl;

    while ( !que.empty() ) {

        int idx = que.front();
        que.pop();

        for (int i = 0; i < regionCount; i++) {

            if (regionRoute.ptr<int>(i)[idx] == 0) continue;
			if (regionRelation.ptr<float>(i)[idx] <= 0) {
                below[i]--;
                if (below[i] == 0) {
                    low_layer[i] = low_layer[idx] + 1;
                    que.push(i);
                }
            }
        }
    }

    delete[] below;
    que.clear();

    int *high_layer = new int[regionCount];
    int *above = new int[regionCount];
    for (int i = 0; i < regionCount; i++) {

        above[i] = 0;
        high_layer[i] = INF;

        for (int j = 0; j < regionCount; j++) {

            if (regionRoute.ptr<int>(i)[j] == 0) continue;
			if (regionRelation.ptr<float>(i)[j] >= 0) above[i]++;
        }
        if (above[i] == 0) {
            que.push(i);
            high_layer[i] = low_layer[i];
        }
    }

    //for (int i = 0; i < regionCount; i++) cout << above[i] << endl;
    while (!que.empty()) {

        int idx = que.front();
        que.pop();

        for (int i = 0; i < regionCount; i++) {

            if (regionRoute.ptr<int>(i)[idx] == 0) continue;

			if (regionRelation.ptr<float>(i)[idx] >= 0) {
                high_layer[i] = min(high_layer[i], high_layer[idx] - 1);
                above[i]--;
                if (above[i] == 0) {
                    que.push(i);
                }
            }
        }
    }

    delete[] above;
	delete &que;

    for (int i = 0; i < regionCount; i++) {
        regionLayer[i] = (high_layer[i]+low_layer[i]) / 2;
    }

    delete[] low_layer;
    delete[] high_layer;

	for (int i = 0; i < regionCount; i++) {

		if (regionLayer[i] > 10) {
			regionLayer[i] = 9;
		} else if (regionLayer[i] > 8) {
			regionLayer[i] = 8;
		} else if (regionLayer[i] > 6) {
			regionLayer[i] = 7;
		}
	}
}

#endif // LAYERS_H

