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
#include <opencv2/opencv.hpp>


using namespace std;
using namespace cv;

const int dxdy[8][2] = { { -1, 0 }, { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } };
const int max_que_cap = 50000000;//50000000;
const int colorDiffThreshold = 30;
const int pixelConnect = 8;
const int clusterCoveringThreshold = 100;
const int pixelClusterSizeThreshold = 10;
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
            int mostSimilarColoridx;
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

void fineConvexBoundary( vector<Point> &pixelBound, Point p0, Point p1 ) {

    int dx = p1.x - p0.x;
    int dy = p1.y - p0.y;

    if ( dx == 0 && dy == 0 ) {
        pixelBound.push_back( p0 );
        return;
    }
    size_t last = pixelBound.size();
    if ( abs(dx) <= abs(dy) ) {

        if ( dy < 0 ) {
            swap( p0, p1 );
            dx = -dx;
            dy = -dy;
        }

        double slope = (double)dx / dy;
        for ( int step = 1; step < dy; step++ ) {
            int _y = p0.y + step;
            int _x = p0.x + cvRound( slope * step );
            pixelBound.push_back( Point( _x, _y ) );
        }

    } else {
        if ( dx < 0 ) {
            swap( p0, p1 );
            dx = -dx;
            dy = -dy;
        }

        double slope = (double)dy / dx;
        for ( int step = 1; step < dx; step++ ) {
            int _x = p0.x + step;
            int _y = p0.y + cvRound( slope * step );
            pixelBound.push_back( Point( _x, _y ) );
        }

    }
}

bool pixelBoundCompare( const Point &p0, const Point &p1 ) {
    if ( p0.x == p1.x ) {
        return p0.y < p1.y;
    } else return p0.x < p1.x;
}

bool linkCompare( const typeLink &l0, const typeLink &l1 ) {

    if ( l0.w == l1.w ) {
        return l0.baseArea > l1.baseArea;
    } else return l0.w > l1.w;

}

void coveringDetect( const Mat &pixelCluster, const int clusterCount, const vector<int> &clusterElementCount,
                     const int idx, const vector<Point> &clusterBound, vector<typeLink> &clusterLink ) {

    vector<Point> pixelBound;

    pixelBound.push_back( clusterBound[0] );
    for ( size_t i = 1; i < clusterBound.size(); i++ ) {

        pixelBound.push_back( clusterBound[i] );
        fineConvexBoundary( pixelBound, clusterBound[i - 1], clusterBound[i] );
    }
    fineConvexBoundary( pixelBound, clusterBound[clusterBound.size() - 1], clusterBound[0] );

    sort( pixelBound.begin(), pixelBound.end(), pixelBoundCompare );

    vector<int> clusterCoveringArea( clusterCount + 1, 0 );

    for ( size_t i = 0; i < pixelBound.size(); ) {

        size_t j;
        for ( j = i; j < pixelBound.size(); j++ ) {
            if ( pixelBound[j].x != pixelBound[i].x ) break;
        }
        j--;
        int x = pixelBound[i].x;
        for ( int y = pixelBound[i].y; y <= pixelBound[j].y; y++ ) {

            int clusteridx = pixelCluster.ptr<int>( y )[x];
            clusterCoveringArea[clusteridx]++;
        }
        i = j + 1;
    }
    for ( int i = 1; i <= clusterCount; i++ ) {

        if ( i == idx || clusterCoveringArea[i] == 0 ) continue;

        int w = clusterCoveringArea[i] * clusterCoveringThreshold / clusterElementCount[i];
        if ( w < 5 ) continue;
        clusterLink.push_back( typeLink( idx, i, w, clusterElementCount[idx] ) );
    }
}

void readImg( const char *imgName, Mat &cannyImg, Mat &smoothImg ) {

    Mat inputImg = imread( imgName );
    const char* inputImgName = "Input Image.png";
    //imshow( inputImgName, inputImg );

    const char* cannyImgName = "Canny Image.png";
    Canny( inputImg, cannyImg, 100, 200 );
    imshow( cannyImgName, cannyImg );
    //imwrite( cannyImgName, cannyImg );

    const char* smoothImgName = "Smooth Image.png";
    smoothMat( inputImg, smoothImg );
    imshow( smoothImgName, smoothImg );
    imwrite( smoothImgName, smoothImg );
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
    imshow( clusterImgName, clusterImg );
    imwrite( clusterImgName, clusterImg );

}

