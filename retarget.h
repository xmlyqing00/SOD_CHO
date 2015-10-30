#ifndef RETARGET
#define RETARGET

#include "comman.h"

void retargetImage(Mat &retargetImg, const Mat &inputImg, const Mat &pixelRegion,
			   int *regionLayer, const int regionCount) {

    Size inSize = inputImg.size();
    Size outSize = inputImg.size();

    outSize.width = cvRound(outSize.width * RESIZE_RATE);
	retargetImg = Mat(inSize, CV_8UC1, Scalar(0));

    int max_layer = 0;
	for (int i = 0; i < regionCount; i++) {
		max_layer = max(max_layer, regionLayer[i]);
    }
    max_layer++;

	float *layerWeight = new float[max_layer];
	for (int i = 0; i < max_layer; i++) {
		layerWeight[i] = 1 - pow(e, -(float)(i+1) / 2);
		//layerWeight[i] = layerWeight[i] * 1.5 + 0.5;
	}

	for (int y = 0; y < inSize.height; y++) {
		for (int x = 0; x < inSize.width; x++) {
			retargetImg.ptr<uchar>(y)[x] = 255 * layerWeight[regionLayer[pixelRegion.ptr<int>(y)[x]]];
		}
	}
//	float lambda = 0;
//	for (int i = 0; i < regionCount; i++) {
//		lambda += 1 / (2 * layerWeight[regionLayer[i]]);
//	}
//	lambda = (outSize.width - inSize.width) / lambda;

//	int *layerBucket = new int[max_layer];
//	vector<int> *layerComponent = new vector<int>[max_layer];
//	float *layerResize = new float[max_layer];

//	for (int y = 0; y < inSize.height; y++) {

//		for (int i = 0; i < max_layer; i++) {
//			layerBucket[i] = 0;
//			layerComponent[i].clear();
//		}
//		for (int x = 0; x < inSize.width; x++) {
//			int regionIdx = pixelRegion.ptr<int>(y)[x];
//			int layerIdx = regionLayer[regionIdx];
//			layerBucket[layerIdx]++;
//			layerComponent[layerIdx].push_back(x);
//		}

//		for (int i = 0; i < max_layer; i++) {

//			if (layerBucket[i] == 0) {
//				layerResize[i] = 0;
//			} else {
//				float weight = layerWeight[regionLayer[i]];
//				float tmp = 2 * weight * layerBucket[i] - lambda;
//				layerResize[i] = tmp / (2 * weight);
//				if (layerResize[i] < 0) {
//					printf("resize i=%d r=%f", i, layerResize[i]);
//					cout << endl;
//				}
//			}
//		}

//		float tmp = 0;
//		for (int i = 0; i < max_layer; i++) {
//			tmp += layerResize[i];

//		}
//		printf("%f %d", tmp, outSize.width );
//		cout << endl;
//	}

//	delete[] layerComponent;
//	delete[] layerResize;
//	delete[] layerBucket;
	delete[] layerWeight;
}

#endif // RETARGET

