#include "allsegs.h"
#include <QMap>

QMap<int, cv::Vec3b> segColorTable_g;

CAllSegs::CAllSegs()
{
	ms_sigmaS		= 3.0;
	ms_sigmaR		= 3.0;
	ms_minSize		= 30;
	ws_basinvalue	= 0;
	ws_threshold	= 4;
	ws_wsvalue		= 25;

	color_threshold = 5.0;
	area_threshold = 0.6;

	regCount = -1;
	labels	= NULL;

	segMode	= MS_MODE;

	InitColorTable();

	//regNeighborhoodNext.clear();
}

CAllSegs::~CAllSegs()
{
	if (labels)
	{
		delete labels;
		labels = NULL;
	}

}

bool CAllSegs::InitSegs( cv::Mat &inImage, SEG_MODE mode )
{
	if (inImage.empty())
		return false;

	inImage.copyTo(image);
	segMode = mode;
	labels = new int[image.rows*image.cols];

	if (!labels || image.empty()) 
		return false;

	return true;

}

bool CAllSegs::RunSeg()
{
	if (image.empty())
		return false;

	double time = 0;
	if (segMode == MS_MODE)
	{
		clock_t begin = clock();
		msImageProcessor ms;
		ms.DefineImage(image.data, COLOR, image.rows, image.cols);
        ms.Segment(ms_sigmaS, ms_sigmaR, ms_minSize, HIGH_SPEEDUP);
		regCount = ms.MyGetRegions(labels);
		clock_t end = clock();
		time = (double)(end-begin) / CLOCKS_PER_SEC;
	}

	GetRegions();
	GetNeighborhood();
	RefineSeg();


	//GetSimPat();

	return true;
}

void CAllSegs::RefineSeg()
{
	// TODO ...

	for (int i = 0; i < regCount; ++i)
	{
		// if one patch is surrounded by only one patch
		if (1 == regNeighborhood[i].size())
		{
			  std::set<int>::iterator it = regNeighborhood[i].begin();

			  regAverage[*it] = regAverage[i];
		}
		
	}

}

int CAllSegs::GetRegCount( int* _labels, int _length )
{
	int count = -1;
	for (int i=0; i<_length; i++)
	{
		if (count < _labels[i])
		{
			count = labels[i];
		}
	}

	count = count + 1;

	return count;

}

bool CAllSegs::GetRegions()
{
	regCluster.clear();
	for (int i=0; i<regCount; i++)
	{
		std::vector<cv::Point> cluster;
		regCluster.push_back(cluster);
	}

	//regClusters
	for (int i=0; i<image.rows; i++)
	{
		for (int j=0; j<image.cols; j++)
		{
			int regIndex = labels[i*image.cols+j];
			regCluster[regIndex].push_back(cv::Point(j, i));
		}
	}

	//regAverage
	regAverage.clear();
	for (int i=0; i<regCount; i++)
	{
		cv::Vec3d pixAvr = cv::Vec3d(0,0,0);
		for (std::vector<cv::Point>::iterator it=regCluster[i].begin(); it!=regCluster[i].end(); it++)
		{
			cv::Vec3b tempPix = image.at<cv::Vec3b>((*it).y,(*it).x);
			pixAvr[0] += (double)tempPix[0];
			pixAvr[1] += (double)tempPix[1];
			pixAvr[2] += (double)tempPix[2];
		}
		pixAvr[0] /= regCluster[i].size();
		pixAvr[1] /= regCluster[i].size();
		pixAvr[2] /= regCluster[i].size();
		regAverage.push_back(pixAvr);
	}

	return true;
}

