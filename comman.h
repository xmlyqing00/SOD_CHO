#ifndef COMMAN_H
#define COMMAN_H

//#define SHOW_IMAGE
//#define POS_NEG_RESULT_OUTPUT
//#define EVALUATE_MASK
//#define SAVE_SALIENCY

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
const int PYRAMID_SIZE = 5;
const int QUANTIZE_LEVEL = 5;
const double BORDER_REGION = 0.1;
const int SALIENCY_THRESHOLD = 70;
const int HIGH_SALIENCY_THRESHOLD = 230;
const int LOW_SALIENCY_THRESHOLD = 25;
const double MIN_REGION_SALIENCY = 0.2;

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

bool cmpTypeEdge(const TypeEdge &e1, const TypeEdge &e2);
bool cmpColor0(const TypeColorSpace &c0, const TypeColorSpace &c1);
bool cmpColor1(const TypeColorSpace &c0, const TypeColorSpace &c1);
bool cmpColor2(const TypeColorSpace &c0, const TypeColorSpace &c1);
bool cmpSimilarColor(const pair<int,double> &p0, const pair<int,double> &p1);

bool isOutside( int x, int y, int boundX, int boundY );
int float2sign(const double &f);
int hashVec3b(const Vec3b &v);
void initTransparentImage(Mat &img);
void init();
double colorDiff(const Vec3f &p0, const Vec3f &p1);

int getPointDist(const Point &p0, const Point &p1);
void getRegionColor(vector<Vec3f> &regionColor, const int &regionCount,	const Mat &pixelRegion, const Mat &LABImg);
void getRegionDist(Mat &regionDist, const Mat &pixelRegion, const int regionCount);
void getRegionElement( vector<Point> *regionElement, int *regionElementCount, const Mat &pixelRegion);
int getElementHead( int u, int *head );

void readImage( const char *imgName, Mat &inputImg, Mat &LABImg );
void writeRegionImageRandom( const int regionCount, const Mat &pixelRegion, const char *imgName, const int showFlag, const int writeFlag);
void writeRegionImageRepresent(const Mat &pixelRegion, const vector<Vec3f> &regionColor, const char *imgName, const int showFlag, const int writeFlag);



#endif // COMMAN_H
