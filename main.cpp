#include "comman.h"
#include "segment.h"
#include "pyramid.h"
#include "saliency.h"
#include "evaluate.h"

int main(int args, char **argv) {

	FILE *testConfig = fopen("test_config.txt", "a");
	int testNum = 0;

	for (double GAMA = 1.0; GAMA <= 1.0; GAMA += 0.1) {
	for (double PARAM1 = 8; PARAM1 <= 8; PARAM1 += 10) {
	for (int PARAM2 = 15; PARAM2 <= 15; PARAM2 += 100) {

		fprintf(testConfig, "%d\t%.3lf\t%.3lf\t%d\n", testNum, GAMA, PARAM1, PARAM2);
		printf("%d\t%.3lf\t%.3lf\t%d", testNum, GAMA, PARAM1, PARAM2);
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
		FILE *resultFile = fopen("result.txt", "w");
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
			getSaliencyMap(saliencyMap, regionCount, pyramidRegion);

			//getEvaluateResult_MSRA(precision, recall, saliencyMap, userData[string(testFile->d_name)]);
			getEvaluateResult_1000(precision, recall, saliencyMap, binaryMask, testFile->d_name, resultFile);

#ifdef POS_NEG_RESULT_OUTPUR
			Mat tmpMap;
			Size matSize = LABImg.size();
			Mat resultMap(matSize.height, matSize.width*3, CV_8UC3);

			cvtColor(LABImg, tmpMap, COLOR_Lab2RGB);
			tmpMap.copyTo(resultMap(Rect(0, 0, matSize.width, matSize.height)));

			cvtColor(saliencyMap, tmpMap, COLOR_GRAY2RGB);
			tmpMap.copyTo(resultMap(Rect(matSize.width*1, 0, matSize.width, matSize.height)));

			cvtColor(binaryMask[string(testFile->d_name)], tmpMap, COLOR_GRAY2RGB);
			tmpMap.copyTo(resultMap(Rect(matSize.width*2, 0, matSize.width, matSize.height)));

			resize(resultMap, resultMap, Size(), 0.5, 0.5);

			char fileName[100];
			sprintf(fileName, "test/result/%04d_%s", (int)(precision.back()*10000), testFile->d_name);
			imwrite(fileName, resultMap);
			imwrite("Result_Image.png", resultMap);
			imshow("Result_Image.png", resultMap);

			if (precision.back() < 0.7) {
				char fileName[100];
				sprintf(fileName, "test/negative/%d_%s", PARAM2,testFile->d_name);
				imwrite(fileName, inputImg);
			} else {
				char fileName[100];
				sprintf(fileName, "test/positive/%d_%s", PARAM2,testFile->d_name);
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

		fprintf(testConfig, "precision %.3lf\t recall %.3lf\n", sum1, sum2);

		fprintf(resultFile, "\ntotal %.5lf %.5lf\n", sum1, sum2);
		fclose(resultFile);

		char logName[100];
		sprintf(logName, "logs/%d_%d_%d.txt", (int)(sum1*10000), (int)(sum2*10000), testNum);
		FILE *logFile = fopen(logName, "w");
		fprintf(logFile, "%d\t%.3lf\t%.3lf\t%d\n", testNum, GAMA, PARAM1, PARAM2);
		fclose(logFile);

		testNum++;

	}
	}
	}

	fclose(testConfig);

    return 0;

}
