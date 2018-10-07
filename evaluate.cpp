#include "evaluate.h"

void getGroundTruth(map<string,Mat> &binaryMask, const string & dirName) {

	cout << "Ground Truth Input ..." << endl;
	binaryMask.clear();

	char fileNameFormat[100];
	memset(fileNameFormat, 0, sizeof(fileNameFormat));
	for (size_t i = 0; i < dirName.length(); i++) fileNameFormat[i] = dirName[i];
	strcat(fileNameFormat, "/%s");

	DIR *testDir = opendir(dirName.c_str());
	dirent *testFile;

	while ((testFile = readdir(testDir)) != NULL) {

		if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;

		char inputImgName[100];
		sprintf(inputImgName, fileNameFormat, testFile->d_name);

		string str(testFile->d_name);
		Mat maskMat = imread(inputImgName, 0);
		maskMat = maskMat(Rect(CROP_WIDTH, CROP_WIDTH, maskMat.cols-2*CROP_WIDTH, maskMat.rows-2*CROP_WIDTH));
		binaryMask[str] = maskMat;

	}
}

bool evaluateMap(double &precision, double &recall, double &MAE, const Mat &mask, const Mat &saliencyMap) {

	int area_saliency = sum(saliencyMap).val[0] / 255;
	int area_mask = sum(mask).val[0] / 255;
	int area_intersection = sum(saliencyMap & mask).val[0] / 255;

	MAE =  mean(abs(saliencyMap - mask)).val[0] / 255;

	if (area_saliency > 0) {
		precision = (double)(area_intersection) / area_saliency;
		recall = (double)(area_intersection) / area_mask;
	} else {
		precision = 0;
		recall = 0;
	}

	if (precision < 0.8) return false;
		else return true;

}

void benchMark(char *datasetName) {

	char folderName[100] = "test/";
	strcat(folderName, datasetName);

	char logName[100];
	strcpy(logName, folderName);
	strcat(logName, "/recall_precision.txt");
	FILE *recall_precision_File = fopen(logName, "w");

	int testNum = 0;

	char groundtruthName[100];
	strcpy(groundtruthName, folderName);
	strcat(groundtruthName, "/groundtruth");
	map<string, Mat> binaryMask;
	getGroundTruth(binaryMask, groundtruthName);

	map<int, vector<double> > precision, recall, f_measure;
	map<int, vector<int> > methodCount;

	fprintf(recall_precision_File, "Tf=[");

	for (int Tf = 250; Tf >= 0; Tf -= 5) {

		fprintf(recall_precision_File, "%d, ", Tf);

		printf("%d\t%d", testNum, Tf);
		cout << endl;

		char dirName[100];
		strcpy(dirName, folderName);
		strcat(dirName, "/Saliency");

		DIR *testDir = opendir(dirName);
		dirent *testFile;

		int c = 0;

		for (int i = 0; i < 6; i++) {
			methodCount[i].push_back(0);
			precision[i].push_back(0);
			recall[i].push_back(0);
		}

		while ((testFile = readdir(testDir)) != NULL) {

			if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
			string str(testFile->d_name);
			if (str.find(".jpg") != string::npos) continue;
			int methodStr_st = str.find_last_of('_');
			int methodStr_ed = str.find_last_of('.');
			string methodStr = str.substr(methodStr_st + 1, methodStr_ed-methodStr_st-1);
			string imgId = str.substr(0, methodStr_st);

			int methodId = -1;
			if (methodStr == "CHO") {
				methodId = 0;
			}
			if (methodStr == "RC") {
				methodId = 1;
			}
			if (methodStr == "HC") {
				methodId = 2;
			}
			if (methodStr == "LC") {
				methodId = 3;
			}
			if (methodStr == "SR") {
				methodId = 4;
			}
			if (methodStr == "FT") {
				methodId = 5;
			}
			methodCount[methodId].back()++;

			c++;
			//cout << c << " " << testFile->d_name << endl;

			char fileNameFormat[100];
			strcpy(fileNameFormat, dirName);
			strcat(fileNameFormat, "/%s");
			char inputImgName[100];
			sprintf(inputImgName, fileNameFormat, testFile->d_name);

			Mat inputImg = imread(inputImgName, 0);
			Mat saliencyMap;
			if (methodStr == "CHO") {
				saliencyMap = inputImg;
			} else {
				saliencyMap = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH));
			}
			threshold(saliencyMap, saliencyMap, Tf, 255, THRESH_BINARY);

			double MAE;
			evaluateMap(precision[methodId].back(), recall[methodId].back(), MAE, binaryMask[imgId], saliencyMap);

		}

		for (int k = 0; k < 6; k++) {

			double tmp_precision = precision[k].back() / methodCount[k].back();
			double tmp_recall = recall[k].back() / methodCount[k].back();
			double f = (1 + 0.3) * tmp_precision * tmp_recall / (0.3 * tmp_precision + tmp_recall);
			f_measure[k].push_back(f);

			printf("%.4lf\t%.4lf\t%.4lf ", tmp_recall, tmp_precision, f);
			cout << endl;

		}
	}

	fprintf(recall_precision_File, "];\n");

	for (int k = 0; k < 6; k++) {

		cout << k << endl;

		fprintf(recall_precision_File,"precision=[");
		for (size_t i = 0; i < precision[k].size(); i++) {
			fprintf(recall_precision_File, "%.4lf, ", precision[k][i] / methodCount[k][i]);
		}
		fprintf(recall_precision_File,"];\n");

		fprintf(recall_precision_File,"recall=[");
		for (size_t i = 0; i < recall[k].size(); i++) {
			fprintf(recall_precision_File, "%.4lf, ", recall[k][i] / methodCount[k][i]);
		}
		fprintf(recall_precision_File,"];\n");

		fprintf(recall_precision_File,"f=[");
		for (size_t i = 0; i < f_measure[k].size(); i++) {
			fprintf(recall_precision_File, "%.4lf, ", f_measure[k][i]);
		}
		fprintf(recall_precision_File,"];\n");
		cout << endl;
	}
	fclose(recall_precision_File);

}

