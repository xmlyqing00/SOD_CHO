#include "comman.h"
#include "segment.h"
#include "pyramid.h"
#include "saliency.h"
#include "cutobj.h"
#include "evaluate.h"

int main(int args, char **argv) {

#ifdef EVALUATE_MASK
	benchMark(argv[1]);
	return 0;
#endif

	int st_time = clock();

	string dataset_name = "HKU-IS/";
	string dir_path = "/Ship01/Dataset/" + dataset_name;
	string gt_dir_path = dir_path + "gt/";
	string img_dir_path = dir_path + "imgs/";
	string result_dir_path = "saliency/" + dataset_name;

	map<string, Mat> binaryMask;
	getGroundTruth(binaryMask, gt_dir_path);

	DIR *img_dir = opendir(img_dir_path.c_str());
	dirent *img_file_name;
	int fileNum = 0;

	const int test_num = args - 1;
	vector<int> PARAM_SET(test_num, 0);
	vector<double> precision_param(test_num, 0);
	vector<double> recall_param(test_num, 0);
	vector<double> MAE_param(test_num, 0);

	for (int i = 0; i < test_num; i++) {
		PARAM_SET[i] = atoi(argv[i+1]);
	}

	cout << "Test " << test_num << " Params: ";
	for (int i = 0; i < test_num; i++) cout << PARAM_SET[i] << " ";
	cout << endl;

	while ((img_file_name = readdir(img_dir)) != NULL) {

		if (strcmp(img_file_name->d_name, ".") == 0 || strcmp(img_file_name->d_name, "..") == 0) continue;
		fileNum++;
		cout << fileNum << " " << img_file_name->d_name << endl;

		string input_img_name = img_dir_path + img_file_name->d_name;

		Mat inputImg, LABImg;
		readImage(input_img_name, inputImg, LABImg);

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
		string output_path = result_dir_path + img_file_name->d_name;
		imwrite(output_path, saliencyMap);
		// continue;
#endif
		for (int paramIdx = 0; paramIdx < test_num; paramIdx++) {

			Mat saliencyObj;
			//getSaliencyObj(saliencyObj, saliencyMap, LABImg, PARAM_SET2[param2]);
			threshold(saliencyMap, saliencyObj, PARAM_SET[paramIdx], 255, THRESH_BINARY);

			double tmp_precision, tmp_recall, tmp_MAE;

			evaluateMap(tmp_precision, tmp_recall, tmp_MAE, binaryMask[img_file_name->d_name], saliencyObj);

			precision_param[paramIdx] += tmp_precision;
			recall_param[paramIdx] += tmp_recall;
			MAE_param[paramIdx] += tmp_MAE;

			double avg_prec = precision_param[paramIdx] / fileNum;
			double avg_recall = recall_param[paramIdx] / fileNum;
			double F = ((1.3) * avg_prec * avg_recall) / (0.3 * avg_prec + avg_recall);

			if (fileNum % 100 == 0) {
				printf("PARAM %d\tprec %.3lf recall %.3lf F: %.3lf\tMAE: %.3lf\n",
					PARAM_SET[paramIdx],
				   	avg_prec, avg_recall, F,
					MAE_param[paramIdx] / fileNum);
			}
			

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

	for (int paramIdx = 0; paramIdx < test_num; paramIdx++) {

		double a = precision_param[paramIdx] / fileNum;
		double b = recall_param[paramIdx] / fileNum;
		double c = (1 + 0.3) * a * b / (0.3 * a + b);
		double d = MAE_param[paramIdx] / fileNum;
		printf("%03d : prec: %.3lf\trecall %.3lf\tF: %.3lf MAE: %.3lf\n", PARAM_SET[paramIdx], a, b, c, d);
	}

	cout << (clock() - st_time) / 1000.0 << endl;

	return 0;

}