void clusterImg( Mat &pixelCluster, int &clusterCount,
                 const Mat &cannyImg, const Mat &smoothImg ) {

    typeQue<Point> &que = *(new typeQue<Point>);

    for ( int y = 0; y < smoothImg.rows; y++ ) {
        for ( int x = 0; x < smoothImg.cols; x++ ) {

            if ( pixelCluster.ptr<int>( y )[x] != 0 ) continue;
            if ( cannyImg.ptr<uchar>( y )[x] == 255 ) continue;

            pixelCluster.ptr<int>( y )[x] = ++clusterCount;

            que.clear();
            que.push( Point( x, y ) );

            vector<Point> pixelBuffer;
            pixelBuffer.push_back( Point( x, y ) );

            while ( !que.empty() ) {

                Point nowP = que.front();
                que.pop();
                Vec3b seedColor = smoothImg.ptr<Vec3b>( y )[x];

                for ( int k = 0; k < pixelConnect; k++ ) {

                    int newX = nowP.x + dxdy[k][0];
                    int newY = nowP.y + dxdy[k][1];

                    if ( isOutside( newX, newY, smoothImg.cols, smoothImg.rows ) ) continue;
                    if ( pixelCluster.ptr<int>( newY )[newX] != 0 ) continue;
                    if ( colorDiff( seedColor, smoothImg.ptr<Vec3b>( newY )[newX] ) > colorDiffThreshold ) continue;

                    pixelCluster.ptr<int>( newY )[newX] = clusterCount;
                    pixelBuffer.push_back( Point( newX, newY ) );

                    if ( cannyImg.ptr<uchar>( newY )[newX] != 255 ) {
                        que.push( Point( newX, newY ) );
                    }
                }
            }
            if ( pixelBuffer.size() < pixelClusterSizeThreshold ) {
                for ( size_t i = 0; i < pixelBuffer.size(); i++ ) {
                    pixelCluster.ptr<int>( pixelBuffer[i].y )[pixelBuffer[i].x] = 0;
                }
                clusterCount--;
            }
        }
    }

    delete &que;

    cout << clusterCount << endl;

    //drawClusterImg( clusterCount, pixelCluster, "Cluster Image.png" );

}

void getClusterLink( const Mat &pixelCluster, const int clusterCount,
                      const Mat &smoothImg, vector<typeLink> &clusterLink ) {

    vector<int> clusterElementCount( clusterCount + 1, 0 );
    vector< vector<Point> > clusterPixel( clusterCount + 1 );
    vector< vector<Point> > clusterBound( clusterCount + 1 );

    for ( int y = 0; y < smoothImg.rows; y++ ) {
        for ( int x = 0; x < smoothImg.cols; x++ ) {

            int clusteridx = pixelCluster.ptr<int>( y )[x];
            if ( clusteridx == 0 ) continue;
            clusterElementCount[clusteridx]++;
            clusterPixel[clusteridx].push_back( Point( x, y ) );
        }
    }


    for ( int i = 1; i <= clusterCount; i++ ) {

        convexHull( clusterPixel[i], clusterBound[i] );
        coveringDetect( pixelCluster, clusterCount, clusterElementCount, i, clusterBound[i], clusterLink );
    }

    sort(clusterLink.begin(), clusterLink.end(), linkCompare);

    cout << clusterLink.size() << endl;
    //for ( size_t i = 0; i < clusterLink.size(); i++ ) {
    //	cout << clusterLink[i].u << " " << clusterLink[i].v << " " << clusterLink[i].w << " " << clusterLink[i].baseArea << endl;
    //}
}

void newTopologyLink( const int u, const int v, vector<int> &topologyHead,
                      vector<typeTopology> &topologyLink ) {

    typeTopology oneTopologyLink( u, v, topologyHead[u] );
    topologyHead[u] = topologyLink.size();
    topologyLink.push_back( oneTopologyLink );

}

void updateAncestorRelation( const int u, const int v, const vector<int> &topologyHead,
                             const vector<typeTopology> &topologyLink, Mat &ancestorRelation ) {

    typeQue<int> &que = *(new typeQue<int>);

    que.push( v );
    while ( !que.empty() ) {

        int idx = que.front();
        que.pop();

        bitwise_or( ancestorRelation.row( u ), ancestorRelation.row( idx ), ancestorRelation.row( idx ) );
        ancestorRelation.ptr<uchar>( idx )[u] = 255;

        for ( int p = topologyHead[idx]; p != -1; p = topologyLink[p].next ) {
            que.push( topologyLink[p].v );
        }
    }

    delete &que;
}

int getGroupHead( int u, vector<int> &clusterGroup ) {

    if ( clusterGroup[u] != u ) {
        clusterGroup[u] = getGroupHead( clusterGroup[u], clusterGroup );
    }
    return clusterGroup[u];
}

