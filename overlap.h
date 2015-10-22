#ifndef OVERLAP_H
#define OVERLAP_H

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

void getHorizontalBound(vector<Point> &horizontalBound, const vector<Point> &clusterBound) {

    horizontalBound.push_back(clusterBound[0]);
    for (size_t i = 1; i < clusterBound.size(); i++) {

        horizontalBound.push_back(clusterBound[i]);
        rasterizeLine(horizontalBound, clusterBound[i - 1], clusterBound[i]);
    }
    rasterizeLine(horizontalBound, clusterBound[clusterBound.size() - 1], clusterBound[0]);

    sort( horizontalBound.begin(), horizontalBound.end(), cmpPoint );
}

void getOverlap(Mat &clusterOverlap, const int clusterIdx, const Mat &pixelCluster,
                const vector<Point> &horizontalBound) {

    for ( size_t i = 0; i < horizontalBound.size(); ) {

        size_t j;
        for ( j = i; j < horizontalBound.size(); j++ ) {
            if ( horizontalBound[j].y != horizontalBound[i].y ) break;
        }
        j--;
        int y = horizontalBound[i].y;
        for ( int x = horizontalBound[i].x; x <= horizontalBound[j].x; x++ ) {

            int neighbourIdx = pixelCluster.ptr<int>( y )[x];
            if (neighbourIdx == clusterIdx) continue;
            clusterOverlap.ptr<int>(clusterIdx)[neighbourIdx]++;
        }
        i = j + 1;
    }
}

float getCoveringValue(float overlap0, float overlap1) {

	if (max(overlap0, overlap1) < CLUSTER_CONNECTED) {
		return -INF;
    } else {
        if (overlap0 > overlap1) {

            if (overlap1 == 0) return 1;
            float tmp = (float)overlap0 / overlap1;
			tmp = 1 - pow(e, 1 - tmp);
			return (abs(tmp) < CLUSTER_COVERING) ? 0 : tmp;
        } else {

            if (overlap0 == 0) return -1;
            float tmp = (float)overlap1 / overlap0;
			tmp = pow(e, 1 - tmp) - 1;
			return (abs(tmp) < CLUSTER_COVERING) ? 0 : tmp;
        }
    }
}

#endif // OVERLAP_H

