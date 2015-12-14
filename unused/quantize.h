#ifndef QUANTIZE_H
#define QUANTIZE_H

#include "comman.h"

void getQuantizeImage(Mat &colorMap, vector<Vec3f> &platte, const Mat &colorImg) {

	Size imgSize = colorImg.size();
	vector<TypeColorSpace> colorSet;
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			colorSet.push_back(TypeColorSpace(Point(x,y), colorImg.ptr<Vec3f>(y)[x]));
		}
	}

	vector< vector<TypeColorSpace> > medianCutQue;
	medianCutQue.push_back(colorSet);

	for (int level = 0; level < QUANTIZE_LEVEL; level++) {

		vector< vector<TypeColorSpace> > tmpQue;

		for (size_t i = 0; i < medianCutQue.size(); i++) {

			Vec3f minColor(255, 255, 255);
			Vec3f maxColor(0, 0, 0);
			for (size_t j = 0; j < medianCutQue[i].size(); j++) {
				for (int k = 0; k < 3; k++) {
					minColor.val[k] = min(minColor.val[k], medianCutQue[i][j].color[k]);
					maxColor.val[k] = max(maxColor.val[k], medianCutQue[i][j].color[k]);
				}
			}

			int cut_dimension = 0;
			double max_range = 0;
			for (int k = 0; k < 3; k++) {
				if (maxColor.val[k] - minColor.val[k] > max_range) {
					max_range = maxColor.val[k] - minColor.val[k];
					cut_dimension = k;
				}
			}

			switch (cut_dimension) {
			case 0:
				sort(medianCutQue[i].begin(), medianCutQue[i].end(), cmpColor0);
				break;
			case 1:
				sort(medianCutQue[i].begin(), medianCutQue[i].end(), cmpColor1);
				break;
			case 2:
				sort(medianCutQue[i].begin(), medianCutQue[i].end(), cmpColor2);
				break;
			default:
				cout << "error in cut" << endl;
				exit(0);
			}

			int mid_pos = medianCutQue[i].size() / 2;
			vector<TypeColorSpace> part0(medianCutQue[i].begin(), medianCutQue[i].begin() + mid_pos);
			vector<TypeColorSpace> part1(medianCutQue[i].begin() + mid_pos, medianCutQue[i].end());

			tmpQue.push_back(part0);
			tmpQue.push_back(part1);
		}

		medianCutQue = tmpQue;
	}

	for (size_t i = 0; i < medianCutQue.size(); i++) {

		Vec3f meanColor = Vec3f(medianCutQue[i][medianCutQue[i].size()>>1].color);
		platte.push_back(meanColor);
	}

	int range = int(0.1 * platte.size());
	colorMap = Mat(imgSize, CV_8UC1);
	for (size_t i = 0; i < medianCutQue.size(); i++) {

		for (size_t j = 0; j < medianCutQue[i].size(); j++) {

			Vec3f c = Vec3f(medianCutQue[i][j].color);

			size_t best_fit = i;
			double min_diff = INF;
			for (int k = 0; k < range; k++) {

				int tmpIdx = i + k;
				if (tmpIdx < 0 || tmpIdx >= (int)medianCutQue.size()) continue;
				double tmp = colorDiff(c, platte[tmpIdx]);
				if (tmp < min_diff) {
					min_diff = tmp;
					best_fit = tmpIdx;
				}

				tmpIdx = i - k;
				if (tmpIdx < 0 || tmpIdx >= (int)medianCutQue.size()) continue;
				tmp = colorDiff(c, platte[tmpIdx]);
				if (tmp < min_diff) {
					min_diff = tmp;
					best_fit = tmpIdx;
				}
			}
			colorMap.at<uchar>(medianCutQue[i][j].pos) = best_fit;
		}
	}

#ifdef SHOW_IMAGE
	Mat quantizeImg(imgSize, CV_32FC3);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			quantizeImg.ptr<Vec3f>(y)[x] = platte[colorMap.ptr<uchar>(y)[x]];
		}
	}
	cvtColor(quantizeImg, quantizeImg, COLOR_Lab2BGR);
	imshow("Quantize_Image.png", quantizeImg);
	imwrite("Quantize_Image.png", quantizeImg);
#endif

}

#endif // QUANTIZE_H