void compResults_10K() {

	int threshold_f = 200;

	char dirName[100] = "test/MSRA10K/input";

	char fileNameFormat[100];
	memset(fileNameFormat, 0, sizeof(fileNameFormat));
	for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
	strcat(fileNameFormat, "/%s");

	DIR *testDir = opendir(dirName);
	dirent *testFile;

	int exampleCount = 0;

	const string methodStrArr[8] = {"FT","RC","SR","CHO","LC","CB","SEG","HC"};

	while ((testFile = readdir(testDir)) != NULL) {

		if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
		string str(testFile->d_name);

		string originStr = (string)"test/MSRA10K/input/" + testFile->d_name;
		Mat originImg = imread(originStr);
		originImg = originImg(Rect(CROP_WIDTH, CROP_WIDTH, originImg.cols-2*CROP_WIDTH, originImg.rows-2*CROP_WIDTH));

		int methodStr_st = str.find_last_of('.');
		string imgId = str.substr(0, methodStr_st);

		map<string, double> precision, recall;
		map<string, Mat> saliencyMap;

		Mat binaryMask;

		for (int i = 0; i < 8; i++) {

			string methodStr = methodStrArr[i];

			string imgName = "test/MSRA10K/Saliency/" + imgId + "_" + methodStr + ".png";
			precision[methodStr] = 0;
			recall[methodStr] = 0;

			string maskName = "test/MSRA10K/GT/" + imgId + ".png";
			binaryMask = imread(maskName);
			binaryMask = binaryMask(Rect(CROP_WIDTH, CROP_WIDTH, binaryMask.cols-2*CROP_WIDTH, binaryMask.rows-2*CROP_WIDTH));


			Mat inputImg = imread(imgName);
			Mat saliencyObj;
			if (methodStr == "CHO" || methodStr == "MIX") {
				saliencyMap[methodStr] = inputImg;
			} else {
				saliencyMap[methodStr] = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH));
			}

			threshold(saliencyMap[methodStr], saliencyObj, threshold_f, 255, THRESH_BINARY);

			double MAE;
			evaluateMap(precision[methodStr], recall[methodStr], MAE, binaryMask, saliencyObj);

		}


		if (precision["CHO"] < precision["RC"] + 0.2) continue;

		Size imgSize = originImg.size();
		Mat output(imgSize.height+20, imgSize.width * 10 + 90, CV_8UC3, Scalar(255, 255, 255));

		originImg.copyTo(output(Rect(0, 10, imgSize.width, imgSize.height)));
		saliencyMap["LC"].copyTo(output(Rect(10 + imgSize.width, 10, imgSize.width, imgSize.height)));
		saliencyMap["SR"].copyTo(output(Rect(10*2 + imgSize.width*2, 10, imgSize.width, imgSize.height)));
		saliencyMap["FT"].copyTo(output(Rect(10*3 + imgSize.width*3, 10, imgSize.width, imgSize.height)));
		saliencyMap["SEG"].copyTo(output(Rect(10*4 + imgSize.width*4, 10, imgSize.width, imgSize.height)));
		saliencyMap["CB"].copyTo(output(Rect(10*5 + imgSize.width*5, 10, imgSize.width, imgSize.height)));
		saliencyMap["HC"].copyTo(output(Rect(10*6 + imgSize.width*6, 10, imgSize.width, imgSize.height)));
		saliencyMap["RC"].copyTo(output(Rect(10*7 + imgSize.width*7, 10, imgSize.width, imgSize.height)));
		saliencyMap["CHO"].copyTo(output(Rect(10*8 + imgSize.width*8, 10, imgSize.width, imgSize.height)));
		binaryMask.copyTo(output(Rect(10*9 + imgSize.width*9, 10, imgSize.width, imgSize.height)));

		char outputName[100];
		sprintf(outputName, "test/MSRA10K/examples/%d.png", exampleCount);
		imwrite(outputName, output);

		exampleCount++;
		cout << exampleCount << endl;

	}
}