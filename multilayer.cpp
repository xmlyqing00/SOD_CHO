#include "multilayer.h"

float getMinIntDiff(int SEGMENT_THRESHOLD, int regionSize) {
	return (float)SEGMENT_THRESHOLD / regionSize;
}

int Point2Index(Point u, int width) {
	return u.y * width + u.x;
}

void overSegmentation(TypeRegionSet &regionSet, const Mat &LABImg) {

	vector<TypeEdgePts> edges;
	Size imgSize = LABImg.size();
	const int leftside[4] = {2, 3, 5, 7};

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			Point nowP = Point(x, y);

			for (int k = 0; k < 4; k++) {

				Point newP = nowP + dxdy[leftside[k]];
				if (isOutside(newP.x, newP.y, imgSize.width, imgSize.height)) continue;

				Vec3f nowColor = LABImg.ptr<Vec3f>(nowP.y)[nowP.x];
				Vec3f newColor = LABImg.ptr<Vec3f>(newP.y)[newP.x];
				double diff = calcVec3fDiff(nowColor, newColor);
				edges.push_back(TypeEdgePts(nowP, newP, diff));
			}
		}
	}

	sort(edges.begin(), edges.end(), cmpTypeEdgePts);

	int pixelCount = imgSize.height * imgSize.width;
	int *regionHead = new int[pixelCount];
	int *regionSize = new int[pixelCount];
	double *minIntDiff = new double[pixelCount];
	for (int i = 0; i < pixelCount; i++) {
		regionHead[i] = i;
		regionSize[i] = 1;
		minIntDiff[i] = getMinIntDiff(SEGMENT_THRESHOLD, 1);
	}

	for (size_t i = 0; i < edges.size(); i++) {

		int uIdx = Point2Index(edges[i].u, imgSize.width);
		int vIdx = Point2Index(edges[i].v, imgSize.width);
		int pu = getElementHead(uIdx, regionHead);
		int pv = getElementHead(vIdx, regionHead);
		if (pu == pv) continue;

		if ((edges[i].w <= minIntDiff[pu]) && (edges[i].w <= minIntDiff[pv])) {

			regionHead[pv] = pu;
			regionSize[pu] += regionSize[pv];
			minIntDiff[pu] = edges[i].w + getMinIntDiff(SEGMENT_THRESHOLD, regionSize[pu]);
		}
	}

	delete[] minIntDiff;

	for (size_t i = 0; i < edges.size(); i++) {

		int uIdx = Point2Index(edges[i].u, imgSize.width);
		int vIdx = Point2Index(edges[i].v, imgSize.width);
		int pu = getElementHead(uIdx, regionHead);
		int pv = getElementHead(vIdx, regionHead);
		if (pu == pv) continue;

		if ((regionSize[pu] < MIN_REGION_SIZE) || (regionSize[pv] < MIN_REGION_SIZE)) {
			regionHead[pv] = pu;
			regionSize[pu] += regionSize[pv];
		}
	}

	// get main region
	int idx = 0;
	regionSet.regionCount = 0;

	int *regionIndex = new int[pixelCount];
	for (int i = 0; i < pixelCount; i++) regionIndex[i] = -1;

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			int pIdx = getElementHead(idx, regionHead);
			if (regionIndex[pIdx] == -1) {
				regionSet.img2region.ptr<int>(y)[x] = regionSet.regionCount;
				regionIndex[pIdx] = regionSet.regionCount++;
			} else {
				regionSet.img2region.ptr<int>(y)[x] = regionIndex[pIdx];
			}
			idx++;
		}
	}

	delete[] regionHead;
	delete[] regionIndex;
	delete[] regionSize;

	//cout << regionSet.regionCount << endl;

#ifdef SHOW_IMAGE
	regionSet.writeRegionImage();
#endif

}

void buildMultiLayerModel(vector<TypeRegionSet> &multiLayerModel, const Mat &paletteMap, const Mat &colorImg) {

	Size imgSize = colorImg.size();
	TypeRegionSet regionSet(imgSize, 0);

	overSegmentation(regionSet, colorImg);

	regionSet.calcRegionsAttr(paletteMap);

	multiLayerModel.push_back(regionSet);

	while (!regionSet.mergeEnd) {

		regionSet.mergeRegions();
		multiLayerModel.push_back(regionSet);

#ifdef SHOW_IMAGE
		regionSet.writeRegionImage(0);
#endif

	}

}
