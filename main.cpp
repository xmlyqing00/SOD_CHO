#include "comman.h"
#include "segmentation.h"
#include "cluster.h"
#include "layers.h"

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
	getClusterRelation(clusterRelation, clusterRoute, pixelCluster, clusterCount, inputImg);

	int *clusterLayer = new int[clusterCount];
	getClusterLayer(clusterLayer, clusterRelation, clusterRoute, clusterCount);

	delete[] clusterLayer;

//	Mat tmp = Mat::zeros(smoothImg.size(), CV_8UC3);
//	for (int i = 0; i < clusterCount; i++) {

//		cout << clusterLayer[i].idx << " " << clusterLayer[i].z_value << endl;

//		for (int y = 0; y < tmp.rows; y++) {
//			for (int x = 0; x < tmp.cols; x++) {
//				if (pixelCluster.ptr<int>(y)[x] == clusterLayer[i].idx) {
//					tmp.ptr<Vec3b>(y)[x] = smoothImg.ptr<Vec3b>(y)[x];
//				}
//			}
//		}
//		imshow("tmp", tmp);
//		waitKey(50);
//	}

//	tmp = Mat::zeros(smoothImg.size(), CV_8UC3);
//	for (int y = 0; y < tmp.rows; y++) {
//		for (int x = 0; x < tmp.cols; x++) {
//			if (pixelCluster.ptr<int>(y)[x] == 247) {
//				tmp.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
//			}
//			if (pixelCluster.ptr<int>(y)[x] == 246) {
//				tmp.ptr<Vec3b>(y)[x] = Vec3b(0, 255, 0);
//			}
//			if (pixelCluster.ptr<int>(y)[x] == 192) {
//				tmp.ptr<Vec3b>(y)[x] = Vec3b(0, 0, 255);
//			}
//		}
//	}
//	//resize(tmp, tmp, Size(), 0.5, 0.5);

//	imshow("ijk", tmp);

    waitKey( 0 );

    return 0;

}
