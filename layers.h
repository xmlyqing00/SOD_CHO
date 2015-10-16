#ifndef LAYERS_H
#define LAYERS_H

#include "comman.h"

void getClusterLayer(int *clusterLayer, const Mat &clusterRelation, const Mat &clusterRoute,
                    const int clusterCount) {

    TypeQue<int> &que = *(new TypeQue<int>);

    int *low_layer = new int[clusterCount];
    int *below = new int[clusterCount];
    for (int i = 0; i < clusterCount; i++) {
        low_layer[i] = -1;
        below[i] = 0;
    }
    for (int i = 0; i < clusterCount; i++) {

        for (int j = 0; j < clusterCount; j++) {

            if (clusterRoute.ptr<int>(i)[j] == 0) continue;
            if (clusterRelation.ptr<float>(i)[j] < 0) below[i]++;
        }
        if (below[i] == 0) {
            low_layer[i] = 0;
            que.push(i);
        }
    }

    while ( !que.empty() ) {

        int idx = que.front();
        que.pop();

        for (int i = 0; i < clusterCount; i++) {

            if (clusterRoute.ptr<int>(i)[idx] == 0) continue;
            if (clusterRelation.ptr<float>(i)[idx] < 0) {
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

    int *high_layer = new int[clusterCount];
    int *above = new int[clusterCount];
    for (int i = 0; i < clusterCount; i++) {

        above[i] = 0;
        high_layer[i] = INF;

        for (int j = 0; j < clusterCount; j++) {

            if (clusterRoute.ptr<int>(i)[j] == 0) continue;
            if (clusterRelation.ptr<float>(i)[j] > 0) above[i]++;
        }
        if (above[i] == 0) {
            que.push(i);
            high_layer[i] = low_layer[i];
        }
    }

    //for (int i = 0; i < clusterCount; i++) cout << above[i] << endl;
    while (!que.empty()) {

        int idx = que.front();
        que.pop();

        for (int i = 0; i < clusterCount; i++) {

            if (clusterRoute.ptr<int>(i)[idx] == 0) continue;

            if (clusterRelation.ptr<float>(i)[idx] > 0) {
                high_layer[i] = min(high_layer[i], high_layer[idx] - 1);
                above[i]--;
                if (above[i] == 0) {
                    que.push(i);
                }
            }
        }
    }

    delete[] above;

    for (int i = 0; i < clusterCount; i++) {
        clusterLayer[i] = (high_layer[i]+low_layer[i]) / 2;
    }

    delete[] low_layer;
    delete[] high_layer;
}

#endif // LAYERS_H

