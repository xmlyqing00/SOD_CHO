#ifndef SPECTRAL_H
#define SPECTRAL_H

#include "comman.h"

void getSpectralSaliency(Mat &spectralSaliency, const Mat &inputImg) {

	Mat grayImg;
	cvtColor(inputImg, grayImg, COLOR_RGB2GRAY);

	Size padded_size;
	Mat padded;
	padded_size = Size(getOptimalDFTSize(grayImg.cols), getOptimalDFTSize(grayImg.rows));
	copyMakeBorder(grayImg, padded,
				   0, padded_size.height - grayImg.rows,
				   0, padded_size.width - grayImg.cols,
				   BORDER_CONSTANT, Scalar::all(0));

	Mat complexPlanes[] = {Mat_<double>(padded), Mat::zeros(padded_size, CV_64FC1)};
	Mat complexMat;
	merge(complexPlanes, 2, complexMat);

	dft(complexMat, complexMat);
	split(complexMat, complexPlanes);

	Mat magnitudeMat, angleMat;
	cartToPolar(complexPlanes[0], complexPlanes[1], magnitudeMat, angleMat);
	//magnitudeMat += Scalar::all(1);
	log(magnitudeMat, magnitudeMat);

	Mat smoothedMagnitudeMat;
	blur(magnitudeMat, smoothedMagnitudeMat, Size(3,3));
	Mat spectralResidual = magnitudeMat - smoothedMagnitudeMat;

	normalize(spectralResidual, spectralResidual, 0, 1, CV_MINMAX);

	polarToCart(spectralResidual, angleMat, complexPlanes[0], complexPlanes[1]);
	merge(complexPlanes, 2, complexMat);

	dft(complexMat, complexMat, DFT_INVERSE);
	split(complexMat, complexPlanes);
	cartToPolar(complexPlanes[0], complexPlanes[1], magnitudeMat, angleMat);
	pow(magnitudeMat, 2, spectralSaliency);

	GaussianBlur(magnitudeMat, spectralSaliency, Size(11, 11), 2.5);
	normalize(spectralSaliency, spectralSaliency, 0, 1, NORM_MINMAX);

	imshow("Spectral_Saliency", spectralSaliency);

}

#endif // SPECTRAL_H

