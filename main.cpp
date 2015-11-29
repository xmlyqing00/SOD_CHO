#include "comman.h"
#include "segment.h"
#include "pyramid.h"
#include "saliency.h"
#include "cutobj.h"
#include "evaluate.h"

int main(int args, char **argv) {

#ifdef EVALUATE_MASK

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
			if (methodCount.find(methodStr) == methodCount.end()) {
				methodCount[methodStr] = 1;
				precision[methodStr] = 0;
				recall[methodStr] = 0;
			} else {
				methodCount[methodStr]++;
			}

			c++;
			cout << c << " " << testFile->d_name << endl;

			char inputImgName[100];
			sprintf(inputImgName, fileNameFormat, testFile->d_name);

			Mat inputImg = imread(inputImgName, 0);
			Mat saliencyMap;
			if (methodStr == "MIX" || methodStr == "CHO") {
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

	return 0;

#endif


	char dirName[100];
	sprintf(dirName, "test/%s", argv[1]);

	char fileNameFormat[100];
	memset(fileNameFormat, 0, sizeof(fileNameFormat));
	for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
	strcat(fileNameFormat, "/%s");

//	map<string, Rect> userData;
//	getUserData_MSRA(userData, argv[1]);

	map<string, Mat> binaryMask;
	getUserData_1000(binaryMask, "test/binarymask");

	vector<double> precision, recall;
	DIR *testDir = opendir(dirName);
	dirent *testFile;
	int fileNum = 0;

	while ((testFile = readdir(testDir)) != NULL) {

		if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
		fileNum++;
		//if (fileNum % 7 != 0) continue;
		if (fileNum >= 1001) break;
		cout << fileNum << " " << testFile->d_name << endl;

		char inputImgName[100];
		sprintf(inputImgName, fileNameFormat, testFile->d_name);

		Mat inputImg, LABImg;
		readImage(inputImgName, inputImg, LABImg);

		Mat W;
		Mat over_pixelRegion;
		int over_regionCount;
		segmentImage(W, over_pixelRegion, over_regionCount, LABImg);

		vector<Mat> pyramidRegion;
		vector<int> regionCount;
		buildPyramidRegion(pyramidRegion, regionCount, over_pixelRegion, W);

		Mat saliencyMap;
		getSaliencyMap(saliencyMap, regionCount, pyramidRegion, over_pixelRegion, over_regionCount, LABImg);

		Mat saliencyObj;
		getSaliencyObj(saliencyObj, saliencyMap);

		//int len = strlen(testFile->d_name);
		//string str0 = string(testFile->d_name).substr(0, len-4);
		//char saliencyFileName[100];
		//string str = "Saliency/" + str0 + "_CHO.png";
		//imwrite(str, saliencyMap);
		//str = "Saliency/" + str0 + "_MIX.png";
		//imwrite(str, saliencyMap);

		//getEvaluateResult_MSRA(precision, recall, saliencyMap, userData[string(testFile->d_name)]);
		getEvaluateObj_1000(precision, recall, saliencyObj, binaryMask, testFile->d_name);

#ifdef POS_NEG_RESULT_OUTPUT
		Mat tmpMap;
		Size matSize = LABImg.size();
		Mat resultMap(matSize.height*2, matSize.width*2, CV_8UC3, Scalar(0));

		tmpMap = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH)).clone();
		tmpMap.copyTo(resultMap(Rect(0, 0, matSize.width, matSize.height)));

		cvtColor(binaryMask[string(testFile->d_name)], tmpMap, COLOR_GRAY2RGB);
		tmpMap.copyTo(resultMap(Rect(matSize.width, 0, matSize.width, matSize.height)));

		cvtColor(saliencyMap, tmpMap, COLOR_GRAY2RGB);
		tmpMap.copyTo(resultMap(Rect(0, matSize.height, matSize.width, matSize.height)));

		cvtColor(saliencyObj, tmpMap, COLOR_GRAY2RGB);
		tmpMap.copyTo(resultMap(Rect(matSize.width, matSize.height, matSize.width, matSize.height)));


		resize(resultMap, resultMap, Size(), 0.5, 0.5);

		char fileName[100];
		sprintf(fileName, "test/result/%04d_%04d__%s", (int)(precision.back()*10000), (int)(recall.back()*10000), testFile->d_name);
		//sprintf(fileName, "test/result/%s", testFile->d_name);
		imwrite(fileName, resultMap);
		imwrite("Result_Image.png", resultMap);
		imshow("Result_Image.png", resultMap);

		if (precision.back() < 0.8 || recall.back() < 0.8) {
			char fileName[100];
			sprintf(fileName, "test/negative/%s", testFile->d_name);
			imwrite(fileName, inputImg);
		} else {
			char fileName[100];
			sprintf(fileName, "test/positive/%s", testFile->d_name);
			imwrite(fileName, inputImg);
		}
#endif

#ifdef SHOW_IMAGE
		waitKey(0);
#endif

		double sum1 = 0;
		double sum2 = 0;
		for (size_t i = 0; i < precision.size(); i++) {
			sum1 += precision[i];
			sum2 += recall[i];
		}

		sum1 = sum1 / precision.size();
		sum2 = sum2 / recall.size();

		cout << " total " << sum1 << " " << sum2 << endl;

	}

	double sum1 = 0;
	double sum2 = 0;
	for (size_t i = 0; i < precision.size(); i++) {
		sum1 += precision[i];
		sum2 += recall[i];
	}
	sum1 = sum1 / precision.size();
	sum2 = sum2 / recall.size();

	return 0;

}
