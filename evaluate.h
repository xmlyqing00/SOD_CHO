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
	cvtColor(saliencyMap, cmpMap, CV_GRAY2BGR);
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
		str = str.substr(0, str.length()-4);
		binaryMask[str] = maskMat;

	}
}

void getEvaluateMap_1000(double &precision, double &recall, const Mat &mask, const Mat &saliencyMap) {

	int area_saliency = sum(saliencyMap).val[0] / 255;
	int area_mask = sum(mask).val[0] / 255;
	int area_intersection = sum(saliencyMap & mask).val[0] / 255;

	double tmp_precision, tmp_recall;

	if (area_saliency > 0) {
		tmp_precision = (double)(area_intersection) / area_saliency;
		tmp_recall = (double)(area_intersection) / area_mask;
	} else {
		tmp_precision = 0;
		tmp_recall = 0;
	}

	precision += tmp_precision;
	recall += tmp_recall;

}

void getEvaluateObj_1000(double &precision, double &recall, const Mat &saliencyMap, const Mat &mask) {

	int area_saliency = sum(saliencyMap).val[0] / 255;
	int area_mask = sum(mask).val[0] / 255;
	int area_intersection = sum(saliencyMap & mask).val[0] / 255;

	precision = (double)(area_intersection) / area_saliency;
	recall = (double)(area_intersection) / area_mask;

}

void compMaskOthers() {

	FILE *recall_precision_File = fopen("logs/recall_precision.txt", "w");

	int testNum = 0;

	for (int PARAM1 = 250; PARAM1 > 0; PARAM1 -= 5) {

		printf("%d\t%d", testNum, PARAM1);
		cout << endl;

		char dirName[100] = "Saliency";

		char fileNameFormat[100];
		memset(fileNameFormat, 0, sizeof(fileNameFormat));
		for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
		strcat(fileNameFormat, "/%s");

		map<string, Mat> binaryMask;
		getUserData_1000(binaryMask, "test/binarymask");

		DIR *testDir = opendir(dirName);
		dirent *testFile;

		map<string, double> precision, recall;
		map<string, int> methodCount;

		int c = 0;

		while ((testFile = readdir(testDir)) != NULL) {

			if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
			string str(testFile->d_name);
			if (str.find(".jpg") != string::npos) continue;
			int methodStr_st = str.find_last_of('_');
			int methodStr_ed = str.find_last_of('.');
			string methodStr = str.substr(methodStr_st + 1, methodStr_ed-methodStr_st-1);
			string imgId = str.substr(0, methodStr_st);

			if (methodStr != "CHO") continue;

			if (methodCount.find(methodStr) == methodCount.end()) {
				methodCount[methodStr] = 1;
				precision[methodStr] = 0;
				recall[methodStr] = 0;
			} else {
				methodCount[methodStr]++;
			}

			c++;
			//cout << c << " " << testFile->d_name << endl;

			char inputImgName[100];
			sprintf(inputImgName, fileNameFormat, testFile->d_name);

			Mat inputImg = imread(inputImgName, 0);
			Mat saliencyMap;
			if (methodStr == "CHO") {
				saliencyMap = inputImg;
			} else {
				saliencyMap = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH));
			}
			threshold(saliencyMap, saliencyMap, PARAM1, 255, THRESH_BINARY);

			getEvaluateMap_1000(precision[methodStr], recall[methodStr], binaryMask[imgId], saliencyMap);

		}

		map<string,double>::iterator it;
		for (it = precision.begin(); it != precision.end(); it++) {

			string methodStr = it->first;
			double tmp_precision = precision[methodStr] / methodCount[methodStr];
			double tmp_recall = recall[methodStr] / methodCount[methodStr];

			fprintf(recall_precision_File, "%.4lf\t%.4lf\t", tmp_recall, tmp_precision);

			cout << methodStr << endl;

		}
		fprintf(recall_precision_File, "\n");
	}
	fclose(recall_precision_File);

}

#endif // EVALUATE

