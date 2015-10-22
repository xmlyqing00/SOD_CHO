#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include "comman.h"

void getSpatialCorrelation(Mat &clusterSpatialCorrelation, const Mat &pixelCluster, const int clusterCount) {

	Point *clusterCenter = new Point[clusterCount];
	int *clusterRadius = new int[clusterCount];

	int *clusterElementCount = new int[clusterCount];
	vector<Point> *clusterElement = new vector<Point>[clusterCount];
	for (int i = 0; i < clusterCount; i++) {
		clusterElementCount[i] = 0;
		clusterElement->clear();
	}
	getClusterElement(clusterElement, clusterElementCount, pixelCluster);

	for (int i = 0; i < clusterCount; i++) {

		clusterCenter[i] = Point(0, 0);
		for (size_t j = 0; j < clusterElement[i].size(); j++) {
			clusterCenter[i] += clusterElement[i][j];
		}
		clusterCenter[i].x /= clusterElementCount[i];
		clusterCenter[i].y /= clusterElementCount[i];
	}

	for (int i = 0; i < clusterCount; i++) {

		clusterRadius[i] = 0;
		for (size_t j = 0; j < clusterElement[i].size(); j++) {
			int distTmp = getPointDist(clusterCenter[i], clusterElement[i][j]);
			clusterRadius[i] = max(clusterRadius[i], distTmp);
		}
	}

	for (int i = 0; i < clusterCount; i++) {
		for (int j = i + 1; j < clusterCount; j++) {

			int centerDist = getPointDist(clusterCenter[i], clusterCenter[j]);
			if (CLUSTER_CORRELATION * (clusterRadius[i] + clusterRadius[j]) >  centerDist) {
				clusterSpatialCorrelation.ptr<uchar>(i)[j] = 1;
				clusterSpatialCorrelation.ptr<uchar>(j)[i] = 1;
			} else {
				clusterSpatialCorrelation.ptr<uchar>(i)[j] = 0;
				clusterSpatialCorrelation.ptr<uchar>(j)[i] = 0;
			}
		}
	}

	delete[] clusterCenter;
	delete[] clusterRadius;
	delete[] clusterElementCount;
	delete[] clusterElement;

}

void mergeCluster( Mat &pixelCluster, int &clusterCount, vector<Vec3b> &clusterColor ) {

	Mat clusterSpatialCorrelation(clusterCount, clusterCount, CV_8UC1);
	getSpatialCorrelation(clusterSpatialCorrelation, pixelCluster, clusterCount);

	TypeLink **clusterNeighbour = new TypeLink*[clusterCount];
	for (int i = 0; i < clusterCount; i++) clusterNeighbour[i] = NULL;
	getClusterNeighbour(clusterNeighbour, pixelCluster, clusterCount);

	int *replaceByIdx = new int[clusterCount];
	int *replaceCount = new int[clusterCount];
	Vec3b *replaceColor = new Vec3b[clusterCount];
	for (int i = 0; i < clusterCount; i++) {
		replaceByIdx[i] = i;
		replaceCount[i] = 1;
		replaceColor[i] = clusterColor[i];
	}

	for (int i = 0; i < clusterCount; i++) {
		for (TypeLink *p0 = clusterNeighbour[i]; p0 != NULL; p0 = p0->next) {
			for (TypeLink *p1 = clusterNeighbour[i]; p1 != NULL; p1 = p1->next) {

				if (p0->v == p1->v) continue;
				if (clusterSpatialCorrelation.ptr<uchar>(p0->v)[p1->v] == 0) continue;

				if (colorDiff(clusterColor[p0->v], clusterColor[p1->v]) < COLOR_DIFF / 2) {

					int pa0 = getElementHead(p0->v, replaceByIdx);
					int pa1 = getElementHead(p1->v, replaceByIdx);
					Vec3i color = (Vec3i)replaceColor[pa0] * replaceCount[pa0] +
								  (Vec3i)replaceColor[pa1] * replaceCount[pa1];
					replaceCount[pa0] += replaceCount[pa1];
					replaceColor[pa0] = color / replaceCount[pa0];
					replaceByIdx[pa0] = pa1;
				}
			}
		}
	}

	for (int i = 0; i < clusterCount; i++) {
		for (TypeLink *p = clusterNeighbour[i]; p != NULL;) {
			TypeLink *_p = p->next;
			delete p;
			p = _p;
		}
	}
	delete[] clusterNeighbour;

	//for (int i = 1; i <= clusterCount; i++) cout << i << " " << replaceByIdx[i] << " " <<  <<endl;

	int *clusterBucket = new int[clusterCount];
	memset(clusterBucket, 0, sizeof(int)*(clusterCount));
	for (int i = 0; i < clusterCount; i++) {
		clusterBucket[getElementHead(i, replaceByIdx)] = 1;
	}

	int _clusterCount = 0;
	for (int i = 0; i < clusterCount; i++) {
		if (clusterBucket[i] == 1) clusterBucket[i] = _clusterCount++;
	}

	for (int y = 0; y < pixelCluster.rows; y++) {
		for (int x = 0; x < pixelCluster.cols; x++) {

			int clusterIdx = pixelCluster.ptr<int>(y)[x];
			pixelCluster.ptr<int>(y)[x] = clusterBucket[replaceByIdx[clusterIdx]];
		}
	}

	//for (int i = 1; i <= clusterCount; i++) cout << i << " " << replaceByIdx[i] << " " << clusterBucket[i] << " " << clusterColor[i] << endl;

	vector<Vec3b> _clusterColor(_clusterCount);
	for (int i = 0; i < clusterCount; i++) {
		int clusterIdx = clusterBucket[replaceByIdx[i]];
		_clusterColor[clusterIdx] = clusterColor[i];
	}
	clusterColor = _clusterColor;
	clusterCount = _clusterCount;

	delete[] clusterBucket;
	delete[] replaceColor;
	delete[] replaceCount;
	delete[] replaceByIdx;

}

