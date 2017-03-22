#pragma once

#ifndef PAD_IMG_SYMMETRIC
#define PAD_IMG_SYMMETRIC 0   
#define PAD_IMG_ZERO      1
#endif
#include "CmDefinition.h"

// #define CV_Assert_(expr, args) \
// {\
// 	if(!(expr)) {\
// 	string msg = cv::format args; \
// 	CmLog::LogError("%s in %s:%d\n", msg.c_str(), __FILE__, __LINE__); \
// 	cv::error(cv::Exception(CV_StsAssert, msg, __FUNCTION__, __FILE__, __LINE__) ); }\
// }

// inline double MatMin(CMat &m) {double minVal; minMaxLoc(m, &minVal, NULL); return minVal; }
// inline double MatMax(CMat &m) {double maxVal; minMaxLoc(m, NULL, &maxVal); return maxVal; }
// template<class T> inline Point_<T> operator / (const Point_<T> &p, double s) { return Point_<T>(p.x /s, p.y/s);}
// template<class T> inline void operator /= (Point_<T> &p, double s) {p.x /= s, p.y /= s;}
// template<class T> inline Vec<T, 3> operator / (const Vec<T, 3> &v, double s) { return Vec<T, 3>(v[0] /s, v[1]/s, v[2]/s);}

class CmCv
{
public:
	// AbsAngle: Calculate magnitude and angle of vectors.
	static void AbsAngle(CMat& cmplx32FC2, Mat& mag32FC1, Mat& ang32FC1);

	// GetCmplx: Get a complex value image from it's magnitude and angle.
	static void GetCmplx(CMat& mag32F, CMat& ang32F, Mat& cmplx32FC2);

	// Mat2GrayLog: Convert and arbitrary mat to [0, 1] for display.
	// The result image is in 32FCn format and range [0, 1.0].
	// Mat2GrayLinear(log(img+1), newImg). In place operation is supported.
	static void Mat2GrayLog(CMat& img, Mat& newImg);

	// Low frequency part is always been move to the central part:
	//				 -------                          -------	
	//				| 1 | 2 |                        | 3 | 4 |	
	//				 -------            -->           -------	
	//				| 4 | 3 |                        | 2 | 1 |	
	//				 -------                          -------	
	static void FFTShift(Mat& img);

	// Swap the content of two Mat with same type and size
    static inline void Swap(Mat a, Mat b);

	// Normalize size/image to min(width, height) = shortLen and use width 
	// and height to be multiples of unitLen while keeping its aspect ratio 
	// as much as possible. unitLen must not be 0.
	static inline Size NormalizeSize(const Size& sz, int shortLen, int unitLen = 1);
	static inline void NormalizeImg(CMat&img, Mat& dstImg, int shortLen = 256, int unitLen = 8);

	// Get image region by two corner point.
	static inline Rect GetImgRange(Point p1, Point p2, Size imgSz);

	// Check an image (with size imgSz) point and correct it if necessary
	static inline void CheckPoint(Point &p, Size imgSz);

	static inline void Merge(CMat &m0, CMat &m1, CMat &m2, Mat &m);
	
	// Get mask region. 
	static Rect GetMaskRange(CMat &mask1u, int ext = 0);
	
	// Get continuous components for same label regions. Return region index mat,
	// index-counter pair (Number of pixels for each index), and label of each idx
    static int GetRegions(const Mat_<byte> &label1u, Mat_<int> &regIdx1i, vecI &idxCount, vecB idxLabel = vecB(), bool noZero = false);

	// Get continuous components for non-zero labels. Return region index mat (region index 
	// of each mat position) and sum of label values in each region
	static int GetNZRegions(const Mat_<byte> &label1u, Mat_<int> &regIdx1i, vecI &idxSum);

	// Get continuous None-Zero label Region with Largest Sum value
	static Mat GetNZRegionsLS(CMat &mask1u, double ignoreRatio = 0.02);
	
	// Get points in border regions
	static int GetBorderPnts(Size sz, double ratio, vector<Point> &bdPnts);

