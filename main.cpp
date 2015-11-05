#include "comman.h"
#include "segment.h"
#include "merge.h"
#include "relation.h"
#include "layers.h"
#include "depth.h"
#include "saliency.h"
#include "retarget.h"

int main(int args, char **argv) {

	init();

//	for (int fileIdx = 0; fileIdx < 300; fileIdx++) {

//		char fileName[100];
//		sprintf(fileName, "test//nju400//depth//00%03d_left.jpg", fileIdx);
//		Mat depth_truth = imread(fileName);
//		imshow("depth-truth", depth_truth);

//		sprintf(fileName, "test//nju400//image//00%03d_left.jpg", fileIdx);
//		cout << fileName << endl;

		Mat inputImg, smoothImg, cannyImg;
		//readImage( fileName, inputImg, smoothImg, cannyImg);
		readImage( argv[1], inputImg, smoothImg, cannyImg);

		Mat pixelRegion;
		int regionCount = 0;
		vector<Vec3b> regionColor;
		segmentImage(pixelRegion, regionCount, regionColor, cannyImg, smoothImg );

		int old_regionCount;
		do {
			old_regionCount = regionCount;
			mergeRegion(pixelRegion, regionCount, regionColor);
		} while (old_regionCount != regionCount);

		Mat regionRelation, regionRoute;
		getRegionRelation(regionRelation, regionRoute, pixelRegion, regionCount);

		int *regionLayer = new int[regionCount];
		getRegionLayer(regionLayer, regionRelation, regionRoute, pixelRegion, regionCount);

		Mat depthMap;
		getDepthMap(depthMap, inputImg, pixelRegion, regionLayer, regionCount);

		imshow("input", inputImg);
		imshow("depth", depthMap);
		waitKey(1);

	//	Mat saliencyMap;
	//	getSaliencyMap(saliencyMap, pixelRegion, regionLayer, regionCount);
	//	imshow("saliency", saliencyMap);
		waitKey(0);
		//Mat resizeImg;
		//retargetImage(resizeImg, inputImg, pixelRegion, regionLayer, regionCount);
		delete[] regionLayer;
	//}

	//resize(inputImg, inputImg, Size(), 0.5, 0.5);
	//resize(resizeImg, resizeImg, Size(), 0.5, 0.5);

    return 0;

}
