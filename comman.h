#ifndef COMMAN_H
#define COMMAN_H

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <utility>
#include <map>
#include <bitset>
#include <vector>
#include <stack>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

const Point dxdy[8] = { Point( -1, 0 ), Point( 0, -1 ), Point( 1, 0 ), Point( 0, 1 ), Point( -1, -1 ), Point( 1, -1 ), Point( -1, 1 ), Point( 1, 1 ) };
const double e = 2.718281828459;
const double PI = 3.14159265358;
const float FLOAT_EPS = 1e-8;
const int PIXEL_CONNECT = 8;
const float RESIZE_RATE = 0.5;
const int LINE_LENGTH = 10;
const float LINE_ANGLE = -0.9845;
#define INF 2000000000

const int COLOR_DIFF = 20;
const int REGION_SIZE = 25;
const int REGION_CORRELATION = 2;
const float CONTOUR_COMPLETION = 0.01;
const float REGION_CONNECTED = 0.001;
const float REGION_COVERING = 0.01;

struct TypeLink {
	int u, v;
	TypeLink *next;

	TypeLink() {
		u = 0;
		v = 0;
		next = NULL;
	}
	TypeLink( int _u, int _v, TypeLink *_next ) {
		u = _u;
		v = _v;
		next = _next;
	}
};

struct TypeLayer {
	float z_value;
	int idx;
	TypeLayer() {
		z_value = 0;
		idx = 0;
	}
	TypeLayer(float _z_value, int _idx) {
		z_value = _z_value;
		idx = _idx;
	}
};

struct TypeLine {
	Point u, v;
	TypeLine() {
		u = Point(0, 0);
		v = Point(0, 0);
	}
	TypeLine(Point _u, Point _v) {
		u = _u;
		v = _v;
	}
};

class Vec3bCompare{
public:
	bool operator() (const Vec3b &a, const Vec3b &b) {
		for (int i = 0; i < 3; i++) {
			if (a.val[i] < b.val[i]) return true;
			if (a.val[i] > b.val[i]) return false;
		}
		return true;
	}
};

bool cmpPoint( const Point &p0, const Point &p1 ) {
	if ( p0.y == p1.y ) {
		return p0.x < p1.x;
	} else return p0.y < p1.y;
}

bool cmpTypeLayer(const TypeLayer &a, const TypeLayer &b) {
	return a.z_value < b.z_value;
}

bool cmpVec3b(const Vec3b &a, const Vec3b &b) {

	int _a = *(int*)(&a);
	int _b = *(int*)(&b);
	return _a < _b;
}

bool isOutside( int x, int y, int boundX, int boundY ) {

	if ( x < 0 || y < 0 || x >= boundX || y >= boundY ) return true;
	return false;

}

int colorDiff(const Vec3b &p0, const Vec3b &p1 ) {

	int diffRes = 0;
	for ( int i = 0; i < 3; i++ ) diffRes += abs( p0.val[i] - p1.val[i] );
	return diffRes;

}

int getPointDist(const Point &p0, const Point &p1) {

	return cvRound(sqrt((p0.x-p1.x)*(p0.x-p1.x) + (p0.y-p1.y)*(p0.y-p1.y)));
}

void getRegionElement( vector<Point> *regionElement, int *regionElementCount,
					   const Mat &pixelRegion) {

	for ( int y = 0; y < pixelRegion.rows; y++ ) {
		for ( int x = 0; x < pixelRegion.cols; x++ ) {

			int regionIdx = pixelRegion.ptr<int>( y )[x];
			regionElementCount[regionIdx]++;
			regionElement[regionIdx].push_back( Point( x, y ) );
		}
	}
}

int getElementHead( int u, int *head ) {

	if ( head[u] != u ) {
			head[u] = getElementHead( head[u], head );
	}
	return head[u];
}

int hashVec3b(const Vec3b &v) {

	int d = *(int*)(&v) & 0x00ffffff;
	return d;
}

Vec3b deHashVec3b(int d) {

	d = d | 0xff000000;
	Vec3b v = *(Vec3b*)(&d);
	return v;
}

void readImage( const char *imgName, Mat &inputImg, Mat &smoothImg, Mat &cannyImg ) {

    inputImg = imread( imgName );
	imwrite( "Input_Image.png", inputImg );

    Canny( inputImg, cannyImg, 100, 200 );
	imwrite( "Canny_Image.png", cannyImg );

	GaussianBlur(inputImg, smoothImg, Size(3,3), 0.5);
	imwrite("Smooth_Image.png", smoothImg);

}

bool readImageFromCap( VideoCapture &cap, Mat &inputImg, Mat &cannyImg ) {

	//inputImg = imread( imgName );
	cap.read(inputImg);
	if (inputImg.empty()) return false;
	resize(inputImg, inputImg, Size(), 0.5, 0.5);
	//const char* inputImgName = "Input Image.png";
	//imwrite( inputImgName, inputImg );

	Canny( inputImg, cannyImg, 100, 200 );
	//const char* cannyImgName = "Canny Image.png";
	//imwrite( cannyImgName, cannyImg );

	return true;
}

void writeRegionImage( const int regionCount, const Mat &pixelRegion, const char *imgName ) {

    srand( clock() );
    Mat regionImg = Mat::zeros( pixelRegion.size(), CV_8UC3 );
    vector<Vec3b> color;
    color.push_back( Vec3b() );
    for ( int i = 0; i < regionCount; i++ ) {

		uchar t0 = rand() * 255;
		uchar t1 = rand() * 255;
		uchar t2 = rand() * 255;
		color.push_back( Vec3b( t0, t1, t2 ) );
    }

    for ( int y = 0; y < pixelRegion.rows; y++ ) {
		for ( int x = 0; x < pixelRegion.cols; x++ ) {
			regionImg.ptr<Vec3b>(y)[x] = color[pixelRegion.ptr<int>(y)[x]];
		}
    }
    //imshow( regionImgName, regionImg );
	imwrite(imgName, regionImg);

}

//const int CONNECTED_COUNT = 10;
//const float COVERING_RATE = 0.001;
//void getRegionNeighbour(TypeLink **regionNeighbour, const Mat &pixelRegion, const int regionCount) {

//	Mat connectedCount(regionCount, regionCount, CV_32SC1, Scalar(0));

//    for (int y = 0; y < pixelRegion.rows; y++) {
//        for (int x = 0; x < pixelRegion.cols; x++) {

//			int idx0 = pixelRegion.ptr<int>(y)[x];

//            for (int k = 0; k < PIXEL_CONNECT; k++) {

//                Point newP = Point(x,y) + dxdy[k];

//                if (isOutside(newP.x, newP.y, pixelRegion.cols, pixelRegion.rows)) continue;
//				int idx1 = pixelRegion.ptr<int>(newP.y)[newP.x];
//				if (idx1 != idx0) connectedCount.ptr<int>(idx0)[idx1]++;

//            }
//        }
//    }

//	for (int i = 0; i < regionCount; i++) {
//		for (int j = 0; j < regionCount; j++) {

//			if (connectedCount.ptr<int>(i)[j] < CONNECTED_COUNT) continue;

//			TypeLink *oneLink;

//			oneLink = new TypeLink(i, j, regionNeighbour[i]);
//			regionNeighbour[i] = oneLink;

//		}
//	}
//}

#endif // COMMAN_H

