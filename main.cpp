#include "comman.h"
#include "type_region.h"
#include "type_file.h"
#include "multilayer.h"
#include "saliency.h"
#include "evaluate.h"
//#include "cutobj.h"

int main(int args, char **argv) {

#ifdef EVALUATE_MASK
	//compMaskOthers_1K();
	//compMaskOthers_10K();
	compResults_10K();
	return 0;
#endif

	int st_time = clock();

	TypeFile fileSet(argv[1]);
	map<string, Mat> binaryMask;
	getGroundTruth(binaryMask, fileSet);

	char *inputFileName;
	double avgPrecision = 0, avgRecall = 0;

	while ((inputFileName = fileSet.getNextFileName(FILE_INPUT)) != NULL) {

		Mat inputImg, LABImg;
		readImage(inputFileName, inputImg, LABImg);

		Mat paletteMap;
		vector<Vec3f> palette;
		quantizeColorSpace(paletteMap, palette, LABImg);

		vector<TypeRegionSet> multiLayerModel;
		buildMultiLayerModel(multiLayerModel, paletteMap, LABImg);

		Mat saliencyMap;
		getSaliencyMap(saliencyMap, multiLayerModel, paletteMap);

		Mat salientRegions;
		saliencyMap.convertTo(salientRegions, CV_8UC1, 255);
		threshold(salientRegions, salientRegions, 250, 255, THRESH_BINARY);

		string imgId(inputFileName);
		int str_st = imgId.find_last_of('/');
		int str_ed = imgId.find_last_of('.');
		imgId = imgId.substr(str_st + 1, str_ed - str_st - 1);

		double precision, recall;
		evaluateMap(precision, recall, binaryMask[imgId], salientRegions);
		avgPrecision += precision;
		avgRecall += recall;
		printf("%d %.5lf %.5lf %.5lf %.5lf", fileSet.count[FILE_INPUT], precision, recall, avgPrecision/fileSet.count[FILE_INPUT], avgRecall/fileSet.count[FILE_INPUT]);
		cout << endl;

		string saliencyMapName = "test/MSRA10K/Saliency_CHO/" + imgId + "_CHO.png";
		imwrite(saliencyMapName, saliencyMap);

		//evaluateMap(precision_param[paramIdx], recall_param[paramIdx], binaryMask[imgId], saliencyObj);

		continue;
//		for (int param2 = 0; param2 < test_num2; param2++) {

//			Mat saliencyObj;
//			//getSaliencyObj(saliencyObj, saliencyMap, LABImg, PARAM_SET2[param2]);
//			threshold(saliencyMap, saliencyObj, 250, 255, THRESH_BINARY);

//			int paramIdx = param2;
//			double tmp_precision = precision_param[paramIdx];
//			double tmp_recall = recall_param[paramIdx];

//			evaluateMap(precision_param[paramIdx], recall_param[paramIdx], binaryMask[imgId], saliencyObj);

//			tmp_precision = precision_param[paramIdx] - tmp_precision;
//			tmp_recall = recall_param[paramIdx] - tmp_recall;

//			printf("cur %lf %lf total %lf %lf",
//				   tmp_precision, tmp_recall,
//				   precision_param[paramIdx] / fileNum, recall_param[paramIdx] / fileNum);

//			cout << endl;
////			waitKey();

//#ifdef POS_NEG_RESULT_OUTPUT
//			Mat tmpMap;
//			Size matSize = LABImg.size();
//			Mat resultMap(matSize.height, matSize.width*4, CV_8UC3, Scalar(0));

//			tmpMap = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH)).clone();
//			tmpMap.copyTo(resultMap(Rect(0, 0, matSize.width, matSize.height)));

//			cvtColor(binaryMask[imgId], tmpMap, COLOR_GRAY2BGR);
//			//tmpMap.copyTo(resultMap(Rect(matSize.width, 0, matSize.width, matSize.height)));
//			tmpMap.copyTo(resultMap(Rect(matSize.width*3, 0, matSize.width, matSize.height)));

//			cvtColor(saliencyMap, tmpMap, COLOR_GRAY2BGR);
//			//tmpMap.copyTo(resultMap(Rect(0, matSize.height, matSize.width, matSize.height)));
//			tmpMap.copyTo(resultMap(Rect(matSize.width*1, 0, matSize.width, matSize.height)));

//			cvtColor(saliencyObj, tmpMap, COLOR_GRAY2BGR);
//			//tmpMap.copyTo(resultMap(Rect(matSize.width, matSize.height, matSize.width, matSize.height)));
//			tmpMap.copyTo(resultMap(Rect(matSize.width*2, 0, matSize.width, matSize.height)));

//			resize(resultMap, resultMap, Size(), 0.5, 0.5);

//			char fileName[100];
//			sprintf(fileName, "test/result/%04d_%04d__%s", (int)(tmp_precision*10000), (int)(tmp_recall*10000), testFile->d_name);
//			//sprintf(fileName, "test/result/%s", testFile->d_name);
//			if (tmp_precision < 0.8 || tmp_recall < 0.8)
//				imwrite(fileName, resultMap);
//			imwrite("debug_output/Result_Image.png", resultMap);
//			imshow("Result_Image.png", resultMap);

//			if (tmp_precision < 0.8 || tmp_recall < 0.8) {
//				char fileName[100];
//				sprintf(fileName, "test/negative/%s", testFile->d_name);
//				imwrite(fileName, inputImg);
//			}
//#endif
//#ifdef SHOW_IMAGE
//			waitKey(0);
//#endif
//		}
	}

//	for (int param1 = 0; param1 < test_num1; param1++) {
//		for (int param2 = 0; param2 < test_num2; param2++) {

//			int paramIdx = param1 * test_num2 + param2;

//			double a = precision_param[paramIdx] / fileNum;
//			double b = recall_param[paramIdx] / fileNum;
//			double c = (1 + 0.3) * a * b / (0.3 * a + b);
//			printf("%03d : %lf\t%lf\t%lf", PARAM_SET2[param2], a, b, c);
//			cout << endl;
//		}
//	}

	cout << (clock() - st_time) / 1000.0 << endl;

	return 0;

}
