
// #include <cv.h>
// #include <highgui.h>

#pragma once

#include "cvHelper.h"


#include "msImageProcessor.h"
#include "../graphCut/graph.h"
#include <set>

class CAllSegs
{
public:
	CAllSegs();
	~CAllSegs();

	enum SEG_MODE {MS_MODE, WS_MODE};

	int regCount;
	int *labels;
	std::vector<cv::Vec3d> regAverage;
	std::vector<std::vector<cv::Point>> regCluster;
	
	std::vector<std::set<int>> regNeighborhood;
	//std::vector<std::set<int>> regNeighborhoodNext;

	std::vector<cv::Point> simPat;
	std::vector<std::set<int>> simPatches;
	std::vector<cv::Point> simPatArea;
	std::vector<std::set<int>> simPatchesArea;

	bool InitSegs(cv::Mat &inImage, SEG_MODE mode);

	void SetParas(float _sigmaS, float _sigmaR, int _minSize, int _basinvalue, \
					int _threshold, int _wsvalue, double _colorthreshold, double _areathreshold );
    bool RunSeg();

	void RefineSeg();

	cv::Mat segs;
	bool GetSegs();

	bool GetSegs(QImage&);

protected:


private:

	//parameters
	float	ms_sigmaS;
	float	ms_sigmaR;
	int		ms_minSize;
	int		ws_basinvalue;
	int		ws_threshold;
	int		ws_wsvalue;

	double color_threshold;
	double area_threshold;

	cv::Mat image;

	SEG_MODE segMode;

	void InitColorTable();

	int GetRegCount(int* _labels, int _length);
	bool GetRegions();
	bool GetNeighborhood();
	bool GetSimPat();



	cv::Vec3b RandColor()
	{
		cv::Vec3b c;
		srand( (unsigned)(time(NULL) + rand()));
		c[0] = rand()%255;
		srand( (unsigned)(time(NULL) + rand()));
		c[1] = rand()%255;
		srand( (unsigned)(time(NULL) + rand()));
		c[2] = rand()%255;
		return c;
	};
};
