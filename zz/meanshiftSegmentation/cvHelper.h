#ifndef CV_HELPER_H
#define CV_HELPER_H

#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <iostream>

#define ErrorMessage()														\
	{																		\
	std::cout<<"Error in function "<<__FUNCTION__<<" : file "<<__FILE__<<" : line "<<__LINE__<<std::endl; \
	exit(1);																\
	}		
#define WarningMessage()													\
	{																		\
		std::cout<<"Warning in function "<<__FUNCTION__<<" : file "<<__FILE__<<" : line "<<__LINE__<<std::endl; \
	}


namespace CvHelper
{
	/************************************************************************/
	/* Define Macros                                                        */
	/************************************************************************/
#define CvAssertM(msg, Condition)                                        \
	{                                                                       \
		if( !(Condition) )                                                  \
		{																	\
			cvError(CV_StsInternal, __FUNCTION__,  "Assertion: " #msg #Condition " failed", __FILE__, __LINE__); \
			exit(1);														\
		}																	\
	}


	/************************************************************************/
	/* Randomly generate RGB color											*/
	/************************************************************************/
	inline CvScalar RandColor()
	{
		CvScalar c;
		srand( (unsigned)(time(NULL) + rand()));
		c.val[0] = rand()%255;
		srand( (unsigned)(time(NULL) + rand()));
		c.val[1] = rand()%255;
		srand( (unsigned)(time(NULL) + rand()));
		c.val[2] = rand()%255;

		return c;
	}

	/************************************************************************/
	/* Display image by window												*/
	/************************************************************************/
	inline void DisplayImage(const IplImage* img, const char* title, int wait=0)
	{
		cvNamedWindow(title);
		cvShowImage(title, img);
		cvWaitKey(wait);
		cvDestroyWindow(title);
	}

	class ExhibitWins
	{
		public:
			ExhibitWins(int flags = CV_WINDOW_AUTOSIZE)
			{
				_win_number = 0;
				_win_flags = flags;
			}
			~ExhibitWins()
			{
				cvWaitKey();
				Close();
			}
			inline void Close(void)
			{
				for (std::vector<std::string>::iterator it=m_names.begin(); it!=m_names.end(); it++)
				{
					cvDestroyWindow(it->c_str());
				}
			}
			inline void Show(const CvArr* img, char* title = NULL, int wait = 10)
			{
				_win_number++;
				std::string name;
				if (title==NULL)
				{
					char name_s[256];
                    sprintf(name_s, "figure %d", _win_number);
					name = std::string(name_s);
				}
				else
					name = std::string(title);
				m_names.push_back(name);
				cvNamedWindow(name.c_str(), _win_flags);
				cvShowImage(name.c_str(), img);
				cvWaitKey(wait);
			}

			inline void SoleShow(const CvArr* img, char* title = NULL, int wait = -1)
			{
				_win_number++;
				std::string name;
				if (title==NULL)
				{
					char name_s[256];
                    sprintf(name_s, "figure %d", _win_number);
					name = std::string(name_s);
				}
				else
					name = std::string(title);
				m_names.clear();
				m_names.push_back(name);
				cvNamedWindow(name.c_str(), _win_flags);
				cvShowImage(name.c_str(), img);
				cvWaitKey(wait);
			}

		private:
			std::vector<std::string> m_names;
			int _win_number;
			int _win_flags;
	};

	/************************************************************************/
	/* Mat2GrayLinear: Linearly convert and 32FC1 mat to gray image for		*/
	/*     display. The result image is in 32FC1 format and range [0, 1.0]  */
	/* buffer32FC1: A buffer can be supplied to avoid memory allocate.		*/
	/************************************************************************/
	inline IplImage* Mat2GrayLinear(const IplImage* mat32FC1, IplImage* buffer32FC1 /* = NULL */)
	{
		if (buffer32FC1 == NULL)
		{
			buffer32FC1 = cvCreateImage(cvGetSize(mat32FC1), IPL_DEPTH_32F, 1);
		}
		else
		{
			CvAssertM("", mat32FC1->depth == IPL_DEPTH_32F && mat32FC1->nChannels == 1);
		}

		double minVal = 0, maxVal = 0;
		cvMinMaxLoc(mat32FC1, &minVal, &maxVal );
		CvAssertM("", maxVal - minVal > 1e-6);
		//double scale = 1.0/(maxVal - minVal);
		double scale = 1.0/(maxVal);
		//double shift = -minVal * scale;
		double shift = 0;

		cvConvertScale(mat32FC1, buffer32FC1, scale, shift);
		return buffer32FC1;
	}

	inline IplImage* Mat2GrayLinear(const CvMat* mat32FC1, IplImage* buffer32FC1 /* = NULL */)
	{
		if (buffer32FC1 == NULL)
		{
			buffer32FC1 = cvCreateImage(cvGetSize(mat32FC1), IPL_DEPTH_32F, 1);
		}
		else
		{
			CvAssertM("", mat32FC1->type & IPL_DEPTH_32F);
		}

		double minVal = 0, maxVal = 0;
		cvMinMaxLoc(mat32FC1, &minVal, &maxVal );
		CvAssertM("", maxVal - minVal > 1e-6);
		//double scale = 1.0/(maxVal - minVal);
		double scale = 1.0/(maxVal);
		//double shift = -minVal * scale;
		double shift = 0;

		cvConvertScale(mat32FC1, buffer32FC1, scale, shift);
		return buffer32FC1;
	}


	/************************************************************************/
	/* Scaloar2Color: Linearly convert a scalar to RGB color				*/
	/************************************************************************/
	inline void HSVtoRGB(double *r, double *g, double *b, double h, double s, double v )
	{
		int i;
		float f, p, q, t;
		if( s == 0 ) {
			// achromatic (grey)
			*r = *g = *b = v;
			return;
		}
		h /= 60;			// sector 0 to 5
		i = (int)floor( h );
		f = (float)h - i;			// factorial part of h
		p = (float)v * ( 1 - s );
		q = (float)v * ( 1 - s * f );
		t = (float)v * ( 1 - s * ( 1 - f ) );
		switch( i ) {
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
		}
	}

	inline CvScalar Scalar2Color(double val, double minv, double maxv)
	{
		CvScalar color;
		double r, g, b, h, s = 0.8, v = 1;
		double cval = (val - minv) / (maxv - minv);
		double highh = 233;   // h 0~350

		h = highh - highh*cval;
		v = 0.5 + cval*(1-0.5);
		HSVtoRGB(&r, &g, &b, h, s, v);
		color.val[2] = r*255;
		color.val[1] = g*255;
		color.val[0] = b*255;

		return color;
	}


	/************************************************************************/
	/* ExtractContour: Find contour from a binary image	(OpenCV)			*/
	/************************************************************************/
	inline std::vector< std::vector<CvPoint> > ExtractContour(const IplImage* pMask)
	{
		IplImage* pBinaMask = cvCloneImage(pMask);
		CvSeq* contours = NULL;
		CvMemStorage* storage = cvCreateMemStorage(0);

		std::vector< std::vector<CvPoint> > ALLContours;
		int nContour = cvFindContours( pBinaMask, storage, &contours, sizeof(CvContour), CV_RETR_LIST,
			CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );


		for (CvSeq* c = contours; c!=NULL; c=c->h_next)
		{
			int count = c->total;

			count -= !CV_IS_SEQ_CLOSED (c);
			std::vector<CvPoint> pts;
			for (int i=0; i<count; i++)
			{
				//CvPoint* pt = (CvPoint*)cvGetSeqElem(c, i);
				//pts.push_back(cvPoint(pt->x, pt->y));
				pts.push_back(cvPoint(((CvPoint*)cvGetSeqElem(c, i))->x, ((CvPoint*)cvGetSeqElem(c, i))->y));
			}
			ALLContours.push_back(pts);
		}

		cvReleaseMemStorage(&storage);
		cvReleaseImage(&pBinaMask);
		return ALLContours;
	}


	/************************************************************************/
	/* ExtractContour: Find contour from a binary image	(OpenCV)			*/
	//返回提取到的轮廓中的点数最多的一个轮廓
	/************************************************************************/
	inline std::vector<CvPoint> ExtractOuterContour(IplImage* pMask)
	{
		IplImage* pBinaMask = cvCloneImage(pMask);
		CvSeq* contours = NULL;
		CvMemStorage* storage = cvCreateMemStorage(0);

		std::vector< std::vector<CvPoint> > ALLContours;
		int nContour = cvFindContours( pBinaMask, storage, &contours, sizeof(CvContour), CV_RETR_EXTERNAL,
			CV_CHAIN_APPROX_NONE, cvPoint(0,0) );


		for (CvSeq* c = contours; c!=NULL; c=c->h_next)
		{
			int count = c->total;

			count -= !CV_IS_SEQ_CLOSED (c);
			std::vector<CvPoint> pts;
			for (int i=0; i<count; i++)
			{
				//CvPoint* pt = (CvPoint*)cvGetSeqElem(c, i);
				//pts.push_back(cvPoint(pt->x, pt->y));
				pts.push_back(cvPoint(((CvPoint*)cvGetSeqElem(c, i))->x, ((CvPoint*)cvGetSeqElem(c, i))->y));
			}
			ALLContours.push_back(pts);
		}
		//寻找所有轮廓中的最大轮廓
		std::vector<CvPoint> maxContour;
		if (ALLContours.size()>0)
		{
			for (unsigned int i=0; i<ALLContours.size(); i++)
			{
				if(ALLContours[i].size()>maxContour.size())
				{
					maxContour.clear();
					maxContour = ALLContours[i];
				}
			}
		}

		cvReleaseMemStorage(&storage);
		cvReleaseImage(&pBinaMask);
		return maxContour;
	}


	/************************************************************************/
	/* ExtractContour: Find contour from a binary image	(OpenCV)			*/
	/* 增加了形态学处理														*/
	/************************************************************************/
	inline std::vector< std::vector<CvPoint> > ExtractContourModified(IplImage* pMask)
	{
		cvErode(pMask, pMask, 0, 1);
		cvDilate(pMask, pMask, 0, 1);

		IplImage* pBinaMask = cvCloneImage(pMask);
		CvSeq* contours = NULL;
		CvMemStorage* storage = cvCreateMemStorage(0);

		std::vector< std::vector<CvPoint> > ALLContours;
		int nContour = cvFindContours( pBinaMask, storage, &contours, sizeof(CvContour), CV_RETR_EXTERNAL,
			CV_CHAIN_APPROX_NONE, cvPoint(0,0) );


		for (CvSeq* c = contours; c!=NULL; c=c->h_next)
		{
			int count = c->total;

			count -= !CV_IS_SEQ_CLOSED (c);
			std::vector<CvPoint> pts;
			for (int i=0; i<count; i++)
			{
				CvPoint* pt = (CvPoint*)cvGetSeqElem(c, i);
				pts.push_back(cvPoint(pt->x, pt->y));
			}
			ALLContours.push_back(pts);
		}

		cvReleaseMemStorage(&storage);
		cvReleaseImage(&pBinaMask);
		return ALLContours;
	}

	/************************************************************************/
	/* TrackContour: Track contour from a binary image by edge tracing		*/
	/************************************************************************/
	inline std::vector<CvPoint> TrackContour(IplImage* image)
	{
		std::vector<CvPoint> pt;
		int height = image->height;
		int width = image->width;
		//寻找开始点，最右，最下
		//先行，后列
		int i,j;
		CvPoint startPt, currPt;

		for (i = 0; i < height; ++i)
		{
			for (j = 0; j < width; ++j)
			{
				// 轮廓颜色判断
				if (CV_IMAGE_ELEM(image, uchar, i, j) == 255)
				{
					startPt.x = i;
					startPt.y = j;

					i = height;
					j = width;
				}
			}
		}
		//设置方向数组
		int direction[8][2] = {{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0}};
		int startDirect ;
		bool FindStartPoint = false;
		bool FindPoint = false;
		//开始跟踪
		currPt.x = startPt.x;
		currPt.y = startPt.y;
		CvPoint NextPt ;
		int NextPtValue;
		startDirect = 0;
		int cnt= 0;
		while(!FindStartPoint/*&&cnt<100000*/)
		{
            FindPoint = false;
			pt.push_back(cvPoint(currPt.y, currPt.x));
			while(!FindPoint)
			{
				//沿预定方向扫描，寻找下一个点
				NextPt.x = currPt.x + direction[startDirect][1];
				NextPt.y = currPt.y + direction[startDirect][0];

				//NextPtValue = CV_IMAGE_ELEM(image, uchar, height-1-NextPt.x, NextPt.y);
				NextPtValue = CV_IMAGE_ELEM(image, uchar, NextPt.x, NextPt.y);
				// 轮廓颜色判断
				if (NextPtValue == 255 && NextPt.x >= 0 && NextPt.x < height && NextPt.y >= 0 && NextPt.y < width)
				{
                    FindPoint = true;
					currPt.x = NextPt.x;
					currPt.y = NextPt.y;
					if (currPt.x == startPt.x&&currPt.y == startPt.y)
					{
                        FindStartPoint = true;
					}

					startDirect = startDirect-2;

					if(startDirect<0) startDirect = startDirect+8;
				}
				else
				{
					startDirect = startDirect+1;
					if (startDirect>=8) startDirect = startDirect-8;
				}
			}
			//cnt ++;
		}

		return pt;
	}

	/************************************************************************/
	/* Draw contours														*/
	/************************************************************************/
	inline void DrawContour(IplImage* pImg, const std::vector<CvPoint>& Contours, CvScalar& color)
	{
		CvPoint* AllPts = new CvPoint [Contours.size()];
		int ConNum = Contours.size();			
		for (unsigned int ic=0; ic<Contours.size(); ic++)
		{
			AllPts[ic].x = Contours[ic].x;
			AllPts[ic].y = Contours[ic].y;
		}

		cvPolyLine(pImg, &AllPts, &ConNum, 1, true, color, 2, CV_AA);
		delete []AllPts;
	}


	/************************************************************************/
	/* Fill region enclosed with contour									*/
	/************************************************************************/
	inline void FillRegionInContour(IplImage* pImg, const std::vector<CvPoint>& Contours, CvScalar& color)
	{
		CvPoint* AllPts = new CvPoint [Contours.size()];
		int ConNum = Contours.size();			
		for (unsigned int ic=0; ic<Contours.size(); ic++)
		{
			AllPts[ic].x = Contours[ic].x;
			AllPts[ic].y = Contours[ic].y;
		}

		cvFillPoly(pImg, &AllPts, &ConNum, 1, color);
		delete []AllPts;
	}

	/************************************************************************/
	/* Fill region enclosed with contour									*/
	/************************************************************************/
	inline bool	SetImagePixel(IplImage *pImg, uchar c)
	{
		//Check the point

		uchar *data = (uchar*)pImg->imageData;
		if (pImg->nChannels == 1)
		{
			for (int h=0; h<pImg->height; h++)
			{
				for (int w=0; w<pImg->width; w++)
				{
					data[h*pImg->widthStep+w] = c;
				}
			}
		}
		else if (pImg->nChannels == 3)
		{
			for (int h=0; h<pImg->height; h++)
			{
				for (int w=0; w<pImg->width; w++)
				{
					data[h*pImg->widthStep+w*pImg->nChannels+0] = c;
					data[h*pImg->widthStep+w*pImg->nChannels+1] = c;
					data[h*pImg->widthStep+w*pImg->nChannels+2] = c;
				}
			}
		}
		else if (pImg->nChannels == 4)
		{
			for (int h=0; h<pImg->height; h++)
			{
				for (int w=0; w<pImg->width; w++)
				{
					data[h*pImg->widthStep+w*pImg->nChannels+0] = c;
					data[h*pImg->widthStep+w*pImg->nChannels+1] = c;
					data[h*pImg->widthStep+w*pImg->nChannels+2] = c;
					data[h*pImg->widthStep+w*pImg->nChannels+3] = c;
				}
			}
		}
		else
		{
			WarningMessage();
			return false;
		}

		return true;
	}

	inline char * GetTime( void )
	{
		time_t now;
		struct tm *timeNow;
		time(&now);
		timeNow = localtime(&now);
		char *cTime = new char[1024];
        sprintf(cTime, "%d_%d_%d_%d-%d-%d", timeNow->tm_year+1900, \
				timeNow->tm_mon+1, timeNow->tm_mday, timeNow->tm_hour, \
				timeNow->tm_min, timeNow->tm_sec);

		return cTime;
	}

	inline	double	powDist(const CvPoint& A, const CvPoint& B)
	{
		return  (double)((A.x - B.x)*(A.x - B.x) + (A.y - B.y)*(A.y - B.y));
	}
	inline bool GetSampling( std::vector<CvPoint> &refPntSet, const int sampleNum )
	{
		if (refPntSet.size() < (size_t)sampleNum)
			return false;

		int	startSize = std::min(int(refPntSet.size()), sampleNum * 3);

		std::vector<int>	idlist(refPntSet.size());
		std::vector<bool>	alive(refPntSet.size(), true);

		for (unsigned int i = 0; i < idlist.size(); i ++)
			idlist[i] = i;

		std::random_shuffle(idlist.begin(), idlist.end());

		std::vector<std::pair<double, std::pair<int,int>>>	pwDist(startSize * (startSize - 1) / 2);
		int	cc = 0;

		for (int i = 0; i < startSize; i ++)
			for (int j = i + 1; j < startSize; j ++)
				pwDist[cc ++] = std::make_pair(powDist(refPntSet[idlist[i]], refPntSet[idlist[j]]), std::make_pair(idlist[i], idlist[j]));
		std::sort(pwDist.begin(), pwDist.end());

		for (int nodeLeft = startSize, i = 0; nodeLeft > sampleNum && i < cc; i ++)
		{
			if (alive[pwDist[i].second.first] && alive[pwDist[i].second.second])
				alive[pwDist[i].second.first] = false, nodeLeft --;
		}

		std::vector<CvPoint> pntSetNew(sampleNum);

		cc = 0;
		for (int i = 0; i < startSize; i ++)
		{
			if (alive[idlist[i]])
				pntSetNew[cc++] = refPntSet[idlist[i]];
		}

		refPntSet.clear();
		refPntSet = pntSetNew;

		return true;
	}
}
#endif
