#ifndef CLUSTERANALYSIS_H
#define CLUSTERANALYSIS_H

#include "basefunction.h"

void fineConvexBoundary( vector<Point> &pixelBound, Point p0, Point p1 ) {

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

void coveringDetect( const Mat &pixelCluster, const int clusterCount, const int *clusterElementCount,
					 const int idx, const vector<Point> &clusterBound, vector<typeLink> &clusterLink ) {

	vector<Point> pixelBound;

	pixelBound.push_back( clusterBound[0] );
	for ( size_t i = 1; i < clusterBound.size(); i++ ) {

		pixelBound.push_back( clusterBound[i] );
		fineConvexBoundary( pixelBound, clusterBound[i - 1], clusterBound[i] );
	}
	fineConvexBoundary( pixelBound, clusterBound[clusterBound.size() - 1], clusterBound[0] );

	sort( pixelBound.begin(), pixelBound.end(), pixelBoundCompare );

	vector<int> clusterCoveringArea( clusterCount + 1, 0 );

	for ( size_t i = 0; i < pixelBound.size(); ) {

		size_t j;
		for ( j = i; j < pixelBound.size(); j++ ) {
			if ( pixelBound[j].x != pixelBound[i].x ) break;
		}
		j--;
		int x = pixelBound[i].x;
		for ( int y = pixelBound[i].y; y <= pixelBound[j].y; y++ ) {

			int clusteridx = pixelCluster.ptr<int>( y )[x];
			clusterCoveringArea[clusteridx]++;
		}
		i = j + 1;
	}
	for ( int i = 1; i <= clusterCount; i++ ) {
		if (idx == 1 && i == 31) {
			cout << endl;
		}
		if ( i == idx || clusterCoveringArea[i] == 0 ) continue;

		int w = clusterCoveringArea[i] * clusterCoveringThreshold / clusterElementCount[i];
		if ( w < 5 ) continue;

		//clusterRealtion.ptr<int>(idx)[i] = w;
		clusterLink.push_back(typeLink(idx, i, w, clusterElementCount[idx]));
		cout << idx << " " << i << " " << w << endl;
	}
}

void getClusterElement( const Mat &pixelCluster, int *clusterElementCount,
						vector<Point> *clusterPixel ) {

	for ( int y = 0; y < pixelCluster.rows; y++ ) {
		for ( int x = 0; x < pixelCluster.cols; x++ ) {

			int clusteridx = pixelCluster.ptr<int>( y )[x];
			if ( clusteridx == 0 ) continue;
			clusterElementCount[clusteridx]++;
			clusterPixel[clusteridx].push_back( Point( x, y ) );
		}
	}
}

void getClusterRelation( Mat &pixelCluster, int &clusterCount, Mat &clusterRelation) {

}

void clusterAnalysis( Mat &pixelCluster, int clusterCount,
					  const Mat &smoothImg, vector<typeLink> &clusterLink ) {

	int *clusterElementCount = new int[clusterCount + 1];
	vector<Point> *clusterPixel = new vector<Point>[clusterCount + 1];

	memset(clusterElementCount, 0, sizeof(int)*(clusterCount + 1));
	getClusterElement(pixelCluster, clusterElementCount, clusterPixel);

	vector< vector<Point> > clusterBound( clusterCount + 1 );

	for ( int i = 1; i <= clusterCount; i++ ) {

		Mat tmpMat = smoothImg.clone();
		convexHull( clusterPixel[i], clusterBound[i] );
		drawContours(tmpMat, clusterBound, i, Scalar(0,255,0));;
		//imshow("Contour", tmpMat);
		cout << i << endl;
		//waitKey(0);
		coveringDetect( pixelCluster, clusterCount, clusterElementCount, i, clusterBound[i], clusterLink );
	}

	Mat clusterRelation = Mat(clusterCount+1, clusterCount+1, CV_8UC1, Scalar(0));
	getClusterRelation(pixelCluster, clusterCount, clusterRelation);

	sort(clusterLink.begin(), clusterLink.end(), linkCompare);

	cout << clusterLink.size() << endl;
	//for ( size_t i = 0; i < clusterLink.size(); i++ ) {
	//	cout << clusterLink[i].u << " " << clusterLink[i].v << " " << clusterLink[i].w << " " << clusterLink[i].baseArea << endl;
	//}

	delete[] clusterElementCount;
	delete[] clusterPixel;

}

void newTopologyLink( const int u, const int v, vector<int> &topologyHead,
					  vector<typeTopology> &topologyLink ) {

	typeTopology oneTopologyLink( u, v, topologyHead[u] );
	topologyHead[u] = topologyLink.size();
	topologyLink.push_back( oneTopologyLink );

}

void updateAncestorRelation( const int u, const int v, const vector<int> &topologyHead,
							 const vector<typeTopology> &topologyLink, Mat &ancestorRelation ) {

	typeQue<int> &que = *(new typeQue<int>);

	que.push( v );
	while ( !que.empty() ) {

		int idx = que.front();
		que.pop();

		bitwise_or( ancestorRelation.row( u ), ancestorRelation.row( idx ), ancestorRelation.row( idx ) );
		ancestorRelation.ptr<uchar>( idx )[u] = 255;

		for ( int p = topologyHead[idx]; p != -1; p = topologyLink[p].next ) {
			que.push( topologyLink[p].v );
		}
	}

	delete &que;
}



void coveringAnalysis( const int clusterCount, const vector<typeLink> &clusterLink,
					   vector<int> &topologyLevel, int *clusterGroup ) {

	vector<bool> linkConflict( clusterLink.size(), false );
	Mat ancestorRelation( clusterCount + 1, clusterCount + 1, CV_8U, Scalar(0) );

	vector<int> topologyHead( clusterCount + 1, -1 );
	vector<typeTopology> topologyLink;
	vector<int> inDegree( clusterCount + 1, 0 );

	for ( int i = 1; i <= clusterCount; i++ ) clusterGroup[i] = i;

	for ( size_t i = 0; i < clusterLink.size(); i++ ) {

		typeLink oneLink = clusterLink[i];

		if ( ancestorRelation.ptr<uchar>( oneLink.u )[oneLink.v] == 255 ) continue;

		unionElement( oneLink.u, oneLink.v, clusterGroup );

		inDegree[oneLink.v]++;
		newTopologyLink( oneLink.u, oneLink.v, topologyHead, topologyLink );
		updateAncestorRelation( oneLink.u, oneLink.v, topologyHead, topologyLink, ancestorRelation );

	}

	//imshow( "Ancestor Relation", ancestorRelation );
	//imwrite( "Ancestor Relation.png", ancestorRelation );

	typeQue<int> &que = *(new typeQue<int>);

	for ( int i = 1; i <= clusterCount; i++ ) {
		if ( inDegree[i] == 0 ) que.push( i );
	}
	//cout << que.size() << endl;
	while ( !que.empty() ) {

		int idx = que.front();
		que.pop();

		for ( int p = topologyHead[idx]; p != -1; p = topologyLink[p].next ) {

			int idv = topologyLink[p].v;
			inDegree[idv]--;
			if ( inDegree[idv] == 0 ) {
				topologyLevel[idv] = topologyLevel[idx] + 1;
				que.push( idv );
			}
		}
	}

	for ( int i = 1; i <= clusterCount; i++ ) {
		if ( inDegree[i] != 0 ) cout << "Error";
	}
}

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


#endif // CLUSTERANALYSIS_H

