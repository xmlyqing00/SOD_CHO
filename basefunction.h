#ifndef BASEFUNCTION_H
#define BASEFUNCTION_H

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
#include <opencv2/opencv.hpp>


using namespace std;
using namespace cv;

const Point dxdy[8] = { Point( -1, 0 ), Point( 0, -1 ), Point( 1, 0 ), Point( 0, 1 ), Point( -1, -1 ), Point( 1, -1 ), Point( -1, 1 ), Point( 1, 1 ) };
const int max_que_cap = 50000000;//50000000;
const int colorDiffThreshold = 15;
const int pixelConnect = 8;
const int clusterCoveringThreshold = 100;
const int pixelClusterSizeThreshold = 25;
const double e = 2.718281828459;

#define INF 2000000000

struct typeLink {
	int u, v, w, baseArea;
	typeLink() {
		u = 0;
		v = 0;
		w = 0;
		baseArea = 0;
	}
	typeLink( int _u, int _v, int _w, int _baseArea ) {
		u = _u;
		v = _v;
		w = _w;
		baseArea = _baseArea;
	}
};

struct typeTopology {
	int u, v, next;
	typeTopology() {
		u = 0;
		v = 0;
		next = -1;
	}
	typeTopology( int _u, int _v, int _next ) {
		u = _u;
		v = _v;
		next = _next;
	}
};

struct typeSearch {
	Point v;
	int level, step;
	typeSearch() {
		v = Point(0, 0);
		level = 0;
		step = 0;
	}
	typeSearch( Point _v, int _level, int _step ) {
		v = _v;
		level = _level;
		step = _step;
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

template<class T>class typeQue {

private:
	T data[max_que_cap];
	int p_front, p_size, p_rear;

public:
	typeQue() {
		p_front = 0;
		p_size = 0;
		p_rear = 0;
	}
	bool empty() {
		if ( p_size == 0 ) return true;
		else return false;
	}
	void push( T x ) {
		data[p_rear++] = x;
		if ( p_rear == max_que_cap ) p_rear = 0;
		p_size++;
		if ( p_size > max_que_cap ) {
			cout << "que out of size !! " << endl;
			return;
		}
	}
	T front() {
		return data[p_front];
	}
	void pop() {
		p_front++;
		if ( p_front == max_que_cap ) p_front = 0;
		p_size--;
	}
	int size() {
		return p_size;
	}
	void clear() {
		p_size = 0;
		p_front = 0;
		p_rear = 0;
	}
	void debug() {
		cout << "que : " << endl;
		for ( int i = p_front; i < p_rear; i++ ) cout << data[i] << endl;
	}
};

bool isOutside( int x, int y, int boundX, int boundY ) {

	if ( x < 0 || y < 0 || x >= boundX || y >= boundY ) return true;
	return false;

}

int colorDiff( Vec3b p0, Vec3b p1 ) {

	int diffRes = 0;
	for ( int i = 0; i < 3; i++ ) diffRes += abs( p0.val[i] - p1.val[i] );
	return diffRes;

}

bool pixelBoundCompare( const Point &p0, const Point &p1 ) {
	if ( p0.x == p1.x ) {
		return p0.y < p1.y;
	} else return p0.x < p1.x;
}

bool linkCompare( const typeLink &l0, const typeLink &l1 ) {

	return (l0.w * l0.baseArea) > (l1.w * l1.baseArea);
	/*
	if ( l0.w == l1.w ) {
		return l0.baseArea > l1.baseArea;
	} else return l0.w > l1.w;
	*/
}

bool colorCompare(const Vec3b &a, const Vec3b &b) {

	int _a = *(int*)(&a);
	int _b = *(int*)(&b);
	return _a < _b;
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

void readImg( const char *imgName, Mat &inputImg, Mat &cannyImg ) {

	inputImg = imread( imgName );
	const char* inputImgName = "Input Image.png";
	//imshow( inputImgName, inputImg );
	imwrite( inputImgName, inputImg );

	Canny( inputImg, cannyImg, 100, 200 );
	const char* cannyImgName = "Canny Image.png";
	//imshow( cannyImgName, cannyImg );
	imwrite( cannyImgName, cannyImg );

}

void drawClusterImg( const int clusterCount, const Mat &pixelCluster, const char *clusterImgName ) {

	srand( clock() );
	Mat clusterImg = Mat::zeros( pixelCluster.size(), CV_8UC3 );
	vector<Vec3b> color;
	color.push_back( Vec3b() );
	for ( int i = 1; i <= clusterCount; i++ ) {

		uchar t0 = rand() * 255;
		uchar t1 = rand() * 255;
		uchar t2 = rand() * 255;
		color.push_back( Vec3b( t0, t1, t2 ) );
	}

	for ( int y = 0; y < pixelCluster.rows; y++ ) {
		for ( int x = 0; x < pixelCluster.cols; x++ ) {
			if ( pixelCluster.ptr<int>( y )[x] == 0 ) continue;
			clusterImg.ptr<Vec3b>( y )[x] = color[pixelCluster.ptr<int>( y )[x]];
		}
	}
	//imshow( clusterImgName, clusterImg );
	imwrite( clusterImgName, clusterImg );

}

int getElementHead( int u, int *head ) {

	if ( head[u] != u ) {
		head[u] = getElementHead( head[u], head );
	}
	return head[u];
}

void unionElement( int u, int v, int *head ) {

	int uHead = getElementHead( u, head );
	int vHead = getElementHead( v, head );
	head[vHead] = uHead;

}

#endif // BASEFUNCTION_H

