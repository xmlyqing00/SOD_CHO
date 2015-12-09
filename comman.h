#ifndef COMMAN_H
#define COMMAN_H

//#define SHOW_IMAGE
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
const int CROP_WIDTH = 8;
const int BORDER_WIDTH = 4;
const int PIXEL_CONNECT = 8;
const int MIN_REGION_SIZE = 200; // 200
const int SEGMENT_THRESHOLD = 50; // 80
const int PYRAMID_SIZE = 3;
const int QUANTIZE_LEVEL = 5;
const double BORDER_REGION = 0.1;
const int SALIENCY_THRESHOLD = 70;
const int HIGH_SALIENCY_THRESHOLD = 240;
const int LOW_SALIENCY_THRESHOLD = 20;
const double MIN_REGION_SALIENCY = 0.5;

#define INF 2000000000

#define sqr(_x) ((_x) * (_x))

struct TypeEdge {
	Point u, v;
	double w;
	TypeEdge() {
		u = Point(0, 0);
		v = Point(0, 0);
		w = 0;
	}
	TypeEdge( Point _u, Point _v, double _w ) {
		u = _u;
		v = _v;
		w = _w;
	}
};

struct TypeColorSpace {
	Point pos;
	float color[3];
	TypeColorSpace() {
		pos = Point(0, 0);
		color[0] = color[1] = color[2] = 0;
	}
	TypeColorSpace(Point _pos, Vec3f _color) {
		pos = _pos;
		for (int i = 0; i < 3; i++) color[i] = _color.val[i];
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

double colorDiff(const Vec3f &p0, const Vec3f &p1) {

	double diffRes = 0;
	for (int i = 0; i < 3; i++) {
		diffRes += sqr(p0.val[i] - p1.val[i]);
	}
	return sqrt(diffRes);

}

int getPointDist(const Point &p0, const Point &p1) {

	return cvRound(sqrt((p0.x-p1.x)*(p0.x-p1.x) + (p0.y-p1.y)*(p0.y-p1.y)));
}

void getRegionColor(vector<Vec3f> &regionColor, const int &regionCount,
					const Mat &pixelRegion, const Mat &LABImg) {

	regionColor = vector<Vec3f>(regionCount, 0);
	vector<int> regionSize(regionCount, 0);
	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			regionColor[regionIdx] += LABImg.ptr<Vec3f>(y)[x];
			regionSize[regionIdx]++;
		}
	}
	for (int i = 0; i < regionCount; i++) {
		for (int k = 0; k < 3; k++) regionColor[i].val[k] /= regionSize[i];
	}

}

void getRegionDist(Mat &regionDist, const Mat &pixelRegion, const int regionCount) {

	vector<Point2d> regionCenter(regionCount, Point2d(0, 0));
	vector<int> regionSize(regionCount, 0);
	Size imgSize = pixelRegion.size();

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];
			regionCenter[regionIdx] += Point2d(x,y);
			regionSize[regionIdx]++;
		}
	}

	for (int i = 0; i < regionCount; i++) {
		regionCenter[i].x /= regionSize[i];
		regionCenter[i].y /= regionSize[i];
	}

	vector<Point2d> regionBias(regionCount, Point2d(0, 0));
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			Point2d tmp = Point2d(x,y) - regionCenter[regionIdx];
			regionBias[regionIdx] += Point2d(abs(tmp.x), abs(tmp.y));
		}
	}

	for (int i = 0; i < regionCount; i++) {
		regionCenter[i].x /= imgSize.width;
		regionCenter[i].y /= imgSize.height;
		regionBias[i].x /= regionSize[i] * imgSize.width;
		regionBias[i].y /= regionSize[i] * imgSize.height;
	}

	regionDist = Mat(regionCount, regionCount, CV_64FC1, Scalar(0));
	for (int i = 0; i < regionCount; i++) {
		for (int j = i + 1; j < regionCount; j++) {
			double dx = regionCenter[i].x - regionCenter[j].x;
			double dy = regionCenter[i].y - regionCenter[j].y;
			double centerDist = sqrt(sqr(dx) + sqr(dy));
			double ratioX = abs(dx) / centerDist;
			double ratioY = abs(dy) / centerDist;
			double regionVar_i = regionBias[i].x * ratioX + regionBias[i].y * ratioY;
			double regionVar_j = regionBias[j].x * ratioX + regionBias[j].y * ratioY;
			regionDist.ptr<double>(i)[j] = max(0.0, centerDist - regionVar_i - regionVar_j);
			regionDist.ptr<double>(j)[i] = regionDist.ptr<double>(i)[j];

			//printf("dx %lf dy %lf centerDist %lf ratioX %lf ratioY %lf regionVar_i %lf regionVar_j %lf dist %lf",
				 //  dx, dy, centerDist, ratioX, ratioY, regionVar_i, regionVar_j, regionDist.ptr<double>(i)[j]);
			//cout << endl;
		}
	}
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
#endif
	Mat tmpImg = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH)).clone();
#ifdef SHOW_IMAGE
	imwrite("Input_Image_Resize.png", tmpImg);
#endif
	GaussianBlur(tmpImg, tmpImg, Size(), 0.5, 0, BORDER_REPLICATE);

//	cvtColor(tmpImg, tmpImg, COLOR_BGR2YCrCb);
//	Mat channels[3];
//	split(tmpImg, channels);
//	equalizeHist(channels[0], channels[0]);
//	merge(channels, 3, tmpImg);
//	cvtColor(tmpImg, tmpImg, COLOR_YCrCb2BGR);
//	imshow("equa", tmpImg);

	tmpImg.convertTo(tmpImg, CV_32FC3, 1.0 / 255);
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

void writeRegionImageRepresent(const Mat &pixelRegion, const vector<Vec3f> &regionColor,
							   const char *imgName, const int showFlag, const int writeFlag) {

	Mat regionImg = Mat::zeros( pixelRegion.size(), CV_32FC3 );
	for ( int y = 0; y < pixelRegion.rows; y++ ) {
		for ( int x = 0; x < pixelRegion.cols; x++ ) {
			int idx = pixelRegion.ptr<int>(y)[x];
			regionImg.ptr<Vec3f>(y)[x] = regionColor[idx];
		}
	}
	cvtColor(regionImg, regionImg, COLOR_Lab2BGR);
	regionImg.convertTo(regionImg, CV_8UC3, 255);
	if (showFlag) imshow(imgName, regionImg);
	if (writeFlag) imwrite(imgName, regionImg);
}

void initTransparentImage(Mat &img) {

	int blockSize = 30;
	int halfBlockSize = 15;
	for (int y = 0; y < img.rows; y++) {
		for (int x = 0; x < img.cols; x++) {
			int _x = x % blockSize;
			int _y = y % blockSize;
			if ((_x < halfBlockSize && _y < halfBlockSize) ||
				(_x >= halfBlockSize && _y >=halfBlockSize)) {
				img.ptr<Vec3b>(y)[x] = Vec3b(182, 182, 182);
			} else {
				img.ptr<Vec3b>(y)[x] = Vec3b(153, 153, 153);
			}
		}
	}
}

#endif // COMMAN_H