void unionClusterGroup( int u, int v, vector<int> &clusterGroup ) {

    int uHead = getGroupHead( u, clusterGroup );
    int vHead = getGroupHead( v, clusterGroup );
    clusterGroup[uHead] = vHead;

}

void coveringAnalysis( const int clusterCount, const vector<typeLink> &clusterLink,
                       vector<int> &topologyLevel, vector<int> &clusterGroup ) {

    vector<bool> linkConflict( clusterLink.size(), false );
    Mat ancestorRelation( clusterCount + 1, clusterCount + 1, CV_8U, Scalar(0) );

    vector<int> topologyHead( clusterCount + 1, -1 );
    vector<typeTopology> topologyLink;
    vector<int> inDegree( clusterCount + 1, 0 );

    for ( int i = 1; i <= clusterCount; i++ ) clusterGroup[i] = i;

    for ( size_t i = 0; i < clusterLink.size(); i++ ) {

        typeLink oneLink = clusterLink[i];

        if ( ancestorRelation.ptr<uchar>( oneLink.u )[oneLink.v] == 255 ) continue;

        unionClusterGroup( oneLink.u, oneLink.v, clusterGroup );

        inDegree[oneLink.v]++;
        newTopologyLink( oneLink.u, oneLink.v, topologyHead, topologyLink );
        updateAncestorRelation( oneLink.u, oneLink.v, topologyHead, topologyLink, ancestorRelation );

    }

    imshow( "Ancestor Relation", ancestorRelation );
    imwrite( "Ancestor Relation.png", ancestorRelation );

    typeQue<int> &que = *(new typeQue<int>);

    for ( int i = 1; i <= clusterCount; i++ ) {
        if ( inDegree[i] == 0 ) que.push( i );
    }
    //cout << que.size() << endl;
    while ( !que.empty() ) {

        int idx = que.front();
        que.pop();

        for ( int p = topologyHead[idx]; p != -1; p = topologyLink[p].next ) {

            int idv = topologyLink[p].v;
            inDegree[idv]--;
            if ( inDegree[idv] == 0 ) {
                topologyLevel[idv] = topologyLevel[idx] + 1;
                que.push( idv );
            }
        }
    }

    for ( int i = 1; i <= clusterCount; i++ ) {
        if ( inDegree[i] != 0 ) cout << "Error";
    }
}

void fineTopologyBoundary( Mat &pixelCluster, const int clusterCount, const vector<int> &topologyLevel ) {

    int maxTopologyLevel = 0;
    for ( int i = 1; i <= clusterCount; i++ ) {
        maxTopologyLevel = max( maxTopologyLevel, topologyLevel[i] );
    }

    int curTopologyLevel = maxTopologyLevel;
    typeQue< typeSearch > &que = *(new typeQue< typeSearch >);

    while ( curTopologyLevel >= 0 ) {

        que.clear();

        for ( int y = 0; y < pixelCluster.rows; y++ ) {
            for ( int x = 0; x < pixelCluster.cols; x++ ) {

                int clusterIdx = pixelCluster.ptr<int>( y )[x];
                if ( clusterIdx == 0 ) continue;
                int level = topologyLevel[pixelCluster.ptr<int>( y )[x]];
                if ( level == curTopologyLevel ) que.push( typeSearch( Point( x, y ), level, 0 ) );
            }
        }

        while ( !que.empty() ) {

            typeSearch idx = que.front();
            que.pop();

            if ( idx.step == 2 && idx.level == curTopologyLevel ) {

                curTopologyLevel--;
                if ( curTopologyLevel >= 0 ) {

                    for ( int y = 0; y < pixelCluster.rows; y++ ) {
                        for ( int x = 0; x < pixelCluster.cols; x++ ) {

                            int level = topologyLevel[pixelCluster.ptr<int>( y )[x]];
                            if ( level == curTopologyLevel ) {
                                que.push( typeSearch( Point( x, y ), level, 0 ) );
                            }
                        }
                    }
                }
            }

            //if ( idx.step > 4 ) continue;

            for ( int k = 0; k < pixelConnect; k++ ) {

                int x = idx.v.x + dxdy[k][0];
                int y = idx.v.y + dxdy[k][1];

                if ( isOutside( x, y, pixelCluster.cols, pixelCluster.rows ) ) continue;
                if ( pixelCluster.ptr<int>( y )[x] == 0 ) {
                    pixelCluster.ptr<int>( y )[x] = pixelCluster.ptr<int>( idx.v.y )[idx.v.x];
                    que.push( typeSearch( Point( x, y ), idx.level, idx.step + 1 ) );
                }
            }
        }

        curTopologyLevel--;

    }
    delete &que;

    drawClusterImg( clusterCount, pixelCluster, "Fine Cluster Image.png" );

    for ( int y = 0; y < pixelCluster.rows; y++ ) {
        for ( int x = 0; x < pixelCluster.cols; x++ ) {

            int clusterIdx = pixelCluster.ptr<int>( y )[x];
            if ( clusterIdx == 0 ) cout << "00000000000000  " << y << " " << x << endl;
        }
    }
}