void segmentation( Mat &pixelCluster, int &clusterCount, vector<Vec3b> &clusterColor,
				   Mat &smoothImg, const Mat &cannyImg, const Mat &inputImg ) {

	TypeQue<Point> &que = *(new TypeQue<Point>);

	for ( int y = 0; y < inputImg.rows; y++ ) {
		for ( int x = 0; x < inputImg.cols; x++ ) {

			if ( pixelCluster.ptr<int>( y )[x] != 0 ) continue;
			if ( cannyImg.ptr<uchar>( y )[x] == 255 ) continue;

			pixelCluster.ptr<int>( y )[x] = ++clusterCount;

			que.clear();
			que.push( Point( x, y ) );

			vector<Point> pixelBuffer;
			pixelBuffer.push_back( Point( x, y ) );
			Vec3b seedColor = inputImg.ptr<Vec3b>( y )[x];

			while ( !que.empty() ) {

				Point nowP = que.front();
				que.pop();
				seedColor = inputImg.ptr<Vec3b>(nowP.y)[nowP.x];

					for ( int k = 0; k < PIXEL_CONNECT; k++ ) {

					Point newP = nowP + dxdy[k];

					if ( isOutside( newP.x, newP.y, inputImg.cols, inputImg.rows ) ) continue;
					if ( pixelCluster.ptr<int>( newP.y )[newP.x] != 0 ) continue;
					if ( colorDiff( seedColor, inputImg.ptr<Vec3b>( newP.y )[newP.x] ) > COLOR_DIFF ) continue;

					pixelCluster.ptr<int>( newP.y )[newP.x] = clusterCount;
					pixelBuffer.push_back( Point( newP.x, newP.y ) );

					if ( cannyImg.ptr<uchar>( newP.y )[newP.x] != 255 ) {
						que.push( Point( newP.x, newP.y ) );
					}
				}
			}
			if ( pixelBuffer.size() < CLUSTER_SIZE ) {
				for ( size_t i = 0; i < pixelBuffer.size(); i++ ) {
					pixelCluster.ptr<int>( pixelBuffer[i].y )[pixelBuffer[i].x] = 0;
				}
				clusterCount--;
			}
		}
	}

	//drawClusterImg( clusterCount, pixelCluster, "Cluster Image.png" );

	que.clear();
	for ( int y = 0; y < pixelCluster.rows; y++ ) {
		for ( int x = 0; x < pixelCluster.cols; x++ ) {

			if ( pixelCluster.ptr<int>( y )[x] != 0 ) que.push( Point( x, y ) );
		}
	}


	while ( !que.empty() ) {

		Point nowP = que.front();
		que.pop();

		int clusterIdx = pixelCluster.ptr<int>(nowP.y)[nowP.x];

			for ( int k = 0; k < PIXEL_CONNECT; k++ ) {

			Point newP = nowP + dxdy[k];

			if ( isOutside( newP.x, newP.y, pixelCluster.cols, pixelCluster.rows ) ) continue;
			if ( pixelCluster.ptr<int>( newP.y )[newP.x] != 0 ) continue;

			pixelCluster.ptr<int>( newP.y )[newP.x] = clusterIdx;
			que.push( Point( newP.x, newP.y ) );
		}
	}

	for (int y = 0; y < pixelCluster.rows; y++) {
		for (int x = 0; x < pixelCluster.cols; x++) {

			if (pixelCluster.ptr<int>(y)[x] == clusterCount) {
				pixelCluster.ptr<int>(y)[x] = 0;
			}
		}
	}

	delete &que;

	map<int, int> *colorBucket = new map<int, int>[clusterCount];

	for (int y = 0; y < pixelCluster.rows; y++) {
		for (int x = 0; x < pixelCluster.cols; x++) {

			int clusterIdx = pixelCluster.ptr<int>(y)[x];
			colorBucket[clusterIdx][hashVec3b(inputImg.ptr<Vec3b>(y)[x])]++;
		}
	}

	clusterColor = vector<Vec3b>(clusterCount);

	for (int i = 0; i < clusterCount; i++) {

		map<int, int>::iterator it;

		int mostAppearColorHash = 0;
		int mostAppearTimes = 0;

		for (it = colorBucket[i].begin(); it != colorBucket[i].end(); ++it) {

			//cout << it->first << " " << it->second << endl;
			if (it->second > mostAppearTimes) {
				mostAppearTimes = it->second;
				mostAppearColorHash = it->first;
			}
		}

		clusterColor[i] = deHashVec3b(mostAppearColorHash);
	}

	delete[] colorBucket;

	mergeCluster( pixelCluster, clusterCount, clusterColor );

	for (int y = 0; y < smoothImg.rows; y++) {
		for ( int x = 0; x < smoothImg.cols; x++) {

			int clusterIdx = pixelCluster.ptr<int>(y)[x];
			smoothImg.ptr<Vec3b>(y)[x] = clusterColor[clusterIdx];
		}
	}
	//cout << clusterCount << endl;

}

#endif // SEGMENTATION_H
