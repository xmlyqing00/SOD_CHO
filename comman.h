#ifndef COMMAN_H
#define COMMAN_H

//#define SHOW_IMAGE
//#define POS_NEG_RESULT_OUTPUT
//#define EVALUATE_MASK
#define SAVE_SALIENCY

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
#include <queue>

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

const Point dxdy[8] = {Point( -1, 0 ), Point( 0, -1 ), Point( 1, 0 ), Point( 0, 1 ),
					   Point( -1, -1), Point( 1, -1 ), Point( -1, 1), Point( 1, 1 )};
const double PI = 3.14159265358;
const double FLOAT_EPS = 1e-8;

const int MIN_REGION_SIZE = 200; // 200
const int SEGMENT_THRESHOLD = 50; // 80
const int SIGMA_DIST = 200;
const int SIGMA_COLOR = 10;
const double MERGE_THRESHOLD = 0.02;
const int QUANTIZE_LEVEL = 5;
const int PIXEL_CONNECT = 8;
const int CROP_WIDTH = 8;
const int BORDER_WIDTH = 8;

const double BORDER_REGION = 0.1;
const int SALIENCY_THRESHOLD = 70;
const int HIGH_SALIENCY_THRESHOLD = 230;
const int LOW_SALIENCY_THRESHOLD = 25;
const double MIN_REGION_SALIENCY = 0.2;

#define INF 2000000000
#define FILE_INPUT 0
#define FILE_GROUNDTRUTH 1

#define sqr(_x) ((_x) * (_x))

struct TypeEdgePts {
	Point u, v;
	double w;
	TypeEdgePts() {
		u = Point(0, 0);
		v = Point(0, 0);
		w = 0;
	}
	TypeEdgePts( Point _u, Point _v, double _w ) {
		u = _u;
		v = _v;
		w = _w;
	}
};

struct TypeEdgeNodes {
	int u, v;
	double w;
	TypeEdgeNodes() {
		u = 0;
		v = 0;
		w = 0;
	}
	TypeEdgeNodes(int _u, int _v, double _w ) {
		u = _u;
		v = _v;
		w = _w;
	}
	friend  bool operator < (const TypeEdgeNodes &a, const TypeEdgeNodes &b) {
		return  a.w < b.w;
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

bool cmpTypeEdgePts(const TypeEdgePts &e1, const TypeEdgePts &e2);

bool cmpTypeEdgeNodes(const TypeEdgeNodes &e1, const TypeEdgeNodes &e2);

bool cmpColor0(const TypeColorSpace &c0, const TypeColorSpace &c1);

bool cmpColor1(const TypeColorSpace &c0, const TypeColorSpace &c1);

bool cmpColor2(const TypeColorSpace &c0, const TypeColorSpace &c1);

bool cmpSimilarColor(const pair<int,double> &p0, const pair<int,double> &p1);

bool isOutside(int x, int y, int boundX, int boundY);

bool isOutside(Point p, Size size);

void quantizeColorSpace(Mat &paletteMap, vector<Vec3f> &platte, const Mat &colorImg);

int float2sign(const double &f);

double calcVec3fDiff(const Vec3f &p0, const Vec3f &p1);

void normalizeVecf(vector<float> &vec);

int hashVec3b(const Vec3b &v);

void initTransparentImage(Mat &img);

double ptsDist(const Point &p0, const Point &p1);

int getElementHead( int u, int *head );

void readImage( const char *imgName, Mat &inputImg, Mat &LABImg );

extern Mat paletteDist;

#endif // COMMAN_H