void getTopologyLevel( const Mat &pixelCluster, const int clusterCount, const vector<int> &topologyLevel,
                       const Mat &smoothImg, vector<int> &clusterGroup ) {

    int maxTopologyLevel = 0;
    for ( int i = 1; i <= clusterCount; i++ ) {
        maxTopologyLevel = max( maxTopologyLevel, topologyLevel[i] );
    }

    vector<int> clusterVisited( clusterCount + 1, 0 );
    /*
    for ( int j = 1; j <= clusterCount; j++ ) {

        if ( clusterVisited[j] != 0 ) continue;
        for ( int k = j; k <= clusterCount; k++ ) {
            if ( getGroupHead( j, clusterGroup ) == getGroupHead( k, clusterGroup ) ) {
                clusterVisited[k] = j;
                cout << k << endl;
            }
        }
        cout << endl;
        */
        for ( int i = 0; i <= maxTopologyLevel; i++ ) {

            //bool end = true;
            Mat topologyImg( smoothImg.size(), CV_8UC3, Scalar( 0 ) );

            for ( int y = 0; y < smoothImg.rows; y++ ) {
                for ( int x = 0; x < smoothImg.cols; x++ ) {

                    int clusterIdx = pixelCluster.ptr<int>( y )[x];
                    //if ( clusterVisited[clusterIdx] != j ) continue;
                    if ( topologyLevel[clusterIdx] == i ) {
                        //end = false;
                        topologyImg.ptr<Vec3b>( y )[x] = smoothImg.ptr<Vec3b>( y )[x];
                    }
                }
            }
            //if ( end ) break;

            char topologyImgName[100];
            sprintf( topologyImgName, "Topology Level %d.png", i );
            //imshow( topologyImgName, topologyImg );
            imwrite( topologyImgName, topologyImg );
            //waitKey();

        }
        //destroyAllWindows();
    //}
}

void retargetImg( const Mat &pixelCluster, const int clusterCount,
                  const vector<int> &topologyLevel, const double retargetRate ) {

    int maxTopologyLevel = 0;
    for ( int i = 1; i <= clusterCount; i++ ) {
        maxTopologyLevel = max( maxTopologyLevel, topologyLevel[i] );
    }

    vector<double> topologyLevelW( maxTopologyLevel + 1 );
    vector<double> scaleParam( maxTopologyLevel + 1 );
    vector<double> offsetParam( maxTopologyLevel + 1 );
    for ( int i = 0; i <= maxTopologyLevel; i++ ) {

        scaleParam[i] = pow( e, -(double)i / 2 );
        topologyLevelW[i] = 1 - scaleParam[i];
        scaleParam[i] = scaleParam[i] * retargetRate;// *pixelCluster.cols;
        offsetParam[i] = 0;

    }

    const char *energyImgName = "Energy Image.png";
    Mat energyImg( pixelCluster.size(), CV_8UC1, Scalar( 0 ) );
    for ( int y = 0; y < pixelCluster.rows; y++ ) {
        for ( int x = 0; x < pixelCluster.cols; x++ ) {
            int level = topologyLevel[pixelCluster.ptr<int>( y )[x]];
            energyImg.ptr<uchar>( y )[x] = cvRound( topologyLevelW[level] * 255 );
        }
    }

    imshow( energyImgName, energyImg );
    imwrite( energyImgName, energyImg );


}

int main() {

    Mat cannyImg, smoothImg;
    readImg( "test//3.png", cannyImg, smoothImg );

    Mat pixelCluster = Mat::zeros( smoothImg.size(), CV_32SC1 );
    int clusterCount = 0;
    clusterImg( pixelCluster, clusterCount, cannyImg, smoothImg );

    vector<typeLink> clusterLink;
    getClusterLink( pixelCluster, clusterCount, smoothImg, clusterLink );

    vector<int> topologyLevel( clusterCount + 1, 0 );
    vector<int> clusterGroup( clusterCount + 1, 0 );
    coveringAnalysis( clusterCount, clusterLink, topologyLevel, clusterGroup );

    fineTopologyBoundary( pixelCluster, clusterCount, topologyLevel );

    getTopologyLevel( pixelCluster, clusterCount, topologyLevel, smoothImg, clusterGroup );

    retargetImg( pixelCluster, clusterCount, topologyLevel, 0.8 );

    waitKey( 0 );

    return 0;

}
