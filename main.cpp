#include "comman.h"
#include "segment.h"
#include "pyramid.h"
#include "saliency.h"
#include "cutobj.h"
#include "evaluate.h"

int main(int args, char **argv) {

#ifdef EVALUATE_MASK
	//compMaskOthers_1K();
	//compMaskOthers_10K();
	compResults_10K();
	return 0;
#endif

	int st_time = clock();

	char dirName[100];
	sprintf(dirName, "test/ECSSD/%s", argv[1]);

	char fileNameFormat[100];
	memset(fileNameFormat, 0, sizeof(fileNameFormat));
	for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
	strcat(fileNameFormat, "/%s");

	const char *GTDir = "test/ECSSD/groundtruth";
	map<string, Mat> binaryMask;
	getGroundTruth(binaryMask, GTDir);

	DIR *testDir = opendir(dirName);
	dirent *testFile;
	int fileNum = 0;

	const int test_num1 = 1;
	const int test_num2 = 1;
	vector<double> precision_param(test_num1 * test_num2, 0);
	vector<double> recall_param(test_num1 * test_num2, 0);
	int PARAM_SET2[test_num2] = {70};

	while ((testFile = readdir(testDir)) != NULL) {

		if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
		fileNum++;
		cout << fileNum << " " << testFile->d_name << endl;

		string imgId = string(testFile->d_name);
		imgId = imgId.substr(0, imgId.length()-4);
		//if (fileNum < 100) continue;
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

#ifdef SHOW_IMAGE
		for (int pyramidIdx = 0; pyramidIdx < PYRAMID_SIZE; pyramidIdx++) {
			vector<Vec3f> regionColor;
			getRegionColor(regionColor, regionCount[pyramidIdx], pyramidRegion[pyramidIdx], LABImg);
			char pyramidRegionName[100];
			sprintf(pyramidRegionName, "debug_output/Pyramid_Color_%d.png", pyramidIdx);
			writeRegionImageRepresent(pyramidRegion[pyramidIdx], regionColor, pyramidRegionName, 0, 1);
		}
#endif

		Mat saliencyMap;
		getSaliencyMap(saliencyMap, regionCount, pyramidRegion, over_pixelRegion, over_regionCount, LABImg);

#ifdef SAVE_SALIENCY
		int len = strlen(testFile->d_name);
		string str = string(testFile->d_name).substr(0, len-4);
		str = "test/MSRA10K/MIX_Cue/" + str + "_CHO.png";
		imwrite(str, saliencyMap);

		continue;

#endif
		for (int param2 = 0; param2 < test_num2; param2++) {

			Mat saliencyObj;
			//getSaliencyObj(saliencyObj, saliencyMap, LABImg, PARAM_SET2[param2]);
			threshold(saliencyMap, saliencyObj, 250, 255, THRESH_BINARY);

			int paramIdx = param2;
			double tmp_precision = precision_param[paramIdx];
			double tmp_recall = recall_param[paramIdx];

			evaluateMap(precision_param[paramIdx], recall_param[paramIdx], binaryMask[imgId], saliencyObj);

			tmp_precision = precision_param[paramIdx] - tmp_precision;
			tmp_recall = recall_param[paramIdx] - tmp_recall;

			printf("cur %lf %lf total %lf %lf",
				   tmp_precision, tmp_recall,
				   precision_param[paramIdx] / fileNum, recall_param[paramIdx] / fileNum);

			cout << endl;
//			waitKey();

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
			sprintf(fileName, "test/result/%04d_%04d__%s", (int)(tmp_precision*10000), (int)(tmp_recall*10000), testFile->d_name);
			//sprintf(fileName, "test/result/%s", testFile->d_name);
			if (tmp_precision < 0.8 || tmp_recall < 0.8)
				imwrite(fileName, resultMap);
			imwrite("debug_output/Result_Image.png", resultMap);
			imshow("Result_Image.png", resultMap);

			if (tmp_precision < 0.8 || tmp_recall < 0.8) {
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

	for (int param1 = 0; param1 < test_num1; param1++) {
		for (int param2 = 0; param2 < test_num2; param2++) {

			int paramIdx = param1 * test_num2 + param2;

			double a = precision_param[paramIdx] / fileNum;
			double b = recall_param[paramIdx] / fileNum;
			double c = (1 + 0.3) * a * b / (0.3 * a + b);
			printf("%03d : %lf\t%lf\t%lf", PARAM_SET2[param2], a, b, c);
			cout << endl;
		}
	}

	cout << (clock() - st_time) / 1000.0 << endl;

	return 0;

}
