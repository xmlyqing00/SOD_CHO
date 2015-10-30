#ifndef SEGMENT_H
#define SEGMENT_H

#include "comman.h"
#include "type_que.h"

void segmentImage( Mat &pixelRegion, int &regionCount, vector<Vec3b> &regionColor,
				   const Mat &cannyImg, const Mat &inputImg ) {

	TypeQue<Point> &que = *(new TypeQue<Point>);

	for ( int y = 0; y < inputImg.rows; y++ ) {
		for ( int x = 0; x < inputImg.cols; x++ ) {

			if ( pixelRegion.ptr<int>( y )[x] != 0 ) continue;
			if ( cannyImg.ptr<uchar>( y )[x] == 255 ) continue;

			pixelRegion.ptr<int>( y )[x] = ++regionCount;

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
					if ( pixelRegion.ptr<int>( newP.y )[newP.x] != 0 ) continue;
					if ( colorDiff( seedColor, inputImg.ptr<Vec3b>( newP.y )[newP.x] ) > COLOR_DIFF ) continue;

					pixelRegion.ptr<int>( newP.y )[newP.x] = regionCount;
					pixelBuffer.push_back( Point( newP.x, newP.y ) );

					if ( cannyImg.ptr<uchar>( newP.y )[newP.x] != 255 ) {
						que.push( Point( newP.x, newP.y ) );
					}
				}
			}
			if ( pixelBuffer.size() < REGION_SIZE ) {
				for ( size_t i = 0; i < pixelBuffer.size(); i++ ) {
					pixelRegion.ptr<int>( pixelBuffer[i].y )[pixelBuffer[i].x] = 0;
				}
				regionCount--;
			}
		}
	}

	//drawRegionImg( regionCount, pixelRegion, "Region Image.png" );

	//TypeQue<Point> &que = *(new TypeQue<Point>);
	que.clear();
	for ( int y = 0; y < pixelRegion.rows; y++ ) {
		for ( int x = 0; x < pixelRegion.cols; x++ ) {

			if ( pixelRegion.ptr<int>( y )[x] != 0 ) que.push( Point( x, y ) );
		}
	}


	while ( !que.empty() ) {

		Point nowP = que.front();
		que.pop();

		int regionIdx = pixelRegion.ptr<int>(nowP.y)[nowP.x];

			for ( int k = 0; k < PIXEL_CONNECT; k++ ) {

			Point newP = nowP + dxdy[k];

			if ( isOutside( newP.x, newP.y, pixelRegion.cols, pixelRegion.rows ) ) continue;
			if ( pixelRegion.ptr<int>( newP.y )[newP.x] != 0 ) continue;

			pixelRegion.ptr<int>( newP.y )[newP.x] = regionIdx;
			que.push( Point( newP.x, newP.y ) );
		}
	}

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			if (pixelRegion.ptr<int>(y)[x] == regionCount) {
				pixelRegion.ptr<int>(y)[x] = 0;
			}
		}
	}

	delete &que;

	map<int, int> *colorBucket = new map<int, int>[regionCount];

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			colorBucket[regionIdx][hashVec3b(inputImg.ptr<Vec3b>(y)[x])]++;
		}
	}

	regionColor = vector<Vec3b>(regionCount);

	for (int i = 0; i < regionCount; i++) {

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

		regionColor[i] = deHashVec3b(mostAppearColorHash);
	}

	delete[] colorBucket;

	//cout << regionCount << endl;
	writeRegionImage(regionCount, pixelRegion, "Segment_Region.png");

}

#endif // SEGMENT_H
