#include "comman.h"
#include "segmentation.h"
#include "cluster.h"

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

int main(int args, char **argv) {

	Mat inputImg, cannyImg;
	readImage( argv[1], inputImg, cannyImg);

	Mat pixelCluster = Mat::zeros( inputImg.size(), CV_32SC1 );
	Mat smoothImg = Mat(inputImg.size(), CV_8UC3);
    int clusterCount = 0;
	vector<Vec3b> clusterColor;
	segmentation(pixelCluster, clusterCount, clusterColor, smoothImg, cannyImg, inputImg );

	writeClusterImage( clusterCount, pixelCluster, "Merge Cluster Image.png" );
	imwrite("Smooth Image.png", smoothImg);

	Mat clusterRelation, clusterRoute;
	getClusterRelation(clusterRelation, clusterRoute, pixelCluster, clusterCount);

	TypeLayer *clusterLayer = new TypeLayer[clusterCount];
	getClusterLayer(clusterLayer, clusterCount, clusterRelation, clusterRoute);

	Mat tmp = Mat::zeros(smoothImg.size(), CV_8UC3);
	for (int i = 0; i < clusterCount; i++) {
		cout << clusterLayer[i].idx << " " << clusterLayer[i].z_value << endl;

		for (int y = 0; y < tmp.rows; y++) {
			for (int x = 0; x < tmp.cols; x++) {
				if (pixelCluster.ptr<int>(y)[x] == clusterLayer[i].idx) {
					tmp.ptr<Vec3b>(y)[x] = smoothImg.ptr<Vec3b>(y)[x];
				}
			}
		}
		imshow("tmp", tmp);
		waitKey(0);
	}
//    vector<int> topologyLevel( clusterCount + 1, 0 );
//	int *clusterGroup = new int[clusterCount + 1];
//    coveringAnalysis( clusterCount, clusterLink, topologyLevel, clusterGroup );

//    getTopologyLevel( pixelCluster, clusterCount, topologyLevel, smoothImg, clusterGroup );
//	delete[] clusterGroup;
//    retargetImg( pixelCluster, clusterCount, topologyLevel, 0.8 );

    waitKey( 0 );

    return 0;

}
