#ifndef EVALUATE
#define EVALUATE

#include "comman.h"

void getGroundTruth(map<string,Mat> &binaryMask, const char *dirName) {

	cout << "Ground Truth Input ..." << endl;
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

		string str(testFile->d_name);
		Mat maskMat = imread(inputImgName, 0);
		maskMat = maskMat(Rect(CROP_WIDTH, CROP_WIDTH, maskMat.cols-2*CROP_WIDTH, maskMat.rows-2*CROP_WIDTH));
		str = str.substr(0, str.length()-4);
		binaryMask[str] = maskMat;

	}
}

bool evaluateMap(double &precision, double &recall, const Mat &mask, const Mat &saliencyMap) {

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

	if (tmp_precision < 0.8) return false;
		else return true;

}

void compMaskOthers_1K() {

	FILE *recall_precision_File = fopen("logs/recall_precision.txt", "w");

	int testNum = 0;

	map<string, Mat> binaryMask;
	getGroundTruth(binaryMask, "test/binarymask");

	for (int PARAM1 = 250; PARAM1 > 0; PARAM1 -= 5) {

		printf("%d\t%d", testNum, PARAM1);
		cout << endl;

		char dirName[100] = "Saliency";

		char fileNameFormat[100];
		memset(fileNameFormat, 0, sizeof(fileNameFormat));
		for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
		strcat(fileNameFormat, "/%s");



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

			evaluateMap(precision[methodStr], recall[methodStr], binaryMask[imgId], saliencyMap);

		}

		map<string,double>::iterator it;
		for (it = precision.begin(); it != precision.end(); it++) {

			string methodStr = it->first;
			double tmp_precision = precision[methodStr] / methodCount[methodStr];
			double tmp_recall = recall[methodStr] / methodCount[methodStr];

			fprintf(recall_precision_File, "%.4lf\t%.4lf\t", tmp_recall, tmp_precision);

			printf("%.4lf\t%.4lf\t", tmp_recall, tmp_precision);
			cout << methodStr << endl;

		}
		fprintf(recall_precision_File, "\n");
	}
	fclose(recall_precision_File);

}

void compMaskOthers_10K() {

	FILE *recall_precision_File = fopen("logs/recall_precision_10K.txt", "w");

	int testNum = 0;

	map<string, Mat> binaryMask;
	getGroundTruth(binaryMask, "test/MSRA10K/GT");

	for (int PARAM1 = 250; PARAM1 >= 0; PARAM1 -= 5) {

		printf("%d\t%d", testNum, PARAM1);
		cout << endl;

		char dirName[100] = "test/MSRA10K/Saliency";

		char fileNameFormat[100];
		memset(fileNameFormat, 0, sizeof(fileNameFormat));
		for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
		strcat(fileNameFormat, "/%s");

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

			if (methodStr != "FT" && methodStr != "RC" && methodStr != "SR" && methodStr != "CHO" &&
				methodStr != "LC" && methodStr != "CB" && methodStr != "SEG" && methodStr != "HC") {
				continue;
			}

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

			Mat saliencyObj;
			threshold(saliencyMap, saliencyObj, PARAM1, 255, THRESH_BINARY);

			bool flag = evaluateMap(precision[methodStr], recall[methodStr], binaryMask[imgId], saliencyObj);
//			if (!flag) {
//				imwrite("test/MSRA10K/result/"+imgId+"_CHO.png", saliencyMap);
//				imwrite("test/MSRA10K/result/"+imgId+"_GT.bmp", binaryMask[imgId]);
//				Mat inputImg = imread("test/MSRA10K/input/"+imgId+".jpg");
//				imwrite("test/MSRA10K/result/"+imgId+"_input.jpg", inputImg);
//				imwrite("test/MSRA10K/negative/"+imgId+".jpg", inputImg);
//			}

		}

		map<string,double>::iterator it;
		for (it = precision.begin(); it != precision.end(); it++) {

			string methodStr = it->first;
			double tmp_precision = precision[methodStr] / methodCount[methodStr];
			double tmp_recall = recall[methodStr] / methodCount[methodStr];

			fprintf(recall_precision_File, "%.4lf\t%.4lf\t", tmp_recall, tmp_precision);

			printf("%.4lf\t%.4lf\t", tmp_recall, tmp_precision);
			cout << methodStr << endl;

		}
		fprintf(recall_precision_File, "\n");
	}
	fclose(recall_precision_File);

}

#endif // EVALUATE
