#ifndef COMMAN_H
#define COMMAN_H

//#define LOG
#define SHOW_IMAGE
#define POS_NEG_RESULT_OUTPUT
//#define EVALUATE_MASK

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
#include <string>
#include <map>
#include <algorithm>
#include <vector>
#include <stack>

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

const Point dxdy[8] = {Point( -1, 0 ), Point( 0, -1 ), Point( 1, 0 ), Point( 0, 1 ),
					   Point( -1, -1), Point( 1, -1 ), Point( -1, 1), Point( 1, 1 )};
const double PI = 3.14159265358;
const double FLOAT_EPS = 1e-8;
const int PIXEL_CONNECT = 8;
const int PYRAMID_SIZE = 5;
const int CROP_WIDTH = 8;
const int MIN_REGION_SIZE = 200; // 200
const int SEGMENT_THRESHOLD = 1000; // 80
const int BORDER_WIDTH = 8;
const double BORDER_REGION = 0.1;
const int QUANTIZE_LEVEL = 5;
const int SALIENCY_THRESHOLD = 70;

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

struct TypeColorSpace {
	Point pos;
	uchar color[3];
	TypeColorSpace() {
		pos = Point(0, 0);
		color[0] = color[1] = color[2] = 0;
	}
	TypeColorSpace(Point _pos, Vec3b _color) {
		pos = _pos;
		for (int i = 0; i < 3; i++) color[i] = _color.val[i];
	}
	TypeColorSpace(Point _pos, Vec3i _color) {
		pos = _pos;
		for (int i = 0; i < 3; i++) color[i] = (uchar)_color.val[i];
	}
};

bool cmpTypeEdge(const TypeEdge &e1, const TypeEdge &e2) {
	return e1.w < e2.w;
}

bool cmpColor0(const TypeColorSpace &c0, const TypeColorSpace &c1) {
	return c0.color[0] < c1.color[0];
}

bool cmpColor1(const TypeColorSpace &c0, const TypeColorSpace &c1) {
	return c0.color[1] < c1.color[1];
}

bool cmpColor2(const TypeColorSpace &c0, const TypeColorSpace &c1) {
	return c0.color[2] < c1.color[2];
}

bool cmpSimilarColor(const pair<int,double> &p0, const pair<int,double> &p1) {
	return p0.second > p1.second;
}

bool isOutside( int x, int y, int boundX, int boundY ) {

	if ( x < 0 || y < 0 || x >= boundX || y >= boundY ) return true;
	return false;

}

int float2sign(const double &f) {
	return (f < -FLOAT_EPS) ? -1 : (f > FLOAT_EPS);
}

int colorDiff(const Vec3b &p0, const Vec3b &p1 ) {

	double diffRes = 1 * sqr( (int)p0.val[0] - (int)p1.val[0] );
	//int diffRes = 0;
	for ( int i = 1; i < 3; i++ ) {
		diffRes += sqr( (int)p0.val[i] - (int)p1.val[i] );
	}
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
#ifdef SHOW_IMAGE
	imwrite("Input_Image.jpg", inputImg);
//	FILE *inputFile = fopen("input.txt", "w");
//	for (int y = 0; y < inputImg.rows; y++) {
//		for (int x = 0; x < inputImg.cols; x++) {
//			for (int k = 0; k < 3; k++) {
//				fprintf(inputFile, "%u\n", inputImg.ptr<Vec3b>(y)[x].val[k]);
//			}
//		}
//	}
//	fclose(inputFile);
#endif
	Mat tmpImg = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH)).clone();
	GaussianBlur(tmpImg, tmpImg, Size(), 1.2, 0, BORDER_REPLICATE);

//	cvtColor(tmpImg, tmpImg, COLOR_BGR2YCrCb);
//	Mat channels[3];
//	split(tmpImg, channels);
//	equalizeHist(channels[0], channels[0]);
//	merge(channels, 3, tmpImg);
//	cvtColor(tmpImg, tmpImg, COLOR_YCrCb2BGR);
//	imshow("equa", tmpImg);

	cvtColor(tmpImg, LABImg, COLOR_BGR2Lab);

}

void writeRegionImageRandom( const int regionCount, const Mat &pixelRegion, const char *imgName,
							 const int showFlag, const int writeFlag) {

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

	if (showFlag) imshow(imgName, regionImg);
	if (writeFlag) imwrite(imgName, regionImg);
}

void writeRegionImageRepresent( const int regionCount, const Mat &pixelRegion, const vector<Vec3b> &regionColor,
								const char *imgName, const int showFlag, const int writeFlag) {

	Mat regionImg = Mat::zeros( pixelRegion.size(), CV_8UC3 );
	for ( int y = 0; y < pixelRegion.rows; y++ ) {
		for ( int x = 0; x < pixelRegion.cols; x++ ) {
			int idx = pixelRegion.ptr<int>(y)[x];
			if (idx != -1) regionImg.ptr<Vec3b>(y)[x] = regionColor[idx];
		}
	}
	cvtColor(regionImg, regionImg, COLOR_Lab2BGR);

	if (showFlag) imshow(imgName, regionImg);
	if (writeFlag) imwrite(imgName, regionImg);
}


#endif // COMMAN_H

