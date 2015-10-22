#include "comman.h"
#include "segmentation.h"
#include "cluster.h"
#include "layers.h"
#include "retarget.h"

int main(int args, char **argv) {

	VideoCapture cap;
	//cap.open(argv[1]);

	Mat inputImg, cannyImg;

	int c = 0;

	//while (readImageFromCap(cap, inputImg, cannyImg)) {

		c++;
		cout << c << endl;

		//if (c < 1000) continue;
		//if (c % 3 != 0) continue;
		readImage( argv[1], inputImg, cannyImg);

		Mat pixelCluster = Mat::zeros( inputImg.size(), CV_32SC1 );
		Mat smoothImg = Mat(inputImg.size(), CV_8UC3);
		int clusterCount = 0;
		vector<Vec3b> clusterColor;
		segmentation(pixelCluster, clusterCount, clusterColor, smoothImg, cannyImg, inputImg );

		//writeClusterImage( clusterCount, pixelCluster, "Merge Cluster Image.png" );
		//imwrite("Smooth Image.png", smoothImg);

		Mat clusterRelation, clusterRoute;
		getClusterRelation(clusterRelation, clusterRoute, pixelCluster, clusterCount);

		int *clusterLayer = new int[clusterCount];
		getClusterLayer(clusterLayer, clusterRelation, clusterRoute, clusterCount);

		Mat resizeImg;
		retargetImage(resizeImg, inputImg, pixelCluster, clusterLayer, clusterCount);
		delete[] clusterLayer;

		//resize(inputImg, inputImg, Size(), 0.5, 0.5);
		//resize(resizeImg, resizeImg, Size(), 0.5, 0.5);
		imshow("input", inputImg);
		imshow("resize", resizeImg);
		waitKey(1);

		//break;
	//}

	waitKey( 0 );

    return 0;

}
