#ifndef EVALUATE
#define EVALUATE

#include "comman.h"

void getUserData_MSRA(map<string,Rect> &userData, const char *dirName) {

	const int buffer_size = 500;
	char userDataFileName[buffer_size];
	sprintf(userDataFileName, "test/MSRA_B/UserData/%s_data.txt", dirName);
	FILE *userDataFile = fopen(userDataFileName, "r");

	char str[buffer_size];
	fgets(str, buffer_size, userDataFile); // unused
	fgets(str, buffer_size, userDataFile); // unused
	for (int i = 0; i < 500; i++) {

		fgets(str, buffer_size, userDataFile);
		int len = strlen(str);
		string fileName = "";
		for (int j = 2; j < len-2; j++) fileName += str[j];

		fgets(str, buffer_size, userDataFile); // unused

		int sum[4] = {0, 0, 0, 0};
		for (int j = 0; j < 9; j++) {

			int tmp[4];
			fscanf(userDataFile, "%d %d %d %d;", &tmp[0],&tmp[1],&tmp[2],&tmp[3]);
			for (int k = 0; k < 4; k++) sum[k] += tmp[k];
		}

		Rect avgRect;
		avgRect.x = sum[0] / 9 - CROP_WIDTH;
		avgRect.y = sum[1] / 9 - CROP_WIDTH;
		avgRect.width = sum[2] / 9 - avgRect.x;
		avgRect.height = sum[3] / 9 - avgRect.y;

		userData[fileName] = avgRect;

		// read two '\n'
		fgets(str, buffer_size, userDataFile); // unused
		fgets(str, buffer_size, userDataFile); // unused

	}
	fclose(userDataFile);
}

void getEvaluateResult_MSRA(vector<double> &precision, vector<double> &recall, const Mat &saliencyMap,
					   const Rect &groundtruth) {

//	Mat saliencySum(saliencyMap.size(), CV_32SC1, Scalar(0));

//	saliencySum.ptr<int>(0)[0] = saliencyMap.ptr<uchar>(0)[0] == 255 ? 1 : 0;
//	for (int x = 1; x < saliencySum.cols; x++) {
//		saliencySum.ptr<int>(0)[x] = saliencySum.ptr<int>(0)[x-1] + saliencyMap.ptr<uchar>(0)[x] == 255 ? 1 : 0;
//	}
//	for (int y = 1; y < saliencySum.rows; y++) {
//		saliencySum.ptr<int>(y)[0] = saliencySum.ptr<int>(y-1)[0] + saliencyMap.ptr<uchar>(y)[0] == 255 ? 1 : 0;
//	}
//	for (int y = 1; y < saliencySum.rows; y++) {
//		for (int x = 1; x < saliencySum.cols; x++) {
//			int tmp1 = saliencySum.ptr<int>(y-1)[x-1];
//			int tmp2 = saliencySum.ptr<int>(y)[x-1];
//			int tmp3 = saliencySum.ptr<int>(y-1)[x];
//			int tmp4 = saliencyMap.ptr<uchar>(y)[x] == 255 ? 1 : 0;
//			saliencySum.ptr<int>(y)[x] = tmp2 + tmp3 + tmp4 - tmp1;
//		}
//	}

//	int criteria = 0.95 * saliencySum.ptr<int>(saliencySum.rows-1)[saliencySum.cols-1];
//	int min_width = INF;
//	int min_height = INF;
//	Rect minRect;
//	for (int top = 0; top < saliencyMap.rows; top++) {
//		for (int bottom = top; bottom < saliencyMap.rows; bottom++) {
//			for (int left = 0; left < saliencyMap.cols; left++) {
//				for (int right = left; right < saliencyMap.cols; right++) {

//					int tmp = saliencySum.ptr<int>(bottom)[right] - saliencySum.ptr<int>(top)[left];
//					if (tmp > criteria) {

//						int width = right - left;
//						int height = top - bottom;
//						if (min_width >= width && min_height >= height) {

//							min_width = width;
//							min_height = height;
//							minRect = Rect(left, top, width, height);
//						}
//					}
//				}
//			}
//		}
//	}

	vector<Point> pts;
	for (int y = 0; y < saliencyMap.rows; y++) {
		for (int x = 0; x < saliencyMap.cols; x++) {
			if (saliencyMap.ptr<uchar>(y)[x] == 255) pts.push_back(Point(x,y));
		}
	}
	Rect minRect = boundingRect(pts);

	Mat cmpMap;
	cvtColor(saliencyMap, cmpMap, CV_GRAY2RGB);
	rectangle(cmpMap, groundtruth, Scalar(255, 0, 0));
	rectangle(cmpMap, minRect, Scalar(0, 0, 255));

	imshow("Compare", cmpMap);

}

void getUserData_1000(map<string,Mat> &binaryMask, const char *dirName) {

	binaryMask.clear();

	char fileNameFormat[100];
	memset(fileNameFormat, 0, sizeof(fileNameFormat));
	for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
	strcat(fileNameFormat, "/%s");

	DIR *testDir = opendir(dirName);
	dirent *testFile;

	while ((testFile = readdir(testDir)) != NULL) {

		if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;

		char inputImgName[100];
		sprintf(inputImgName, fileNameFormat, testFile->d_name);

		int len = strlen(testFile->d_name);
		string str = "";
		for (int i = 0; i < len - 3; i++) str += testFile->d_name[i];
		str += "jpg";
		Mat maskMat = imread(inputImgName, 0);
		maskMat = maskMat(Rect(CROP_WIDTH, CROP_WIDTH, maskMat.cols-2*CROP_WIDTH, maskMat.rows-2*CROP_WIDTH));
		binaryMask[str] = maskMat;

	}
}

void getEvaluateResult_1000(vector<double> &precision, vector<double> &recall,
							const Mat &saliencyMap, map<string,Mat> &binaryMask,
							const char *imgName, FILE *resultFile) {

	Mat mask = binaryMask[string(imgName)];
	int area_saliency = sum(saliencyMap).val[0] / 255;
	int area_mask = sum(mask).val[0] / 255;
	int area_intersection = sum(saliencyMap & mask).val[0] / 255;

	precision.push_back(double(area_intersection) / area_mask);
	recall.push_back((double)(area_intersection) / area_saliency);

	cout << "current " << precision.back() << " " << recall.back();
	fprintf(resultFile, "%s %.5lf %.5lf\n", imgName, precision.back(), recall.back());
	imshow("Binary_Mask", mask);
}

#endif // EVALUATE

