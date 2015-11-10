#ifndef COMMAN_H
#define COMMAN_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <utility>
#include <iostream>
#include <map>
#include <algorithm>
#include <vector>
#include <stack>

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

const Point dxdy[8] = {Point( -1, 0 ), Point( 0, -1 ), Point( 1, 0 ), Point( 0, 1 ),
					   Point( -1, -1), Point( 1, -1 ), Point( -1, 1), Point( 1, 1 )};
const double e = 2.718281828459;
const double PI = 3.14159265358;
const float FLOAT_EPS = 1e-8;
const int PIXEL_CONNECT = 8;
const float RESIZE_RATE = 0.5;
const float STRAIGHT_LINE_ANGLE = -0.9845;
const int PYRAMID_SIZE = 7;

const float MIN_REGION_SIZE = 0.001;
const int MIN_LINE_LENGTH = 10;
const int CONVEX_EXTENSION_SIZE = 2;
const float MIN_COMMAN_AREA = 0.5;
const float MIN_REGION_CONNECTED = 0.001;
const float MIN_COVERING = 0.01;

#define INF 2000000000

#define sqr(_x) ((_x) * (_x))

struct TypeEdge {
	Point u, v;
	int w;
	TypeEdge() {
		u = Point(0, 0);
		v = Point(0, 0);
		w = 0;
	}
	TypeEdge( Point _u, Point _v, int _w ) {
		u = _u;
		v = _v;
		w = _w;
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

bool cmpPoint( const Point &p0, const Point &p1 ) {
	if ( p0.y == p1.y ) {
		return p0.x < p1.x;
	} else return p0.y < p1.y;
}

bool cmpTypeEdge(const TypeEdge &e1, const TypeEdge &e2) {
	return e1.w < e2.w;
}

bool isOutside( int x, int y, int boundX, int boundY ) {

	if ( x < 0 || y < 0 || x >= boundX || y >= boundY ) return true;
	return false;

}

int colorDiff(const Vec3b &p0, const Vec3b &p1 ) {

	int diffRes = 0;
	for ( int i = 0; i < 3; i++ ) diffRes += sqr( p0.val[i] - p1.val[i] );
	return cvRound(sqrt(diffRes));

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

void init() {

	pid_t pid;

	pid = fork();
	if (pid == 0) {
		// Child process
		if (execlp("rm", "rm", "-r", "depth", (char*)0) == -1) {
			perror("SHELL");
		}
		exit(0);
	} else {
		// Parent process
		int status;
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	pid = fork();
	if (pid == 0) {
		// Child process
		if (execlp("mkdir", "mkdir", "depth", (char*)0) == -1) {
			perror("SHELL");
		}
		exit(0);
	} else {
		// Parent process
		int status;
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
}

void readImage( const char *imgName, Mat &inputImg, Mat &LABImg ) {

    inputImg = imread( imgName );
	imwrite( "Input_Image.png", inputImg );
	//Size size = inputImg.size();
	//int cut = size.height * 0.025;
	//inputImg = inputImg(Range(cut, size.height-cut), Range(cut, size.width-cut));

	Mat tmpImg;
	GaussianBlur(inputImg, tmpImg, Size(3,3), 0.5);
	cvtColor(tmpImg, LABImg, COLOR_RGB2Lab);
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

void writeRegionImageRandom( const int regionCount, const Mat &pixelRegion, const char *imgName ) {

    srand( clock() );
    Mat regionImg = Mat::zeros( pixelRegion.size(), CV_8UC3 );
    vector<Vec3b> color;
    for ( int i = 0; i < regionCount; i++ ) {

		uchar t0 = rand() * 255;
		uchar t1 = rand() * 255;
		uchar t2 = rand() * 255;
		color.push_back( Vec3b( t0, t1, t2 ) );
    }

    for ( int y = 0; y < pixelRegion.rows; y++ ) {
		for ( int x = 0; x < pixelRegion.cols; x++ ) {
			int idx = pixelRegion.ptr<int>(y)[x];
			if (idx != -1) regionImg.ptr<Vec3b>(y)[x] = color[idx];
		}
    }
//	imwrite(imgName, regionImg);
//	imshow(imgName, regionImg);
//	waitKey(0);
}

void writeRegionImageRepresent( const int regionCount, const Mat &pixelRegion, const vector<Vec3b> &regionColor, const char *imgName ) {

	Mat regionImg = Mat::zeros( pixelRegion.size(), CV_8UC3 );
	for ( int y = 0; y < pixelRegion.rows; y++ ) {
		for ( int x = 0; x < pixelRegion.cols; x++ ) {
			int idx = pixelRegion.ptr<int>(y)[x];
			if (idx != -1) regionImg.ptr<Vec3b>(y)[x] = regionColor[idx];
		}
	}
	cvtColor(regionImg, regionImg, COLOR_Lab2RGB);
	imwrite(imgName, regionImg);
	//imshow(imgName, regionImg);
	//waitKey(0);
}

void getRegionColor(vector<Vec3b> &regionColor, vector<Vec3b> &regionColorVar,
					const int regionCount, const Mat &pixelRegion, const Mat &img) {

	Vec3i *_regionColor = new Vec3i[regionCount];
	Vec3i *_regionColorVar = new Vec3i[regionCount];
	int *regionSize = new int[regionCount];
	for (int i = 0; i < regionCount; i++) {
		_regionColor[i] =  Vec3i(0, 0, 0);
		_regionColorVar[i] =  Vec3i(0, 0, 0);
		regionSize[i] = 0;
	}
	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			regionSize[regionIdx]++;
			_regionColor[regionIdx] += img.ptr<Vec3b>(y)[x];
		}
	}

	regionColor = vector<Vec3b>(regionCount);
	regionColorVar = vector<Vec3b>(regionCount);

	for (int i = 0; i < regionCount; i++) {
		regionColor[i] = _regionColor[i] / regionSize[i];
		regionColorVar[i] = 0;
	}

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			for (int k = 0; k < 3; k++) {
				_regionColorVar[regionIdx].val[k] +=
						sqr(img.ptr<Vec3b>(y)[x].val[k] - regionColor[regionIdx].val[k]);
			}
		}
	}

	for (int i = 0; i < regionCount; i++) {
		for (int k = 0; k < 3; k++) {
			regionColorVar[i].val[k] = sqrt(_regionColorVar[i].val[k] / regionSize[i]);
		}
	}

	delete[] regionSize;
	delete[] _regionColor;
	delete[] _regionColorVar;

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

