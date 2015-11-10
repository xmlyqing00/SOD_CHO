#include "comman.h"
#include "segment.h"
#include "merge.h"
//#include "relation.h"
#include "layers.h"
#include "depth.h"
#include "saliency.h"
#include "retarget.h"
#include "graph.h"

int main(int args, char **argv) {

	//init();

//	Mat a = Mat(2,2,CV_64FC1);
//	a.ptr<double>(0)[0] = 4;
//	a.ptr<double>(0)[1] = 7;
//	a.ptr<double>(1)[0] = 7;
//	a.ptr<double>(1)[1] = 4;
//	Mat val, vec;
//	eigen(a, val, vec);
//	cout << val << endl;
//	cout << vec << endl;
//	cout << "=========" << endl;
//	cout << a << endl;
//	cout << vec.col(0) << endl;
//	cout << val.ptr<double>(0)[0] << endl;
//	cout << a * vec.col(0) << endl;
//	cout << val.ptr<double>(0)[0] * vec.col(0) << endl;

	FILE *testConfig = fopen("test_config.txt", "w");
	int testNum = 0;

//		fprintf(testConfig, "%d\t%.2f\t%.8lf\t%d\t%d\t%.2f\n", testNum,
//				PARAM1_SEGMENT_THRESHOLD, PARAM2_REGION_SIZE, PARAM3_PARAM3_LINE_LENGTH,
//				PARAM4_COLOR_DIFF, PARAM5_CONTOUR_COMPLETION);
//		printf( "%d\t%.2f\t%.8lf\t%d\t%d\t%.2f", testNum,
//						PARAM1_SEGMENT_THRESHOLD, PARAM2_REGION_SIZE, PARAM3_PARAM3_LINE_LENGTH,
//						PARAM4_COLOR_DIFF, PARAM5_CONTOUR_COMPLETION);
//		cout << endl;

		char param_dir[100];
		sprintf(param_dir, "param_test/%d/", testNum);
		remove(param_dir);
		mkdir(param_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		sprintf(param_dir, "param_test/%d/seg", testNum);
		remove(param_dir);
		mkdir(param_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		sprintf(param_dir, "param_test/%d/merge", testNum);
		remove(param_dir);
		mkdir(param_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

		DIR *testDir = opendir("test/MSRA_B/3/");
		dirent *testFile;

		while ((testFile = readdir(testDir)) != NULL) {

			cout << testFile->d_name << endl;

			if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;

			char inputImgName[100];
			sprintf(inputImgName, "test/MSRA_B/3/%s", testFile->d_name);

			Mat inputImg, LABImg;
			readImage(inputImgName, inputImg, LABImg);

			Mat pixelRegion;
			int regionCount = 0;
			vector<Vec3b> regionColor;
			vector<Vec3b> regionColorVar;
			segmentImage(pixelRegion, regionCount, regionColor, regionColorVar, LABImg);

			vector< vector<int> > *pyramidMap = new vector< vector<int> >[PYRAMID_SIZE];
			Mat *pyramidRegion = new Mat[PYRAMID_SIZE];
			buildPyramidRegion(pyramidRegion, pyramidMap, pixelRegion, regionCount, LABImg, regionColor, regionColorVar);

			Mat W, D;
			vector<int> frontRegion;
			double GAMA = 0.2;
			buildRegionGraph(W, D, frontRegion, pyramidRegion, pyramidMap, regionColor, GAMA);
			delete[] pyramidMap;
			delete[] pyramidRegion;

			Mat saliencyMap;
			getSaliencyMap(saliencyMap, W, D, frontRegion, pixelRegion);

//			Mat regionRelation, regionRoute;
//			getRegionRelation(regionRelation, regionRoute, pyramidRegion, pyramidMap);

//			int *regionLayer = new int[regionCount];
//			getRegionLayer(regionLayer, regionRelation, regionRoute, pixelRegion, regionCount);

//			Mat depthMap;
//			getDepthMap(depthMap, inputImg, pixelRegion, regionLayer, regionCount);

//			imshow("input", inputImg);
//			imshow("depth", depthMap);
//			waitKey(1);


		//	imshow("saliency", saliencyMap);
			waitKey(0);
			//Mat resizeImg;
			//retargetImage(resizeImg, inputImg, pixelRegion, regionLayer, regionCount);
			//delete[] regionLayer;
		//}

		}

		testNum++;
		//resize(inputImg, inputImg, Size(), 0.5, 0.5);
		//resize(resizeImg, resizeImg, Size(), 0.5, 0.5);

	fclose(testConfig);

    return 0;

}
