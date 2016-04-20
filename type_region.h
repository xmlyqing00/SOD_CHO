#ifndef TYPE_REGION_H
#define TYPE_REGION_H

#include "comman.h"

class TypeRegion {

public:
	int ptsCount;
	Point centerPos;
	int avgRadius;
	vector<Point> pts;
	vector<Point> contourPts;
	vector<int> colorHist;

	TypeRegion();

	TypeRegion(const TypeRegion &r0, const TypeRegion &r1);

};

class TypeRegionSet {

public:
	int layerId;
	Size imgSize;
	Mat img2region;
	int regionCount;
	vector<TypeRegion> regions;

	vector<float> CHO;
	Mat CHOMap;

	vector<float> contrast;
	Mat contrastMap;

	bool mergeEnd;
	Mat regionsDist, regionsColorDiff, regionsW;

	TypeRegionSet(const Size &_imgSize, const int _layerId);

	void calcRegionsAttr(const Mat &paletteMap);

	void calcRegionsAttrAfterMerge();

	void calcRegionsWAll();

	void mergeRegions();

	void writeRegionImage(const bool showFlag = 0) const;

	void writeCHOMap(const bool showFlag = 0) const;

	void writeContrastMap(const bool showFlag = 0) const;

	void debugRegionImage(const int u, const int v, const bool writeFlag = 0) const;

};

double calcRegionsDist(const TypeRegion &r0, const TypeRegion &r1);

double calcRegionsColorDiff(const TypeRegion &r0, const TypeRegion &r1);

double calcRegionsW(const TypeRegion &r0, const TypeRegion &r1);

#endif // TYPE_REGION_H

