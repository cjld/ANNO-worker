#pragma once

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "opencv2/opencv.hpp"
// #include <opencv/highgui.h>
// #include <stdlib.h>
#include <random>

using namespace std;
using namespace cv;

extern Point const DIRECTION4[5];
extern Point const DIRECTION8[9];
extern Point const DIRECTION16[17];
extern float const DRT_ANGLE[8];
extern float const PI_FLOAT;
extern float const PI2;
extern double const Cm2PI;
extern float const PI_HALF;
extern double const SQRT2;
extern double const CmEpsilon;

typedef uint8_t byte;

typedef vector<Point2d> PointSetd;
typedef vector<Point2i> PointSeti;
typedef const vector<Point2d> CPointSetd;
typedef const vector<Point2i> CPointSeti;
typedef vector<string> vecS;
typedef vector<int> vecI;
typedef vector<byte> vecB;
typedef vector<float> vecF;
typedef vector<double> vecD;
typedef vector<Mat> vecM;
typedef pair<double, int> CostIdx;
typedef pair<float, int> CostfIdx;
typedef pair<int, int> CostiIdx;
typedef vector<CostIdx> CostIdxV;
typedef vector<CostfIdx> CostfIdxV;
typedef vector<CostiIdx> CostiIdxV;
typedef complex<double> complexD;
typedef complex<float> complexF;

typedef const string CStr;
typedef const Mat CMat;
typedef const SparseMat CSMat;
typedef const vecI CVecI;
typedef const vecB CVecB;
typedef const vecF CVecF;
typedef const vecD CVecD;
typedef const vecM CVecM;
typedef const vecS CVecS;

typedef bool BOOL;
typedef unsigned int DWORD;
typedef int WORD;

extern mt19937 randEng;
extern uniform_int_distribution<int> randInt;

/************************************************************************/
/* Define Macros & Global functions                                     */
/************************************************************************/
const double EPS = 1e-10;		// Epsilon (zero value)
const double INF = 1e200;
#define _S(str) ((str).c_str())

#define CHECK_IND(c, r) (c >= 0 && c < _w && r >= 0 && r < _h)
#define CHK_IND(p) ((p).x >= 0 && (p).x < _w && (p).y >= 0 && (p).y < _h)
#define CHECK_INDEX(p, s) ((p).x >= 0 && (p).x < (s.width) && (p).y >= 0 && (p).y < (s.height))
#define CLAMP(v, l, h)	v = ((v)<(l) ? (l) : (v) > (h) ? (h) : v)

#define ForPointsD(pnt, pntS, pntE, d) 	for (Point pnt = (pntS); pnt.y != (pntE).y; pnt.y += (d)) for (pnt.x = (pntS).x; pnt.x != (pntE).x; pnt.x += (d))
#define ForPointsD_(pnt, pntS, pntE, d) for (Point pnt = (pntS); pnt.x != (pntE).x; pnt.x += (d)) for (pnt.y = (pntS).y; pnt.y != (pntE).y; pnt.y += (d))
#define ForPoints(pnt, pntS, pntE) 		for (Point pnt = (pntS); pnt.y != (pntE).y; pnt.y++) for (pnt.x = (pntS).x; pnt.x != (pntE).x; pnt.x++)
#define ForPoints2(pnt, xS, yS, xE, yE)	for (Point pnt(0, (yS)); pnt.y != (yE); pnt.y++) for (pnt.x = (xS); pnt.x != (xE); pnt.x++)

/************************************************************************/
/* Useful template                                                      */
/************************************************************************/

