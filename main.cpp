#include "comman.h"
#include "segment.h"
#include "pyramid.h"
#include "saliency.h"
#include "cutobj.h"
#include "evaluate.h"

int main(int args, char **argv) {

#ifdef EVALUATE_MASK
	compMaskOthers();
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

		string imgId = string(testFile->d_name);
		imgId = imgId.substr(0, imgId.length()-4);

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
		//getSaliencyObj(saliencyObj, saliencyMap);
		threshold(saliencyMap, saliencyObj, 250, 255, THRESH_BINARY);

		//int len = strlen(testFile->d_name);
		//string str0 = string(testFile->d_name).substr(0, len-4);
		//char saliencyFileName[100];
		//string str = "Saliency/" + str0 + "_CHO.png";
		//imwrite(str, saliencyMap);
		//str = "Saliency/" + str0 + "_MIX.png";
		//imwrite(str, saliencyMap);

		//getEvaluateResult_MSRA(precision, recall, saliencyMap, userData[string(testFile->d_name)]);
		getEvaluateObj_1000(precision, recall, saliencyObj, binaryMask[imgId]);

#ifdef POS_NEG_RESULT_OUTPUT
		Mat tmpMap;
		Size matSize = LABImg.size();
		Mat resultMap(matSize.height*2, matSize.width*2, CV_8UC3, Scalar(0));

		tmpMap = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH)).clone();
		tmpMap.copyTo(resultMap(Rect(0, 0, matSize.width, matSize.height)));

		cvtColor(binaryMask[imgId], tmpMap, COLOR_GRAY2BGR);
		tmpMap.copyTo(resultMap(Rect(matSize.width, 0, matSize.width, matSize.height)));

		cvtColor(saliencyMap, tmpMap, COLOR_GRAY2BGR);
		tmpMap.copyTo(resultMap(Rect(0, matSize.height, matSize.width, matSize.height)));

		cvtColor(saliencyObj, tmpMap, COLOR_GRAY2BGR);
		tmpMap.copyTo(resultMap(Rect(matSize.width, matSize.height, matSize.width, matSize.height)));


		resize(resultMap, resultMap, Size(), 0.5, 0.5);

		char fileName[100];
		sprintf(fileName, "test/result/%04d_%04d__%s", (int)(precision.back()*10000), (int)(recall.back()*10000), testFile->d_name);
		//sprintf(fileName, "test/result/%s", testFile->d_name);
		imwrite(fileName, resultMap);
		imwrite("Result_Image.png", resultMap);
		imshow("Result_Image.png", resultMap);

		if (precision.back() < 0.8) {
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
