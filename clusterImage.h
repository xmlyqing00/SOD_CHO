#ifndef CLUSTERIMAGE
#define CLUSTERIMAGE

#include "basefunction.h"

void mergeCluster( Mat &pixelCluster, int &clusterCount, vector<Vec3b> &clusterColor ) {

	Mat clusterConnectMat(clusterCount+1, clusterCount+1, CV_8UC1, Scalar(0));

	for (int y = 0; y < pixelCluster.rows; y++) {
		for (int x = 0; x < pixelCluster.cols; x++) {

			int clusterIdx0 = pixelCluster.ptr<int>(y)[x];
			for (int k = 0; k < pixelConnect; k++) {

				Point newP = Point(x,y) + dxdy[k];

				if (isOutside(newP.x, newP.y, pixelCluster.cols, pixelCluster.rows)) continue;
				int clusterIdx1 = pixelCluster.ptr<int>(newP.y)[newP.x];
				clusterConnectMat.ptr<uchar>(clusterIdx0)[clusterIdx1] = 255;
			}
		}
	}

	int *replaceByIdx = new int[clusterCount + 1];
	for (int i = 1; i <= clusterCount; i++) replaceByIdx[i] = i;

	for (int i = 1; i <= clusterCount; i++) {
		for (int j = i + 1; j <= clusterCount; j++) {

			if (clusterColor[i] != clusterColor[j]) continue;
			for (int k = 1; k <= clusterCount; k++) {
				if (clusterConnectMat.ptr<uchar>(i)[k] == 0) continue;
				if (clusterConnectMat.ptr<uchar>(j)[k] == 0) continue;
				unionElement(i, j, replaceByIdx);
			}
		}
	}

	//for (int i = 1; i <= clusterCount; i++) cout << i << " " << replaceByIdx[i] << " " <<  <<endl;

	int *sortBucket = new int[clusterCount + 1];
	memset(sortBucket, 0, sizeof(int)*(clusterCount + 1));
	for (int i = 1; i <= clusterCount; i++) sortBucket[getElementHead(i, replaceByIdx)] = 1;

	int _clusterCount = 0;
	for (int i = 1; i <= clusterCount; i++) {
		if (sortBucket[i] == 1) sortBucket[i] = ++_clusterCount;
	}

	for (int y = 0; y < pixelCluster.rows; y++) {
		for (int x = 0; x < pixelCluster.cols; x++) {

			int clusterIdx = pixelCluster.ptr<int>(y)[x];
			pixelCluster.ptr<int>(y)[x] = sortBucket[replaceByIdx[clusterIdx]];
		}
	}

	//for (int i = 1; i <= clusterCount; i++) cout << i << " " << replaceByIdx[i] << " " << sortBucket[i] << " " << clusterColor[i] << endl;

	vector<Vec3b> _clusterColor(_clusterCount + 1);
	for (int i = 1; i <= clusterCount; i++) {
		int clusterIdx = sortBucket[replaceByIdx[i]];
		_clusterColor[clusterIdx] = clusterColor[i];
	}
	clusterColor = _clusterColor;
	clusterCount = _clusterCount;

	delete[] sortBucket;
	delete[] replaceByIdx;

	cout << clusterCount << endl;

	drawClusterImg( clusterCount, pixelCluster, "Merge Cluster Image.png" );
}



void clusterImg( Mat &pixelCluster, int &clusterCount, vector<Vec3b> &clusterColor, const Mat &cannyImg, const Mat &smoothImg ) {

	typeQue<Point> &que = *(new typeQue<Point>);

	for ( int y = 0; y < smoothImg.rows; y++ ) {
		for ( int x = 0; x < smoothImg.cols; x++ ) {

			if ( pixelCluster.ptr<int>( y )[x] != 0 ) continue;
			if ( cannyImg.ptr<uchar>( y )[x] == 255 ) continue;

			pixelCluster.ptr<int>( y )[x] = ++clusterCount;

			que.clear();
			que.push( Point( x, y ) );

			vector<Point> pixelBuffer;
			pixelBuffer.push_back( Point( x, y ) );
			Vec3b seedColor = smoothImg.ptr<Vec3b>( y )[x];

			while ( !que.empty() ) {

				Point nowP = que.front();
				que.pop();
				seedColor = smoothImg.ptr<Vec3b>(nowP.y)[nowP.x];

				for ( int k = 0; k < pixelConnect; k++ ) {

					Point newP = nowP + dxdy[k];

					if ( isOutside( newP.x, newP.y, smoothImg.cols, smoothImg.rows ) ) continue;
					if ( pixelCluster.ptr<int>( newP.y )[newP.x] != 0 ) continue;
					if ( colorDiff( seedColor, smoothImg.ptr<Vec3b>( newP.y )[newP.x] ) > colorDiffThreshold ) continue;

					pixelCluster.ptr<int>( newP.y )[newP.x] = clusterCount;
					pixelBuffer.push_back( Point( newP.x, newP.y ) );

					if ( cannyImg.ptr<uchar>( newP.y )[newP.x] != 255 ) {
						que.push( Point( newP.x, newP.y ) );
					}
				}
			}
			if ( pixelBuffer.size() < pixelClusterSizeThreshold ) {
				for ( size_t i = 0; i < pixelBuffer.size(); i++ ) {
					pixelCluster.ptr<int>( pixelBuffer[i].y )[pixelBuffer[i].x] = 0;
				}
				clusterCount--;
			}
		}
	}

	cout << clusterCount << endl;

	drawClusterImg( clusterCount, pixelCluster, "Cluster Image.png" );

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

		for ( int k = 0; k < pixelConnect; k++ ) {

			Point newP = nowP + dxdy[k];

			if ( isOutside( newP.x, newP.y, pixelCluster.cols, pixelCluster.rows ) ) continue;
			if ( pixelCluster.ptr<int>( newP.y )[newP.x] != 0 ) continue;

			pixelCluster.ptr<int>( newP.y )[newP.x] = clusterIdx;
			que.push( Point( newP.x, newP.y ) );
		}
	}

	drawClusterImg( clusterCount, pixelCluster, "Fine Cluster Image.png" );

	delete &que;

	map<int, int> *colorBucket = new map<int, int>[clusterCount + 1];

	for (int y = 0; y < pixelCluster.rows; y++) {
		for (int x = 0; x < pixelCluster.cols; x++) {

			int clusterIdx = pixelCluster.ptr<int>(y)[x];
			colorBucket[clusterIdx][hashVec3b(smoothImg.ptr<Vec3b>(y)[x])]++;
		}
	}

	clusterColor = vector<Vec3b>(clusterCount + 1);

	for (int i = 1; i <= clusterCount; i++) {

		map<int, int>::iterator it;

		int mostAppearColorHash;
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

}

#endif // CLUSTERIMAGE

