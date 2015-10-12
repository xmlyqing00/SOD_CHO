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
const int MAX_QUE_CAP = 50000000;//50000000;
const int COLOR_DIFF = 15;
const int PIXEL_CONNECT = 8;
const int CLUSTER_COVERING = 10;
const int CLUSTER_SIZE = 25;
const double e = 2.718281828459;
const double PI = 3.14159265358;
const double RELATION_PREV = 0.9;

#define INF 2000000000

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
    T data[MAX_QUE_CAP];
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
        if ( p_rear == MAX_QUE_CAP ) p_rear = 0;
		p_size++;
        if ( p_size > MAX_QUE_CAP ) {
			cout << "que out of size !! " << endl;
			return;
		}
	}
	T front() {
		return data[p_front];
	}
	void pop() {
		p_front++;
        if ( p_front == MAX_QUE_CAP ) p_front = 0;
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

void getClusterNeighbour(TypeLink **clusterNeighbour, const Mat &pixelCluster) {

    for (int y = 0; y < pixelCluster.rows; y++) {
        for (int x = 0; x < pixelCluster.cols; x++) {

			int idx0 = pixelCluster.ptr<int>(y)[x];

            for (int k = 0; k < PIXEL_CONNECT; k++) {

                Point newP = Point(x,y) + dxdy[k];

                if (isOutside(newP.x, newP.y, pixelCluster.cols, pixelCluster.rows)) continue;
				int idx1 = pixelCluster.ptr<int>(newP.y)[newP.x];
				if (idx1 != idx0) {

					bool exist = false;
					for (TypeLink *p = clusterNeighbour[idx0]; p != NULL; p = p->next) {
						if (p->v == idx1) {
							exist = true;
							break;
						}
					}

					if (!exist) {

						TypeLink *oneLink;

						oneLink = new TypeLink(idx0, idx1, clusterNeighbour[idx0]);
						clusterNeighbour[idx0] = oneLink;

						oneLink = new TypeLink(idx1, idx0, clusterNeighbour[idx1]);
						clusterNeighbour[idx1] = oneLink;
					}

				}
            }
        }
    }
}

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

int hashVec3b(const Vec3b &v) {

	int d = *(int*)(&v) & 0x00ffffff;
	return d;
}

Vec3b deHashVec3b(int d) {

	d = d | 0xff000000;
	Vec3b v = *(Vec3b*)(&d);
	return v;
}

void readImage( const char *imgName, Mat &inputImg, Mat &cannyImg ) {

    inputImg = imread( imgName );
    const char* inputImgName = "Input Image.png";
    imwrite( inputImgName, inputImg );

    Canny( inputImg, cannyImg, 100, 200 );
    const char* cannyImgName = "Canny Image.png";
    imwrite( cannyImgName, cannyImg );

}

void writeClusterImage( const int clusterCount, const Mat &pixelCluster, const char *clusterImgName ) {

    srand( clock() );
    Mat clusterImg = Mat::zeros( pixelCluster.size(), CV_8UC3 );
    vector<Vec3b> color;
    color.push_back( Vec3b() );
    for ( int i = 0; i < clusterCount; i++ ) {

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

#endif // BASEFUNCTION_H

