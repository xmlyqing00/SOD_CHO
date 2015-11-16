#include "comman.h"
#include "segment.h"
#include "merge.h"
#include "graph.h"
#include "saliency.h"
#include "evaluate.h"

int main(int args, char **argv) {

	FILE *testConfig = fopen("test_config.txt", "w");
	int testNum = 0;

//		fprintf(testConfig, "%d\t%.2f\t%.8lf\t%d\t%d\t%.2f\n", testNum,
//				PARAM1_SEGMENT_THRESHOLD, PARAM2_REGION_SIZE, PARAM3_PARAM3_LINE_LENGTH,
//				PARAM4_COLOR_DIFF, PARAM5_CONTOUR_COMPLETION);
//		printf( "%d\t%.2f\t%.8lf\t%d\t%d\t%.2f", testNum,
//						PARAM1_SEGMENT_THRESHOLD, PARAM2_REGION_SIZE, PARAM3_PARAM3_LINE_LENGTH,
//						PARAM4_COLOR_DIFF, PARAM5_CONTOUR_COMPLETION);
//		cout << endl;

		char dirName[100] = "test/binaryimg";
		//sprintf(dirName, "test/MSRA_B/%s", argv[1]);

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

		while ((testFile = readdir(testDir)) != NULL) {

			if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
			cout << testFile->d_name << endl;

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
			double GAMA = 1.1;
			buildRegionGraph(W, D, pyramidRegion, pyramidMap, regionColor, GAMA);
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

			cout << " total " << sum1 / precision.size() << " " << sum2 / recall.size() << endl;

			waitKey(200);
		}

		double sum1 = 0;
		double sum2 = 0;
		for (size_t i = 0; i < precision.size(); i++) {
			sum1 += precision[i];
			sum2 += recall[i];
		}

		fprintf(resultFile, "\ntotal %.5lf %.5lf\n", sum1 / precision.size(), sum2 / recall.size());

		fclose(resultFile);

		testNum++;

	fclose(testConfig);

    return 0;

}
