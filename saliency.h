#ifndef SALIENCY_H
#define SALIENCY_H

#include "comman.h"

void getSaliencyMap(Mat &saliencyMap, const Mat &pixelRegion, const int *regionLayer, const int regionCount) {

	int *regionElementCount = new int[regionCount];
	vector<Point> *regionElement = new vector<Point>[regionCount];
	for (int i = 0; i < regionCount; i++) {
		regionElementCount[i] = 0;
		regionElement->clear();
	}
	getRegionElement(regionElement, regionElementCount, pixelRegion);

	int max_layer = 0;
	for (int i = 0; i < regionCount; i++) max_layer = max(max_layer, regionLayer[i]);

	Size size = pixelRegion.size();
	int pixelCount = size.width * size.height;

	int gaussian_size = max(10, (int)(size.width * GAUSSIAN_SIZE));
	int half_gaussian_size = gaussian_size / 2;
	Point centerP = Point(half_gaussian_size, half_gaussian_size);
	Mat gaussianMat = Mat(gaussian_size, gaussian_size, CV_32FC1);
	float sigma = 0.1024 * gaussian_size;
	for (int i = 0; i < gaussian_size; i++) {
		for (int j = 0; j < gaussian_size; j++) {

			int dist = getPointDist(Point(i,j), centerP);
			gaussianMat.ptr<float>(i)[j] = pow(e, -dist/sigma);
		}
	}

	float *regionSaliency1 = new float[regionCount];
	int *regionContrastCount = new int[regionCount];
	memset(regionContrastCount, 0, sizeof(int)*regionCount);
	for (int y = 0; y < size.height; y++) {
		for (int x = 0; x < size.width; x++) {

			int regionIdx = pixelRegion.ptr<int>(y)[x];

			for (int _y = y - half_gaussian_size; _y < y + half_gaussian_size; _y++) {

				if (_y < 0) continue;
				if (_y >= size.height) break;
				for (int _x = x - half_gaussian_size; _x < x + half_gaussian_size; _x++) {

					if (_x < 0) continue;
					if (_x >= size.width) break;

					int _regionIdx = pixelRegion.ptr<int>(_y)[_x];
					int d_v = abs(regionLayer[regionIdx] - regionLayer[_regionIdx]);

					int min_x = min(x, _x);
					int min_y = min(y, _y);
					int max_x = max(x, _x);
					int max_y = max(y, _y);
					int delta_x = max_x - min_x + 1;
					int delta_y = max_y - min_y + 1;
					vector<float> d_m_vec;
					for (float k1 = 0.25; k1 <= 0.75; k1 += 0.25) {
						for (float k2 = 0.25; k2 <= 0.75; k2 += 0.25) {
							Point midP(k1 * delta_x + min_x, k2 * delta_y + min_y);
							int rand_regionIdx = pixelRegion.ptr<int>(midP.y)[midP.x];
							float tmp = max(abs(regionLayer[regionIdx] - regionLayer[rand_regionIdx]),
											abs(regionLayer[_regionIdx] - regionLayer[rand_regionIdx]));
							d_m_vec.push_back(tmp);
						}
					}

					sort(d_m_vec.begin(), d_m_vec.end());
					float d_m = (d_m_vec.back() + d_m_vec[d_m_vec.size()-2] + d_m_vec[d_m_vec.size()-3]) / 3;

					int dx = half_gaussian_size + _x - x;
					int dy = half_gaussian_size + _y - y;
					regionSaliency1[regionIdx] += gaussianMat.ptr<float>(dy)[dx] * (d_v + CONTRAST_BETA * d_m);
					regionContrastCount[regionIdx]++;
				}
			}
		}
	}

	float max_saliency1 = 0;
	for (int i = 0; i < regionCount; i++) {
		//cout << regionSaliency1[i] << " " << regionContrastCount[i] << endl;
		regionSaliency1[i] /= regionContrastCount[i];
		max_saliency1 = max(max_saliency1, regionSaliency1[i]);
	}
	for (int i = 0; i < regionCount; i++) {
		regionSaliency1[i] = 0.1 + 0.9 * (regionSaliency1[i] / max_saliency1);
		//cout << regionSaliency1[i] << endl;
	}

	delete[] regionContrastCount;

	int pixelSum = 0;
	int screenLayer = 0;
	for (int i = 0; i <= max_layer; i++) {
		for (int j = 0; j < regionCount; j++) {
			if (regionLayer[j] == i) pixelSum += regionElementCount[j];
		}
		if (pixelSum * 2 >= pixelCount) {
			screenLayer = min(max_layer, i + 1);
			break;
		}
	}

	float *regionDisparity = new float[regionCount];
	for (int i = 0; i < regionCount; i++) {
		if (regionLayer[i] > screenLayer) {
			regionDisparity[i] = (float)(max_layer-regionLayer[i] + 1) / (max_layer - screenLayer + 1);
		} else {
			regionDisparity[i] = (float)(regionLayer[i] + 1) / (screenLayer + 1);
		}
	}

	float *regionDepthLocal = new float[regionCount];
	memset(regionDepthLocal, 0, sizeof(int)*regionCount);
	for (int y = 0; y < size.height; y++) {

		float avgDepth = 0;
		for (int x = 0; x < size.width; x++) {
			avgDepth += regionLayer[pixelRegion.ptr<int>(y)[x]];
		}
		avgDepth /= size.width;

		for (int x = 0; x < size.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			regionDepthLocal[regionIdx] += abs(avgDepth - regionLayer[regionIdx]);
		}
	}

	for (int i = 0; i < regionCount; i++) {
		regionDepthLocal[i] /= regionElementCount[i];
	}

	float *regionSaliency2 = new float[regionCount];
	float max_saliency2 = 0;
	for (int i = 0; i < regionCount; i++) {
		regionSaliency2[i] = regionDepthLocal[i] * regionDisparity[i];
		max_saliency2 = max(max_saliency2, regionSaliency2[i]);

		//printf("%f %f %f\n", regionSaliency2[i], regionDepthLocal[i], regionDisparity[i]);
	}
	//cout << endl;
	for (int i = 0; i < regionCount; i++) {
		regionSaliency2[i] = 0.9 + 0.1 * (regionSaliency2[i] / max_saliency2);
		//printf("%f\n", regionSaliency2[i]);
	}
	//cout << endl;

	float *regionSaliency = new float[regionCount];
	float max_saliency = 0;
	for (int i = 0; i < regionCount; i++) {
		regionSaliency[i] = regionSaliency1[i] * regionSaliency2[i];
		max_saliency = max(max_saliency, regionSaliency[i]);
	}
	for (int i = 0; i < regionCount; i++) {
		regionSaliency[i] /= max_saliency;
		cout << regionSaliency[i] << endl;
	}
	saliencyMap = Mat(size, CV_8UC1, Scalar(0));
	for (int y = 0; y < size.height; y++) {
		for (int x = 0; x < size.width; x++) {
			int regionIdx = pixelRegion.ptr<int>(y)[x];
			saliencyMap.ptr<uchar>(y)[x] = cvRound(255 * regionSaliency[regionIdx]);
		}
	}

	imwrite("Saliency_Map.png", saliencyMap);

	delete[] regionSaliency1;
	delete[] regionSaliency2;
	delete[] regionSaliency;
	delete[] regionDepthLocal;
	delete[] regionDisparity;
	delete[] regionElementCount;
	for (int i = 0; i < regionCount; i++) regionElement->clear();
	delete[] regionElement;

}

#endif // SALIENCY_H