bool CAllSegs::GetNeighborhood()
{
	regNeighborhood.clear();
	for (int rIdx=0; rIdx<regCount; rIdx++)
	{
		std::set<int> neighborhood;
		for (std::vector<cv::Point>::iterator it=regCluster[rIdx].begin(); it!=regCluster[rIdx].end(); it++)
		{
			int x = (*it).x;
			int y = (*it).y;
			for (int i=-1; i<2; i++)
			{
				for (int j=-1; j<2; j++)
				{
					if (x+j>=0 && x+j<image.cols && y+i>=0 && y+i<image.rows)
					{
						int lbl = labels[(y + i) * image.cols + x + j];
						if (lbl!=rIdx)
						{
							neighborhood.insert(lbl);
						}							
					}
				}
			}
		}
		regNeighborhood.push_back(neighborhood);
	}

	return true;

}

bool CAllSegs::GetSegs()
{	
	if (image.empty()) return false;
	segs = cv::Mat::zeros(image.size(), image.type());

	for (int i=0; i<regCount; i++)
	{
		cv::Vec3b c = cv::Vec3b((uchar)regAverage[i][0],(uchar)regAverage[i][1],(uchar)regAverage[i][2]);
		cv::Vec3b cr = RandColor();
		for (std::vector<cv::Point>::iterator it=regCluster[i].begin(); it!=regCluster[i].end(); it++)
		{
			segs.at<cv::Vec3b>((*it).y,(*it).x) = cr;
		}
	}
	
	return true;
}

void CAllSegs::InitColorTable()
{
	for (int i = 0; i < 3000; ++i)
	{
		int r = qrand() % 255;
		int g = qrand() % 255;
		int b = qrand() % 255;		

        segColorTable_g.insert(i, cv::Vec3b(r, g, b));
	
	}

}

bool CAllSegs::GetSegs(QImage& segShow)
{
	if (image.empty()) return false;
	
	for (int i = 0; i < regCount; i++)
	{
		cv::Vec3b cr = segColorTable_g[i % 3000];
		for (std::vector<cv::Point>::iterator it=regCluster[i].begin(); it!=regCluster[i].end(); it++)
		{
			segShow.setPixel((*it).x, (*it).y, qRgb(cr[0], cr[1], cr[2]));
		}
	}

	return true;
}


void CAllSegs::SetParas( float _sigmaS, float _sigmaR, int _minSize, \
						int _basinvalue, int _threshold, int _wsvalue, \
						double _colorthreshold, double _areathreshold )
{
	ms_sigmaS		= _sigmaS;
	ms_sigmaR		= _sigmaR;
	ms_minSize		= _minSize;	
	ws_basinvalue	= _basinvalue;
	ws_threshold	= _threshold;
	ws_wsvalue		= _wsvalue;

	color_threshold = _colorthreshold;
	area_threshold	= _areathreshold;
}

bool CAllSegs::GetSimPat()
{
	simPat.clear();
	simPatArea.clear();
	simPatches.clear();
	simPatchesArea.clear();

	for(int i=0; i<regCount; i++)
	{
		std::set<int> tempSet;
		simPatches.push_back(tempSet);
		simPatchesArea.push_back(tempSet);
	}

	for (int index1=0; index1<regAverage.size(); index1++)
	{
		cv::Vec3d c1 = regAverage[index1];
		double a1 = (double)regCluster[index1].size();
		for (int index2=/*0*/ index1+1; index2!=index1&&index2<regAverage.size(); index2++)
		{
			cv::Vec3d c2 = regAverage[index2];
			double a2 = (double)regCluster[index2].size();
			double color_dis = std::sqrt(pow(c1[0]-c2[0],2)+pow(c1[1]-c2[1],2)+pow(c1[2]-c2[2],2));
			double area_dis = 0;
			if (a1 > a2)
				area_dis = a2 / a1;
			else
				area_dis = a1 / a2;
			if (color_dis<color_threshold)
			{
				//simPat.push_back((cv::Point(index1, index2)));
				simPatches[index1].insert(index2);
			}
			if (color_dis<color_threshold && area_dis>area_threshold)
			{
				simPat.push_back((cv::Point(index1, index2)));
				simPatArea.push_back(cv::Point(index1,index2));
				simPatchesArea[index1].insert(index2);
			}
		}
	}

	return true;

}
