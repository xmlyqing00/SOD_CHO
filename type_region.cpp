#include "type_region.h"

double calcRegionsDist(const TypeRegion &r0, const TypeRegion &r1) {

	double dist = ptsDist(r0.centerPos, r1.centerPos);
	dist += max(r0.avgRadius, r1.avgRadius);
	return exp(-dist / SIGMA_DIST);
}

double calcRegionsColorDiff(const TypeRegion &r0, const TypeRegion &r1) {

	double colorDiff = 0;
	for (size_t c1 = 0; c1 < r0.colorHist.size(); c1++) {

		if (r0.colorHist[c1] == 0) continue;

		double tmpDiff = 0;
		for (size_t c2 = 0; c2 < r1.colorHist.size(); c2++) {

			if (r1.colorHist[c2] == 0) continue;
			tmpDiff += paletteDist.ptr<double>(c1)[c2] * r1.colorHist[c2];
		}
		colorDiff += r0.colorHist[c1] * tmpDiff / r1.ptsCount;

	}

	colorDiff /= r0.ptsCount;

	return exp(-colorDiff / SIGMA_COLOR);
}

double calcRegionsW(const TypeRegion &r0, const TypeRegion &r1) {

	double dist = calcRegionsDist(r0, r1);
	double colorDiff = calcRegionsColorDiff(r0, r1);
	return dist * colorDiff;

}

TypeRegion::TypeRegion() {
	centerPos = Point(0, 0);
	avgRadius = 0;
	pts.clear();
	contourPts.clear();
	ptsCount = 0;
	colorHist.clear();
}

TypeRegion::TypeRegion(const TypeRegion &r0, const TypeRegion &r1) {

	pts = vector<Point>(r0.pts);
	for (int i = 0; i < r1.ptsCount; i++) {
		pts.push_back(r1.pts[i]);
	}

	ptsCount = pts.size();

	centerPos = Point(0, 0);
	for (int i = 0; i < ptsCount; i++) centerPos += pts[i];
	centerPos.x /= ptsCount;
	centerPos.y /= ptsCount;

	avgRadius = 0;
	for (int i = 0; i < ptsCount; i++) avgRadius += ptsDist(centerPos, pts[i]);
	avgRadius /= ptsCount;

	for (size_t i = 0; i < r0.colorHist.size(); i++) {
		colorHist.push_back(r0.colorHist[i] + r1.colorHist[i]);
	}
}

TypeRegionSet::TypeRegionSet(const Size &_imgSize, const int _layerId) {
	imgSize = _imgSize;
	img2region = Mat(imgSize, CV_32SC1, Scalar(-1));
	regionsW = Mat(imgSize, CV_64FC1, Scalar(0));
	regions.clear();
	regionCount = 0;
	layerId = _layerId;
}

void TypeRegionSet::calcRegionsAttr(const Mat &paletteMap) {

	int paletteSize = paletteDist.rows;

	regions = vector<TypeRegion>(regionCount);
	for (int i = 0; i < regionCount; i++) {
		regions[i].colorHist = vector<int>(paletteSize, 0);
	}

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			Point nowP(x, y);
			int regionId = img2region.at<int>(nowP);
			regions[regionId].pts.push_back(nowP);
			regions[regionId].centerPos += nowP;
			regions[regionId].ptsCount++;

			int colorId = paletteMap.ptr<uchar>(y)[x];
			regions[regionId].colorHist[colorId]++;

			for (int k = 0; k < 4; k++) {

				Point newP = nowP + dxdy[k];
				if (isOutside(newP, imgSize)) continue;
				if (img2region.at<int>(newP) != regionId) {
					regions[regionId].contourPts.push_back(nowP);
					break;
				}
			}
		}
	}

	for (int i = 0; i < regionCount; i++) {

		regions[i].centerPos.x /= regions[i].ptsCount;
		regions[i].centerPos.y /= regions[i].ptsCount;

		int avgRadius = 0;
		for (int j = 0; j < regions[i].ptsCount; j++) {
			avgRadius += ptsDist(regions[i].centerPos, regions[i].pts[j]);
		}

		regions[i].avgRadius = avgRadius / regions[i].ptsCount;

	}

	CHO = vector<double>(regionCount, 0);

}

void TypeRegionSet::calcRegionsAttrAfterMerge() {

	for (int i = 0; i < regionCount; i++) {
		for (int j = 0; j < regions[i].ptsCount; j++) {
			img2region.at<int>(regions[i].pts[j]) = i;
		}
	}

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			Point nowP(x, y);
			int regionId = img2region.at<int>(nowP);

			for (int k = 0; k < 4; k++) {

				Point newP = nowP + dxdy[k];
				if (isOutside(newP, imgSize)) continue;
				if (img2region.at<int>(newP) != regionId) {
					regions[regionId].contourPts.push_back(nowP);
					break;
				}
			}
		}
	}

	CHO = vector<double>(regionCount, 0);
	layerId++;

}

