#ifndef DEPTH_H
#define DEPTH_H

#include "comman.h"

void getDepthMap(Mat &depthMap, const Mat &inputImg, const Mat &pixelRegion, const int *regionLayer, const int regionCount) {

	const int bgBlockSize = inputImg.rows / 20;
	const int halfBgBlockSize = bgBlockSize / 2;

	int max_layer = 0;
	for (int i = 0; i < regionCount; i++) {
		max_layer = max(max_layer, regionLayer[i]);
	}
	max_layer++;

	for (int i = 0; i < max_layer; i++) {

		Mat layerMap = Mat(pixelRegion.size(), CV_8UC3);

		for (int y = 0; y < pixelRegion.rows; y++) {
			for (int x = 0; x < pixelRegion.cols; x++) {
				int _x = x % bgBlockSize;
				int _y = y % bgBlockSize;
				if ((_x < halfBgBlockSize && _y < halfBgBlockSize) ||
					(_x >= halfBgBlockSize && _y >=halfBgBlockSize)) {
					layerMap.ptr<Vec3b>(y)[x] = Vec3b(102, 102, 102);
				} else {
					layerMap.ptr<Vec3b>(y)[x] = Vec3b(153, 153, 154);
				}
			}
		}
		for (int y = 0; y < pixelRegion.rows; y++) {
			for (int x = 0; x < pixelRegion.cols; x++) {
				if (regionLayer[pixelRegion.ptr<int>(y)[x]] == i) {
					layerMap.ptr<Vec3b>(y)[x] = inputImg.ptr<Vec3b>(y)[x];
				}
			}
		}

		char imgName[100];
		sprintf(imgName, "depth//Layer_%d.png", i);
		imwrite(imgName, layerMap);

	}

	depthMap = Mat(pixelRegion.size(), CV_8UC1, Scalar(0));

	uchar *layerWeight = new uchar[max_layer];
	for (int i = 0; i < max_layer; i++) {
		layerWeight[i] = cvRound(255 * (1 - pow(e, -(float)(i*10)/(max_layer*2))));
	}

	for (int y = 0; y < depthMap.rows; y++) {
		for (int x = 0; x < depthMap.cols; x++) {
			depthMap.ptr<uchar>(y)[x] = layerWeight[regionLayer[pixelRegion.ptr<int>(y)[x]]];
		}
	}
	delete[] layerWeight;

	imwrite("depth//Depth_Map.png", depthMap);

}

#endif // DEPTH_H