	// Get border regions, which typically corresponds to background region
	static Mat GetBorderReg(CMat &idx1i, int regNum, double ratio = 0.02, double thr = 0.4);  
	static Mat GetBorderRegC(CMat &img3u, Mat &idx1i, vecI &idxCount);
	
	// Merge similar regions
	static int MergeSimiReg(CMat img3f, Mat idx1i, int regNum, double minColor = 2, double minVar = 0.4);
	//static int MergeSimiReg(CMat &colorIdx1i, int colorNum, Mat idx1i, int regNum, double thr);
	//static int MergeSimiReg(CMat &img3f, Mat idx1i, int regNum, double thr);

	static void fillPoly(Mat& img, const vector<PointSeti> _pnts, const Scalar& color, int lineType = 8, int shift = 0, Point offset = Point());

	// down sample without convolution, similar to cv::pyrDown
	template<class T> static void PyrDownSample(CMat &src, Mat &dst);
	template<class T> static void PyrUpSample(CMat &src, Mat &dst, Size dSz);

    static void inline SaveImgRGB(CStr &fName, CMat &img);// Saving RGB image for QImage data

public:
	static void Demo(const char* fileName = "H:\\Resize\\cd3.avi");

	//// Adding alpha value to img to show. img: 8U3C, alpha 8U1C
	//static void AddAlpha(CvMat *img, CvMat *alpha);
	//static void AddAlpha(CvMat *img, CvMat *alpha, CvScalar bgColor);

	//// Rotate and scale an image
	//static CvMat* RotateScale(IN CvMat *src, IN double angle = 1.0, double scale = 1.0);
	//static void RotateScale(IN CvMat *src, OUT CvMat *dst, double angle = 1.0, double factor = 1.0);
};

// Normalize size/image to min(width, height) = shortLen and use width 
// and height to be multiples of unitLen while keeping its aspect ratio 
// as much as possible. unitLen must not be 0.

Size CmCv::NormalizeSize(const Size& sz, int shortLen, int unitLen)
{
	double ratio = double(shortLen) / min(sz.width, sz.height);
	return Size(cvRound(sz.width * ratio / unitLen) * unitLen, cvRound(sz.height * ratio /unitLen) * unitLen);
}

void CmCv::NormalizeImg(CMat&img, Mat& dstImg, int shortLen, int unitLen)
{
	resize(img, dstImg, NormalizeSize(img.size(), shortLen, unitLen));
}

void CmCv::CheckPoint(Point &p, Size imgSz)
{
	p.x = max(0, p.x), p.y = max(0, p.y);
	p.x = min(imgSz.width - 1, p.x);
	p.y = min(imgSz.height - 1, p.y);
}

Rect CmCv::GetImgRange(Point p1, Point p2, Size imgSz)
{
	CheckPoint(p1, imgSz);
	CheckPoint(p2, imgSz); 
	return Rect(min(p1.x, p2.x), min(p1.y, p2.y), abs(p1.x - p2.x), abs(p1.y - p2.y));
}

void CmCv::Merge(CMat &m0, CMat &m1, CMat &m2, Mat &m)
{
	CMat ms[3] = {m0, m1, m2};
	Mat rMat;
	cv::merge(ms, 3, rMat);
	m = rMat;
}

template<class T> void CmCv::PyrDownSample(CMat &src, Mat &dst)
{
	dst.create((src.rows+1)/2, (src.cols+1)/2, src.type());
	for (int r = 0; r < dst.rows; r++)	{
		const T *sP = src.ptr<T>(r * 2);
		T *dP = dst.ptr<T>(r);
		for (int c = 0; c < dst.cols; c++)
			dP[c] = sP[c*2];
	}
}

template<class T> void CmCv::PyrUpSample(CMat &src, Mat &dst, Size sz)
{
	dst.create(sz, src.type());
	for (int r = 0; r < dst.rows; r++)	{
		const T *sP = src.ptr<T>(r/2);
		T *dP = dst.ptr<T>(r);
		for (int c = 0; c < dst.cols; c++)
			dP[c] = sP[c/2];
	}
}

void CmCv::SaveImgRGB(CStr &fName, CMat &img)
{
	Mat saveImg;
	cvtColor(img, saveImg, CV_RGB2BGR);
	imwrite(fName, saveImg);
}
