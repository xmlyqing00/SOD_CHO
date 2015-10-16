#ifndef BASEFUNCTION_H
#define BASEFUNCTION_H

#include "types.h"

bool isOutside( int x, int y, int boundX, int boundY ) {

	if ( x < 0 || y < 0 || x >= boundX || y >= boundY ) return true;
	return false;

}

int colorDiff( Vec3b p0, Vec3b p1 ) {

	int diffRes = 0;
	for ( int i = 0; i < 3; i++ ) diffRes += abs( p0.val[i] - p1.val[i] );
	return diffRes;

}

int getPointDist(Point p0, Point p1) {

	return cvRound(sqrt((p0.x-p1.x)*(p0.x-p1.x) + (p0.y-p1.y)*(p0.y-p1.y)));
}

void getClusterElement( vector<Point> *clusterElement, int *clusterElementCount,
						const Mat &pixelCluster) {

	for ( int y = 0; y < pixelCluster.rows; y++ ) {
		for ( int x = 0; x < pixelCluster.cols; x++ ) {

			int clusterIdx = pixelCluster.ptr<int>( y )[x];
			clusterElementCount[clusterIdx]++;
			clusterElement[clusterIdx].push_back( Point( x, y ) );
		}
	}
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

