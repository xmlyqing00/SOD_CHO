#include "comman.h"
#include "segment.h"
#include "merge.h"
#include "graph.h"
#include "saliency.h"
#include "evaluate.h"

int main(int args, char **argv) {

	FILE *testConfig = fopen("test_config.txt", "a");
	int testNum = 42;

	for (double GAMA = 1.05; GAMA < 1.8; GAMA += 0.05) {
	for (double PARAM1 = 0.0; PARAM1 <= 1; PARAM1 += 0.2) {
	for (double PARAM2 = 0.0; PARAM2 <= 1; PARAM2 += 0.2) {

		fprintf(testConfig, "%d\t%.3lf\t%.3lf\t%.3lf\n", testNum, GAMA, PARAM1, PARAM2);
		printf("%d\t%.3lf\t%.3lf\t%.3lf", testNum, GAMA, PARAM1, PARAM2);
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

			Mat pixelRegion;
			int regionCount = 0;
			vector<Vec3b> regionColor;
			segmentImage(pixelRegion, regionCount, regionColor, LABImg);

			vector< vector<int> > *pyramidMap = new vector< vector<int> >[PYRAMID_SIZE];
			Mat *pyramidRegion = new Mat[PYRAMID_SIZE];
			buildPyramidRegion(pyramidRegion, pyramidMap, pixelRegion, regionCount, LABImg, regionColor);

			Mat W, D;
			buildRegionGraph(W, D, pyramidRegion, pyramidMap, regionColor, GAMA, PARAM1, PARAM2);
			delete[] pyramidMap;
			delete[] pyramidRegion;

			Mat saliencyMap;
			getSaliencyMap(saliencyMap, W, D, pixelRegion);

			//getEvaluateResult_MSRA(precision, recall, saliencyMap, userData[string(testFile->d_name)]);
			getEvaluateResult_1000(precision, recall, saliencyMap, binaryMask, testFile->d_name, resultFile);

			double sum1 = 0;
			double sum2 = 0;
			for (size_t i = 0; i < precision.size(); i++) {
				sum1 += precision[i];
				sum2 += recall[i];
			}

			sum1 = sum1 / precision.size();
			sum2 = sum2 / recall.size();

			cout << " total " << sum1 << " " << sum2 << endl;

#ifdef SHOW_IMAGE
				waitKey(100);
#endif
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
		fprintf(logFile, "%d\t%.3lf\t%.3lf\t%.3lf\n", testNum, GAMA, PARAM1, PARAM2);
		fclose(logFile);

		testNum++;

	}
	}
	}

	fclose(testConfig);

    return 0;

}
