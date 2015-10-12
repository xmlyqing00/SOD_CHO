#ifndef CLUSTER_H
#define CLUSTER_H

#include "comman.h"

void getClusterElement( vector<Point> *clusterElement, int *clusterElementCount,
						const Mat &pixelCluster) {

	for ( int y = 0; y < pixelCluster.rows; y++ ) {
		for ( int x = 0; x < pixelCluster.cols; x++ ) {

			int clusterIdx = pixelCluster.ptr<int>( y )[x];
			clusterElementCount[clusterIdx]++;
			clusterElement[clusterIdx].push_back( Point( x, y ) );
		}
	}
}

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

float getCoveringValue(int overlap0, int overlap1) {

	if (max(overlap0, overlap1) < CLUSTER_COVERING) {
		return 0;
	} else {
		if (overlap0 > overlap1) {
			float tmp = (float)overlap0 / overlap1;
			return 1 - pow(e, 1 - tmp);
		} else {
			float tmp = (float)overlap1 / overlap0;
			return pow(e, 1 - tmp) - 1;
		}
	}
}

void getClusterRelation(Mat &clusterRelation, Mat &clusterRoute,
						const Mat &pixelCluster, const int clusterCount) {

	int *clusterElementCount = new int[clusterCount];
	vector<Point> *clusterElement = new vector<Point>[clusterCount];
	Mat clusterOverlap(clusterCount, clusterCount, CV_32SC1, Scalar(0));

	getClusterElement(clusterElement, clusterElementCount, pixelCluster);

	for ( int i = 0; i < clusterCount; i++ ) {

		vector<Point> clusterBound;
		convexHull( clusterElement[i], clusterBound );

		vector<Point> horizontalBound;
		getHorizontalBound(horizontalBound, clusterBound);

		getOverlap(clusterOverlap, i, pixelCluster, horizontalBound);
	}

	TypeLink **clusterNeighbour = new TypeLink*[clusterCount];
	for (int i = 0; i < clusterCount; i++) clusterNeighbour[i] = NULL;
	getClusterNeighbour(clusterNeighbour, pixelCluster);

	clusterRelation = Mat(clusterOverlap.size(), CV_32FC1, Scalar(0));
	clusterRoute = Mat(clusterOverlap.size(), CV_32SC1, Scalar(0));

	for (int i = 0; i < clusterCount; i++) {

		int *clusterOverlapData = clusterOverlap.ptr<int>(i);
		for (TypeLink *p = clusterNeighbour[i]; p != NULL; p = p->next) {

			int overlap0 = clusterOverlapData[p->v];
			int overlap1 = clusterOverlap.ptr<int>(p->v)[p->u];
			clusterRelation.ptr<float>(p->u)[p->v] = getCoveringValue(overlap0, overlap1);
			clusterRoute.ptr<int>(p->u)[p->v] = 1;
		}
	}

	cout << "relation" << endl;
	cout << clusterRelation << endl;
	cout << "count" << endl;
	cout << clusterRoute << endl;
	cout << "=========" << endl;

	Mat _clusterRelation = clusterRelation.clone();
	Mat _clusterRoute = clusterRoute.clone();

	for (int loop = 0; loop < min(10,clusterCount); loop++) {

		Mat relationTmp(clusterRelation.size(), CV_32FC1, Scalar(0));
		Mat routeTmp(clusterRoute.size(), CV_32SC1, Scalar(0));

		for (int i = 0; i < clusterCount; i++) {

			int *_routeData_i = _clusterRoute.ptr<int>(i);
			float *_relationData_i = _clusterRelation.ptr<float>(i);
			int *routeTmp_i = routeTmp.ptr<int>(i);
			float *relationTmp_i = relationTmp.ptr<float>(i);

			for (int j = 0; j < clusterCount; j++) {

				if (j == i) continue;

				for (int k = 0; k < clusterCount; k++) {

					if (k == i || k == j) continue;

					if (_routeData_i[k] == 0 || clusterRoute.ptr<int>(k)[j] == 0) continue;

					float _relationTmp = _relationData_i[k] + clusterRelation.ptr<float>(k)[j];
					int _routeTmp = _routeData_i[k] * clusterRoute.ptr<int>(k)[j];
					relationTmp_i[j] = _relationData_i[j] * _routeData_i[j] + _relationTmp * _routeTmp;
					routeTmp_i[j] = _routeData_i[j] + _routeTmp;
					relationTmp_i[j] /= routeTmp_i[j];
				}
			}
		}

		_clusterRelation = relationTmp.clone();
		_clusterRoute = routeTmp.clone();

		cout << "relation" << endl;
		cout << _clusterRelation << endl;
		cout << "route" << endl;
		cout << _clusterRoute << endl;
		cout << "======5===" << endl;
	}

	clusterRelation = _clusterRelation.clone();
	clusterRoute = _clusterRoute.clone();

	for (int i = 0; i < clusterCount; i++) {
		for (TypeLink *p = clusterNeighbour[i]; p != NULL;) {
			TypeLink *_p = p->next;
			delete p;
			p = _p;
		}
	}
	delete[] clusterNeighbour;

	delete[] clusterElement;
	delete[] clusterElementCount;

}

