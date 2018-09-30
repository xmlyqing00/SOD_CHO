#include "comman.h"

Mat paletteDist;

bool cmpTypeEdgePts(const TypeEdgePts &e1, const TypeEdgePts &e2) {
	return e1.w < e2.w;
}

bool cmpTypeEdgeNodes(const TypeEdgeNodes &e1, const TypeEdgeNodes &e2) {
	return e1.w < e2.w;
}

bool cmpColor0(const TypeColorSpace &c0, const TypeColorSpace &c1) {
	return c0.color[0] < c1.color[0];
}

bool cmpColor1(const TypeColorSpace &c0, const TypeColorSpace &c1) {
	return c0.color[1] < c1.color[1];
}

bool cmpColor2(const TypeColorSpace &c0, const TypeColorSpace &c1) {
	return c0.color[2] < c1.color[2];
}

bool cmpSimilarColor(const pair<int,double> &p0, const pair<int,double> &p1) {
	return p0.second > p1.second;
}

bool isOutside( int x, int y, int boundX, int boundY ) {
	if ( x < 0 || y < 0 || x >= boundX || y >= boundY ) return true;
	return false;
}

bool isOutside(Point p, Size size) {
	if ( p.x < 0 || p.y < 0 || p.x >= size.width || p.y >= size.height ) return true;
	return false;
}

int float2sign(const double &f) {
	return (f < -FLOAT_EPS) ? -1 : (f > FLOAT_EPS);
}

double ptsDist(const Point &p0, const Point &p1) {
	return sqrt((p0.x-p1.x)*(p0.x-p1.x) + (p0.y-p1.y)*(p0.y-p1.y));
}

double calcVec3fDiff(const Vec3f &p0, const Vec3f &p1) {
	double diff = 0;
	for (int i = 0; i < 3; i++) {
		diff += sqr(p0.val[i] - p1.val[i]);
	}
	return sqrt(diff);
}

void quantizeColorSpace(Mat &paletteMap, vector<Vec3f> &palette, const Mat &colorImg) {

	Size imgSize = colorImg.size();

	paletteMap = Mat(imgSize, CV_8UC1);

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
		palette.push_back(meanColor);
	}

	int range = int(0.1 * palette.size());
	for (size_t i = 0; i < medianCutQue.size(); i++) {

		for (size_t j = 0; j < medianCutQue[i].size(); j++) {

			Vec3f c = Vec3f(medianCutQue[i][j].color);

			size_t best_fit = i;
			double min_diff = INF;
			for (int k = 0; k < range; k++) {

				int tmpIdx = i + k;
				if (tmpIdx < 0 || tmpIdx >= (int)medianCutQue.size()) continue;
				double tmp = calcVec3fDiff(c, palette[tmpIdx]);
				if (tmp < min_diff) {
					min_diff = tmp;
					best_fit = tmpIdx;
				}

				tmpIdx = i - k;
				if (tmpIdx < 0 || tmpIdx >= (int)medianCutQue.size()) continue;
				tmp = calcVec3fDiff(c, palette[tmpIdx]);
				if (tmp < min_diff) {
					min_diff = tmp;
					best_fit = tmpIdx;
				}
			}
			paletteMap.at<uchar>(medianCutQue[i][j].pos) = best_fit;
		}
	}

	paletteDist = Mat(palette.size(), palette.size(), CV_32FC1, Scalar(0));

	for (size_t c1 = 0; c1 < palette.size(); c1++) {
		for (size_t c2 = c1 + 1; c2 < palette.size(); c2++) {
			paletteDist.ptr<float>(c1)[c2] = calcVec3fDiff(palette[c1], palette[c2]);
			paletteDist.ptr<float>(c2)[c1] = paletteDist.ptr<float>(c1)[c2];
		}
	}

	Mat quantizeMap(imgSize, CV_32FC3);
	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {
			quantizeMap.ptr<Vec3f>(y)[x] = palette[paletteMap.ptr<uchar>(y)[x]];
		}
	}

	cvtColor(quantizeMap, quantizeMap, COLOR_Lab2BGR);
	quantizeMap.convertTo(quantizeMap, CV_8UC3, 255);
	imwrite("debug_output/Quantize Image.png", quantizeMap);
}

void normalizeVecf(vector<float> &vec) {

	float max_data = 0;
	float min_data = INF;

	for (size_t i = 0; i < vec.size(); i++) {

		max_data = max(max_data, vec[i]);
		min_data = min(min_data, vec[i]);
	}

	for (size_t i = 0; i < vec.size(); i++) {
		vec[i] = (vec[i] - min_data) / (max_data - min_data);
	}
}

int getElementHead( int u, int *head ) {

	if ( head[u] != u ) {
			head[u] = getElementHead( head[u], head );
	}
	return head[u];
}

int hashVec3b(const Vec3b &v) {

	int d = *(int*)(&v) & 0x00ffffff;
	return d;
}

Vec3b deHashVec3b(int d) {

	d = d | 0xff000000;
	Vec3b * _v = (Vec3b*)&d;
	return *_v;
}

void readImage( const char *imgName, Mat &inputImg, Mat &LABImg ) {

	inputImg = imread( imgName );
#ifdef SHOW_IMAGE
	imwrite("debug_output/Input_Image.jpg", inputImg);
#endif
	Mat tmpImg = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH)).clone();
	medianBlur(tmpImg, tmpImg, 3);

	tmpImg.convertTo(tmpImg, CV_32FC3, 1.0 / 255);
	cvtColor(tmpImg, LABImg, COLOR_BGR2Lab);
	imshow("Input Image", tmpImg);

//	cvtColor(tmpImg, tmpImg, COLOR_BGR2YCrCb);
//	Mat channels[3];
//	split(tmpImg, channels);
//	equalizeHist(channels[0], channels[0]);
//	merge(channels, 3, tmpImg);
//	cvtColor(tmpImg, tmpImg, COLOR_YCrCb2BGR);
//	imshow("equa", tmpImg);

}

void initTransparentImage(Mat &img) {

	int blockSize = 30;
	int halfBlockSize = 15;
	for (int y = 0; y < img.rows; y++) {
		for (int x = 0; x < img.cols; x++) {
			int _x = x % blockSize;
			int _y = y % blockSize;
			if ((_x < halfBlockSize && _y < halfBlockSize) ||
				(_x >= halfBlockSize && _y >=halfBlockSize)) {
				img.ptr<Vec3b>(y)[x] = Vec3b(182, 182, 182);
			} else {
				img.ptr<Vec3b>(y)[x] = Vec3b(153, 153, 153);
			}
		}
	}
}
