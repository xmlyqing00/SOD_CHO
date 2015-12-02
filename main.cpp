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

	map<string, Mat> binaryMask;
	getUserData_MSRA10K(binaryMask, "test/MSRA10K_GT");

	//map<string, Mat> binaryMask;
	//getUserData_1000(binaryMask, "test/binarymask");

	DIR *testDir = opendir(dirName);
	dirent *testFile;
	int fileNum = 0;

	const int test_num1 = 1;
	const int test_num2 = 4;
	vector<double> precision_param(test_num1 * test_num2, 0);
	vector<double> recall_param(test_num1 * test_num2, 0);
	double PARAM_SET1[test_num1] = {0.5};
	int PARAM_SET2[test_num2] = {60, 70, 80, 90};

	while ((testFile = readdir(testDir)) != NULL) {

		if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
		fileNum++;
		//if (fileNum >= 3) break;
		cout << fileNum << " " << testFile->d_name << endl;

		string imgId = string(testFile->d_name);
		imgId = imgId.substr(0, imgId.length()-4);
		//if (fileNum < 612) continue;
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

		for (int param1 = 0; param1 < test_num1; param1++) {

			Mat saliencyMap;
			getSaliencyMap(saliencyMap, regionCount, pyramidRegion, over_pixelRegion, over_regionCount, LABImg, PARAM_SET1[param1]);

			int len = strlen(testFile->d_name);
			string str = string(testFile->d_name).substr(0, len-4);
			str = "Saliency_10K/" + str + "_CHO.png";
			imwrite(str, saliencyMap);

			continue;

			for (int param2 = 0; param2 < test_num2; param2++) {

				Mat saliencyObj;
				getSaliencyObj(saliencyObj, saliencyMap, LABImg, PARAM_SET2[param2]);
				//threshold(saliencyMap, saliencyObj, 250, 255, THRESH_BINARY);

				//getEvaluateResult_MSRA(precision, recall, saliencyMap, userData[string(testFile->d_name)]);
				double precision, recall;
				getEvaluateObj_1000(precision, recall, saliencyObj, binaryMask[imgId]);
				int paramIdx = param1 * test_num2 + param2;
				precision_param[paramIdx] += precision;
				recall_param[paramIdx] += recall;

				printf("cur %lf %lf total %lf %lf", precision, recall,
					   precision_param[paramIdx] / fileNum, recall_param[paramIdx] / fileNum);
				cout << endl;

#ifdef POS_NEG_RESULT_OUTPUT
			Mat tmpMap;
			Size matSize = LABImg.size();
			Mat resultMap(matSize.height, matSize.width*4, CV_8UC3, Scalar(0));

			tmpMap = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH)).clone();
			tmpMap.copyTo(resultMap(Rect(0, 0, matSize.width, matSize.height)));

			cvtColor(binaryMask[imgId], tmpMap, COLOR_GRAY2BGR);
			//tmpMap.copyTo(resultMap(Rect(matSize.width, 0, matSize.width, matSize.height)));
			tmpMap.copyTo(resultMap(Rect(matSize.width*3, 0, matSize.width, matSize.height)));

			cvtColor(saliencyMap, tmpMap, COLOR_GRAY2BGR);
			//tmpMap.copyTo(resultMap(Rect(0, matSize.height, matSize.width, matSize.height)));
			tmpMap.copyTo(resultMap(Rect(matSize.width*1, 0, matSize.width, matSize.height)));

			cvtColor(saliencyObj, tmpMap, COLOR_GRAY2BGR);
			//tmpMap.copyTo(resultMap(Rect(matSize.width, matSize.height, matSize.width, matSize.height)));
			tmpMap.copyTo(resultMap(Rect(matSize.width*2, 0, matSize.width, matSize.height)));

			resize(resultMap, resultMap, Size(), 0.5, 0.5);

			char fileName[100];
			sprintf(fileName, "test/result/%04d_%04d__%s", (int)(precision*10000), (int)(recall*10000), testFile->d_name);
			//sprintf(fileName, "test/result/%s", testFile->d_name);
			if (precision < 0.8)
				imwrite(fileName, resultMap);
			imwrite("Result_Image.png", resultMap);
			imshow("Result_Image.png", resultMap);

			if (precision < 0.8) {
				char fileName[100];
				sprintf(fileName, "test/negative/%s", testFile->d_name);
				imwrite(fileName, inputImg);
			}
#endif
#ifdef SHOW_IMAGE
			waitKey(0);
#endif
			}
		}
	}

	fileNum--;
	for (int param1 = 0; param1 < test_num1; param1++) {
		for (int param2 = 0; param2 < test_num2; param2++) {

			int paramIdx = param1 * test_num2 + param2;

			double a = precision_param[paramIdx] / fileNum;
			double b = recall_param[paramIdx] / fileNum;
			double c = (1 + 0.3) * a * b / (0.3 * a + b);
			printf("%.2lf %03d : %lf\t%lf\t%lf", PARAM_SET1[param1], PARAM_SET2[param2], a, b, c);
			cout << endl;
		}
	}

	return 0;

}
