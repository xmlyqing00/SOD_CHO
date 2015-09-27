#include "basefunction.h"
#include "smoothImage.h"
#include "clusterImage.h"
#include "clusterAnalysis.h"

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

void Gaussian(Mat &inputImg) {

	Mat dst0, dst1;
	int b = 1;
	for (int i = 0; i < 10; i++) {
		b = b * 2;
		pyrDown( inputImg, dst0, Size( inputImg.cols/2, inputImg.rows/2 ));
		resize( dst0, dst1, Size( dst0.cols*b, dst0.rows*b ));
		inputImg = dst0;
		imshow("tmp0", dst0);
		imshow("tmp1", dst1);
		waitKey();
	}
}

int main() {

	Mat inputImg, cannyImg, smoothImg;
	readImg( "test//35.png", inputImg, cannyImg);
	Gaussian(inputImg);
	//smoothMat( inputImg, smoothImg );
	smoothImg = inputImg.clone();

    Mat pixelCluster = Mat::zeros( smoothImg.size(), CV_32SC1 );
    int clusterCount = 0;
	vector<Vec3b> clusterColor;
	clusterImg( pixelCluster, clusterCount, clusterColor, cannyImg, smoothImg );

	fineSmoothImg(pixelCluster, clusterColor, smoothImg);

    vector<typeLink> clusterLink;
	clusterAnalysis(pixelCluster, clusterCount, smoothImg, clusterLink );

    vector<int> topologyLevel( clusterCount + 1, 0 );
	int *clusterGroup = new int[clusterCount + 1];
    coveringAnalysis( clusterCount, clusterLink, topologyLevel, clusterGroup );

    getTopologyLevel( pixelCluster, clusterCount, topologyLevel, smoothImg, clusterGroup );
	delete[] clusterGroup;
    retargetImg( pixelCluster, clusterCount, topologyLevel, 0.8 );

    waitKey( 0 );

    return 0;

}
