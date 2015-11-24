#include "comman.h"
#include "segment.h"
#include "pyramid.h"
#include "saliency.h"
#include "cutobj.h"
#include "evaluate.h"

int main(int args, char **argv) {

#ifdef LOG
	FILE *testConfig = fopen("logs/test_log.txt", "a");
#endif
	int testNum = 0;

	for (int PARAM1 = 115; PARAM1 < 255; PARAM1 += 500) {

#ifdef LOG
		fprintf(testConfig, "%d\tThreshold %d\t", testNum, PARAM1);
#endif
		printf("%d\t%d", testNum, PARAM1);
		cout << endl;

		char dirName[100];
		sprintf(dirName, "test/%s", argv[1]);

		char fileNameFormat[100];
		memset(fileNameFormat, 0, sizeof(fileNameFormat));
		for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
		strcat(fileNameFormat, "/%s");

//		map<string, Rect> userData;
//		getUserData_MSRA(userData, argv[1]);

		map<string, Mat> binaryMask;
		getUserData_1000(binaryMask, "test/binarymask");

		vector<double> precision, recall;
		DIR *testDir = opendir(dirName);
		dirent *testFile;
		int fileNum = 0;

		while ((testFile = readdir(testDir)) != NULL) {

			if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
			fileNum++;
			if (fileNum == 101) break;
			cout << fileNum << " " << testFile->d_name << endl;

			char inputImgName[100];
			sprintf(inputImgName, fileNameFormat, testFile->d_name);

			Mat inputImg, LABImg;
			readImage(inputImgName, inputImg, LABImg);

			Mat W;
			Mat pixelRegion;
			segmentImage(W, pixelRegion, LABImg);

			vector<Mat> pyramidRegion;
			vector<int> regionCount;
			buildPyramidRegion(pyramidRegion, regionCount, pixelRegion, W);

			Mat saliencyMap;
			getSaliencyMap(saliencyMap, regionCount, pyramidRegion, LABImg);

			Mat saliencyObj;
			getSaliencyObj(saliencyObj, saliencyMap);

			//getEvaluateResult_MSRA(precision, recall, saliencyMap, userData[string(testFile->d_name)]);
			getEvaluateResult_1000(precision, recall, saliencyObj, binaryMask, testFile->d_name, PARAM1);

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
			sprintf(fileName, "test/result/%04d_%s", (int)(precision.back()*10000), testFile->d_name);
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
#ifdef SHOW_IMAGE
			waitKey(0);
#endif
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

#ifdef LOG
		fprintf(testConfig, "Precision %.4lf\tRecall %.4lf\n", sum1, sum2);

		char logName[100];
		sprintf(logName, "logs/%04d_%04d_%d.txt", (int)(sum1*10000), (int)(sum2*10000), PARAM1);
		FILE *logFile = fopen(logName, "w");
		fprintf(logFile, "%d\t%d\n", testNum, PARAM1);
		fclose(logFile);
#endif
		testNum++;

	}
#ifdef LOG
	fclose(testConfig);
#endif

	return 0;

}
