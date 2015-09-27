#ifndef SMOOTHIMAGE_H
#define SMOOTHIMAGE_H

#include "basefunction.h"

void smoothMat( const Mat &img, Mat &smoothImg ) {

	const int colorSpaceRange = 256;
	const int filterRange = 10;
	const int colorSpaceDim[3] = { colorSpaceRange, colorSpaceRange, colorSpaceRange };
	Mat colorSpace = Mat( 3, colorSpaceDim, CV_32S, Scalar( 0 ) );
	smoothImg = Mat( img.size(), CV_8UC3 );

	for ( int y = 0; y < img.rows; y++ ) {
		for ( int x = 0; x < img.cols; x++ ) {

			uchar r = img.ptr<Vec3b>( y )[x].val[0];
			uchar g = img.ptr<Vec3b>( y )[x].val[1];
			uchar b = img.ptr<Vec3b>( y )[x].val[2];
			colorSpace.ptr<int>( r, g )[b]++;
		}
	}

	vector<Vec3b> localMaxColor;
	for ( int r = 0; r < colorSpaceRange; r++ ) {
		for ( int g = 0; g < colorSpaceRange; g++ ) {
			for ( int b = 0; b < colorSpaceRange; b++ ) {

				if ( colorSpace.ptr<int>( r, g )[b] < 5 ) continue;

				bool isLocalMax = true;

				for ( int dr = -filterRange; dr <= filterRange; dr++ ) {

					int _r = r + dr;
					if ( _r < 0 ) continue;
					if ( _r >= colorSpaceRange ) break;

					for ( int dg = -filterRange; dg <= filterRange; dg++ ) {

						int _g = g + dg;
						if ( _g < 0 ) continue;
						if ( _g >= colorSpaceRange ) break;

						for ( int db = -filterRange; db <= filterRange; db++ ) {

							int _b = b + db;
							if ( _b < 0 ) continue;
							if ( _b >= colorSpaceRange ) break;

							//cout << colorSpace.ptr<int>( r, g )[b] << " " << colorSpace.ptr<int>( _r, _g )[_b] << endl;
							if ( colorSpace.ptr<int>( r, g )[b] < colorSpace.ptr<int>( _r, _g )[_b] ) {
								isLocalMax = false;
								break;
							}
						}
						if ( !isLocalMax ) break;
					}
					if ( !isLocalMax ) break;
				}

				if ( isLocalMax ) localMaxColor.push_back( Vec3b( r, g, b ) );

			}
		}
	}

	for ( int y = 0; y < img.rows; y++ ) {
		for ( int x = 0; x < img.cols; x++ ) {

			//if ( cannyImg.ptr<uchar>( y )[x] == 255 ) continue;

			Vec3b pixelColor = img.ptr<Vec3b>( y )[x];
			int mostSimilarColoridx = 0;
			int minDiff = INF;
			for ( size_t i = 0; i < localMaxColor.size(); i++ ) {

				int curDiff = colorDiff( pixelColor, localMaxColor[i] );
				if ( minDiff > curDiff ) {
					minDiff = curDiff;
					mostSimilarColoridx = i;
				}
			}
			smoothImg.ptr<Vec3b>( y )[x] = localMaxColor[mostSimilarColoridx];
		}
	}

/*
	for ( size_t i = 0; i < localMaxColor.size(); i++ ) {

		Mat tempImg( smoothImg.size(), CV_8UC3, Scalar( 0 ) );
		for ( int y = 0; y < img.rows; y++ ) {
			for ( int x = 0; x < img.cols; x++ ) {
				if ( smoothImg.ptr<Vec3b>( y )[x] == localMaxColor[i] ) {
					tempImg.ptr<Vec3b>( y )[x] = localMaxColor[i];
				}
			}
		}
		imshow( "tempImg", tempImg );
		waitKey( 0 );
	}*/
	cout << localMaxColor.size() << endl;
}

void fineSmoothImg(const Mat &pixelCluster, const vector<Vec3b> &clusterColor, Mat &smoothImg) {

	for (int y = 0; y < smoothImg.rows; y++) {
		for ( int x = 0; x < smoothImg.cols; x++) {

			int clusterIdx = pixelCluster.ptr<int>(y)[x];
			smoothImg.ptr<Vec3b>(y)[x] = clusterColor[clusterIdx];
		}
	}

	const char *fineSmoothImgName = "Fine Smooth Image.png";
	imwrite(fineSmoothImgName, smoothImg);

}

#endif // SMOOTHIMAGE_H