void TypeRegionSet::calcRegionsWAll() {

	Mat regionsDist(regionCount, regionCount, CV_64FC1, Scalar(0));
	Mat regionsColorDiff(regionCount, regionCount, CV_64FC1, Scalar(0));

	for (int i = 0; i < regionCount; i++) {
		for (int j = i + 1; j < regionCount; j++) {
			regionsDist.ptr<double>(i)[j] = calcRegionsDist(regions[i], regions[j]);
			regionsColorDiff.ptr<double>(i)[j] = calcRegionsColorDiff(regions[i], regions[j]);

//			printf("%d %d dist %.2lf color %.2lf", i, j, regionsDist.ptr<double>(i)[j], regionsColorDiff.ptr<double>(i)[j]);
//			cout << endl;
		}

	}

	regionsW = regionsDist.mul(regionsColorDiff);

//	for (int i = 0; i < regionCount; i++) {
//		for (int j = i + 1; j < regionCount; j++) {
//			printf("%d %d w %.2lf\n", i, j, regionsW.ptr<double>(i)[j]);
//		}
//	}
}

void TypeRegionSet::mergeRegions() {

	calcRegionsWAll();

	priority_queue<TypeEdgeNodes> mergeQueue;

	for (int i = 0; i < regionCount; i++) {
		for (int j = i + 1; j < regionCount; j++) {
			mergeQueue.push(TypeEdgeNodes(i, j, regionsW.ptr<double>(i)[j]));
		}
	}

	vector<bool> regionExist(regionCount, true);
	int old_regionCount = regionCount;

	while (regionCount < old_regionCount * 1.5) {

		TypeEdgeNodes curEdge = mergeQueue.top();
		mergeQueue.pop();

		while (!regionExist[curEdge.u] || !regionExist[curEdge.v]) {
			curEdge = mergeQueue.top();
			mergeQueue.pop();
		}

		regionExist[curEdge.u] = false;
		regionExist[curEdge.v] = false;

		TypeRegion newRegion(regions[curEdge.u], regions[curEdge.v]);

		for (int i = 0; i < regionCount; i++) {
			if (regionExist[i]) {
				mergeQueue.push(TypeEdgeNodes(i, regionCount, calcRegionsW(regions[i], newRegion)));
			}
		}

		regionExist.push_back(true);
		regions.push_back(newRegion);
		regionCount++;

//		printf("u %d v %d dist %.2lf color %.2lf w %.2lf", curEdge.u, curEdge.v, calcRegionsDist(regions[curEdge.u], regions[curEdge.v]), calcRegionsColorDiff(regions[curEdge.u], regions[curEdge.v]), curEdge.w);
//		cout << endl;
//		debugRegionImage("debug", curEdge.u, curEdge.v, 0);
//		waitKey(0);
	}

	int new_regionCount = 0;
	for (int i = 0; i < regionCount; i++) {
		if (regionExist[i]) regions[new_regionCount++] = regions[i];
	}

	regionCount = new_regionCount;
	regions.resize(regionCount);

	calcRegionsAttrAfterMerge();

}

void TypeRegionSet::writeRegionImage(const char *imgName, const bool &showFlag) {

	srand( clock() );
	Mat regionImg = Mat::zeros(imgSize, CV_8UC3 );
	vector<Vec3b> color;
	for ( int i = 0; i < regionCount; i++ ) {

		uchar t0 = rand() * 255;
		uchar t1 = rand() * 255;
		uchar t2 = rand() * 255;
		color.push_back( Vec3b( t0, t1, t2 ) );
	}

	for ( int y = 0; y < imgSize.height; y++ ) {
		for ( int x = 0; x < imgSize.width; x++ ) {
			regionImg.ptr<Vec3b>(y)[x] = color[img2region.ptr<int>(y)[x]];
		}
	}

	imwrite(imgName, regionImg);
	if (showFlag) imshow(imgName, regionImg);

}

void TypeRegionSet::debugRegionImage(const char *imgName, const int u, const int v, const bool &writeFlag) {

	srand( clock() );
	Mat regionImg = Mat::zeros(imgSize, CV_8UC3 );
	vector<Vec3b> color;
	for ( int i = 0; i < regionCount; i++ ) {

		uchar t0 = rand() * 255;
		uchar t1 = rand() * 255;
		uchar t2 = rand() * 255;
		color.push_back( Vec3b( t0, t1, t2 ) );
	}

	for (int i = 0; i < regionCount; i++) {
		for (int j = 0; j < regions[i].ptsCount; j++) {
			regionImg.at<Vec3b>(regions[i].pts[j]) = color[i];
		}
	}

	circle(regionImg, regions[u].centerPos, regions[u].avgRadius, Scalar(255, 0, 0), 3);
	circle(regionImg, regions[v].centerPos, regions[v].avgRadius, Scalar(0, 0, 255), 3);

	imshow(imgName, regionImg);
	if (writeFlag) imwrite(imgName, regionImg);

}