void getClusterLayer(TypeLayer *clusterLayer, const int clusterCount,
					 const Mat &clusterRelation, const Mat &clusterRoute) {

	for (int i = 0; i < clusterCount; i++) {

		clusterLayer[i].idx = i;
		clusterLayer[i].z_value = 0;
		for (int j = 0; j < clusterCount; j++) {
			if (clusterRoute.ptr<int>(i)[j] > 0) {
				clusterLayer[i].z_value += clusterRelation.ptr<float>(i)[j];
			}
		}
		clusterLayer[i].z_value /= clusterCount;
		clusterLayer[i].z_value *= -1;
	}

	sort(clusterLayer, clusterLayer + clusterCount, cmpTypeLayer);
}

//void newTopologyLink( const int u, const int v, vector<int> &topologyHead,
//					  vector<typeTopology> &topologyLink ) {

//	typeTopology oneTopologyLink( u, v, topologyHead[u] );
//	topologyHead[u] = topologyLink.size();
//	topologyLink.push_back( oneTopologyLink );

//}

//void updateAncestorRelation( const int u, const int v, const vector<int> &topologyHead,
//							 const vector<typeTopology> &topologyLink, Mat &ancestorRelation ) {

//	typeQue<int> &que = *(new typeQue<int>);

//	que.push( v );
//	while ( !que.empty() ) {

//		int idx = que.front();
//		que.pop();

//		bitwise_or( ancestorRelation.row( u ), ancestorRelation.row( idx ), ancestorRelation.row( idx ) );
//		ancestorRelation.ptr<uchar>( idx )[u] = 255;

//		for ( int p = topologyHead[idx]; p != -1; p = topologyLink[p].next ) {
//			que.push( topologyLink[p].v );
//		}
//	}

//	delete &que;
//}



//void coveringAnalysis( const int clusterCount, const vector<typeLink> &clusterLink,
//					   vector<int> &topologyLevel, int *clusterGroup ) {

//	vector<bool> linkConflict( clusterLink.size(), false );
//	Mat ancestorRelation( clusterCount + 1, clusterCount + 1, CV_8U, Scalar(0) );

//	vector<int> topologyHead( clusterCount + 1, -1 );
//	vector<typeTopology> topologyLink;
//	vector<int> inDegree( clusterCount + 1, 0 );

//	for ( int i = 1; i <= clusterCount; i++ ) clusterGroup[i] = i;

//	for ( size_t i = 0; i < clusterLink.size(); i++ ) {

//		typeLink oneLink = clusterLink[i];

//		if ( ancestorRelation.ptr<uchar>( oneLink.u )[oneLink.v] == 255 ) continue;

//		unionElement( oneLink.u, oneLink.v, clusterGroup );

//		inDegree[oneLink.v]++;
//		newTopologyLink( oneLink.u, oneLink.v, topologyHead, topologyLink );
//		updateAncestorRelation( oneLink.u, oneLink.v, topologyHead, topologyLink, ancestorRelation );

//	}

//	//imshow( "Ancestor Relation", ancestorRelation );
//	//imwrite( "Ancestor Relation.png", ancestorRelation );

//	typeQue<int> &que = *(new typeQue<int>);

//	for ( int i = 1; i <= clusterCount; i++ ) {
//		if ( inDegree[i] == 0 ) que.push( i );
//	}
//	//cout << que.size() << endl;
//	while ( !que.empty() ) {

//		int idx = que.front();
//		que.pop();

//		for ( int p = topologyHead[idx]; p != -1; p = topologyLink[p].next ) {

//			int idv = topologyLink[p].v;
//			inDegree[idv]--;
//			if ( inDegree[idv] == 0 ) {
//				topologyLevel[idv] = topologyLevel[idx] + 1;
//				que.push( idv );
//			}
//		}
//	}

//	for ( int i = 1; i <= clusterCount; i++ ) {
//		if ( inDegree[i] != 0 ) cout << "Error";
//	}
//}

void getTopologyLevel( const Mat &pixelCluster, const int clusterCount, const vector<int> &topologyLevel,
					   const Mat &smoothImg, int *clusterGroup ) {

	int maxTopologyLevel = 0;
	for ( int i = 1; i <= clusterCount; i++ ) {
		maxTopologyLevel = max( maxTopologyLevel, topologyLevel[i] );
	}

	vector<int> clusterVisited( clusterCount + 1, 0 );
	/*
	for ( int j = 1; j <= clusterCount; j++ ) {

		if ( clusterVisited[j] != 0 ) continue;
		for ( int k = j; k <= clusterCount; k++ ) {
			if ( getElementHead( j, clusterGroup ) == getElementHead( k, clusterGroup ) ) {
				clusterVisited[k] = j;
				cout << k << endl;
			}
		}
*/
		for ( int i = 0; i <= maxTopologyLevel; i++ ) {

			bool end = true;
			Mat topologyImg( smoothImg.size(), CV_8UC3, Scalar( 0 ) );

			for ( int y = 0; y < smoothImg.rows; y++ ) {
				for ( int x = 0; x < smoothImg.cols; x++ ) {

					int clusterIdx = pixelCluster.ptr<int>( y )[x];
					//if ( clusterVisited[clusterIdx] != j ) continue;
					if ( topologyLevel[clusterIdx] == i ) {
						end = false;
						topologyImg.ptr<Vec3b>( y )[x] = smoothImg.ptr<Vec3b>( y )[x];
					}
				}
			}
			//if ( end ) break;

			char topologyImgName[100];
			sprintf( topologyImgName, "Topology Level %d.png", i );
			//imshow( topologyImgName, topologyImg );
			imwrite( topologyImgName, topologyImg );
			//waitKey();

		}
		destroyAllWindows();
	//}
}


#endif // CLUSTER_H