template<typename T> inline T sqr(T x) { return x * x; } // out of range risk for T = byte, ...
template<typename T> inline int CmSgn(T number) {if(abs(number) < EPS) return 0; return number > 0 ? 1 : -1; }
template<class T> inline T pntSqrDist(const Point_<T> &p1, const Point_<T> &p2) {return sqr(p1.x - p2.x) + sqr(p1.y - p2.y);} // out of range risk for T = byte, ...
template<class T> inline double pntDist(const Point_<T> &p1, const Point_<T> &p2) {return sqrt((double)pntSqrDist(p1, p2));} // out of range risk for T = byte, ...
template<class T> inline T vecSqrDist3(const Vec<T, 3> &v1, const Vec<T, 3> &v2) {return sqr(v1[0] - v2[0])+sqr(v1[1] - v2[1])+sqr(v1[2] - v2[2]);} // out of range risk for T = byte, ...
template<class T> inline T vecDist3(const Vec<T, 3> &v1, const Vec<T, 3> &v2) {return sqrt(vecSqrDist3(v1, v2));} // out of range risk for T = byte, ...
template<class T, int D> inline T vecSqrDist(const Vec<T, D> &v1, const Vec<T, D> &v2) {T s = 0; for (int i=0; i<D; i++) s += sqr(v1[i] - v2[i]); return s;} // out of range risk for T = byte, ...
template<class T, int D> inline T vecDist(const Vec<T, D> &v1, const Vec<T, D> &v2) { return sqrt(vecSqrDist(v1, v2)); } // out of range risk for T = byte, ...
template<class T1, class T2> inline void operator /= (Vec<T1, 3> &v1, const T2 v2) { v1[0] = (T1)(v1[0]/v2); v1[1] = (T1)(v1[1]/v2); v1[2] = (T1)(v1[2]/v2); } // out of range risk for T = byte, ...

inline Point round(const Point2d &p) { return Point(cvRound(p.x), cvRound(p.y));}


#define CV_Assert_(expr, args) \
{\
	if(!(expr)) {\
	string msg = cv::format args; \
	CmLog::LogError("%s in %s:%d\n", msg.c_str(), __FILE__, __LINE__); \
	cv::error(cv::Exception(CV_StsAssert, msg, __FUNCTION__, __FILE__, __LINE__) ); }\
}

inline double MatMin(CMat &m) {double minVal; minMaxLoc(m, &minVal, NULL); return minVal; }
inline double MatMax(CMat &m) {double maxVal; minMaxLoc(m, NULL, &maxVal); return maxVal; }
template<class T> inline Point_<T> operator / (const Point_<T> &p, double s) { return Point_<T>((T)(p.x /s), (T)(p.y/s));}
template<class T> inline void operator /= (Point_<T> &p, double s) {p.x /= s, p.y /= s;}
template<class T> inline Vec<T, 3> operator / (const Vec<T, 3> &v, double s) { return Vec<T, 3>((T)(v[0]/s), (T)(v[1]/s), (T)(v[2]/s));}



template<typename T> vector<T> operator +(const vector<T>& v1, const vector<T> &v2)
{
	vector<T> result(v1);
	for (size_t i = 0, iEnd = v1.size(); i != iEnd; i++)
		result[i] = v1[i] + v2[i];
	return result;
}

template<typename T> vector<T> operator -(const vector<T>& v1, const vector<T> &v2)
{
	vector<T> result(v1);
	for (size_t i = 0, iEnd = v1.size(); i != iEnd; i++)
		result[i] = v1[i] - v2[i];
	return result;
}

template<typename T> vector<T> operator *(const vector<T>& v1, const vector<T> &v2)
{
	vector<T> result(v1);
	for (size_t i = 0, iEnd = v1.size(); i != iEnd; i++)
		result[i] = v1[i] * v2[i];
	return result;
}

template<typename T> vector<T> operator /(const vector<T>& v1, const vector<T> &v2)
{
	vector<T> result(v1);
	for (size_t i = 0, iEnd = v1.size(); i != iEnd; i++)
		result[i] = v1[i] * v2[i];
	return result;
}

template<class T> void maxSize(const vector<Point_<T>> &pnts, T &minS, T &maxS)
{
	CV_Assert(pnts.size() > 0);
	minS = maxS = pnts[0].x;
	for (int i = 0, iEnd = (int)pnts.size(); i < iEnd; i++)
	{
		minS = min(minS, pnts[i].x);
		minS = min(minS, pnts[i].y);
		maxS = max(maxS, pnts[i].x);
		maxS = max(maxS, pnts[i].y);
	}
}


