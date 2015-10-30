#include "comman.h"
#include "segment.h"
#include "merge.h"
#include "relation.h"
#include "layers.h"
#include "depth.h"
#include "retarget.h"

int main(int args, char **argv) {

	VideoCapture cap;
	//cap.open(argv[1]);

	Mat inputImg, smoothImg, cannyImg;

	//int c = 0;

	//while (readImageFromCap(cap, inputImg, cannyImg)) {

		//c++;
		//cout << c << endl;

		//if (c < 1000) continue;
		//if (c % 3 != 0) continue;
		readImage( argv[1], inputImg, smoothImg, cannyImg);

		Mat pixelRegion;
		int regionCount = 0;
		vector<Vec3b> regionColor;
		segmentImage(pixelRegion, regionCount, regionColor, cannyImg, smoothImg );

		mergeRegion(pixelRegion, regionCount, regionColor);

		Mat regionRelation, regionRoute;
		getRegionRelation(regionRelation, regionRoute, pixelRegion, regionCount);

		int *regionLayer = new int[regionCount];
		getRegionLayer(regionLayer, regionRelation, regionRoute, pixelRegion, regionCount);

		Mat depthMap;
		getDepthMap(depthMap, inputImg, pixelRegion, regionLayer, regionCount);

		//Mat resizeImg;
		//retargetImage(resizeImg, inputImg, pixelRegion, regionLayer, regionCount);
		delete[] regionLayer;

		//resize(inputImg, inputImg, Size(), 0.5, 0.5);
		//resize(resizeImg, resizeImg, Size(), 0.5, 0.5);
		imshow("input", inputImg);
		imshow("depth", depthMap);
		waitKey(1);

		//break;
	//}

	waitKey( 0 );

    return 0;

}
