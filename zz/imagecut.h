#ifndef IMAGECUT_H
#define IMAGECUT_H

// #include <QtCore>
// #include <QtGui>
// #include <QObject>

#include "meanshiftSegmentation/allsegs.h"
#include "abscutout.h"
#include "cutoutsettings.h"
#include <QSet>
#include "GMM/CmGMM_.h"

using namespace cv;
using namespace std;

class ImageCutOut : public AbsCutOut
{
public:
    ImageCutOut(const QImage&);

	~ImageCutOut();

	virtual void PreSegment();
	virtual void GraphCutSegment();
	virtual void InitGraphCut();
	virtual void AddDataTerms();
	virtual void AddSmoothTerms();
	virtual void SaveCutoutResults();	

	void InitBackgroundSamples();
	void UpdateBackgroundSamples();

    void SetSeedRegion(QPainterPath&);

	void GetContours();
	void GetCutOutResult();

	//void SetMeanShiftPara(int minSize, float sigmaS, float sigmaR, double cth);
	
	void ConstructGMM();

	//void SegmentShow();
	//void ImageSegmentation();

	void SignalSendSelectionResult();
	void SignalSendIsEdgeRefine(bool);

		void SlotGetStrokeWidth(int);
        // return is changed
        bool SlotGetPainterPath(QPainterPath&, bool);
        void SetSeedRegion(QSet<int>, bool _isFg);
        void SetFgRegion(QSet<int>);
		
		void SlotGetPaintPathBg(QPainterPath&, int);

		void SlotGetDrawTrimap(QPainterPath&);

		void SlotEdgeRefine();

public:

	CutOutSettings* paraSetting;
	
	Mat sourceImage;
    // source image
	QImage qSourceImage;
	QImage qTargetImage;

    // show source segments
	QImage sourceSegShow;
	QImage qBgStrokeMask;

	QString sourceName;

	Mat fgMask;
	QImage qFgMask;

	QVector<QRgb> colorTable;

	Mat alphaMatte;
	QImage qAlphaMatte;

	Mat blendTrimap;
	QVector<QPoint> innerBoundary;
	
	int widthImage;
	int heightImage;

	QPainter strokePainter;
	QPainterPath cutSelectPath;
    QPainterPath innerSelectPath;
    vector<vector<Point> > contours;

	bool isImageLoad;
	bool isPreSegOK;
	bool isSelectionOK;
	bool isFg;

	bool isEdgeRefined;

	bool isImage2Video;
	
    CAllSegs *pMsSeg;

	enum {FG_GMM_COMP=2, // 1-3
		  BG_GMM_COMP=4, // 3-6
		  NUM_BGSAMPLE_REGION=200};
	
	QSet<int> seedBg;
	QSet<int> seedRegion;
	QSet<int> sampleBg;
	QSet<int> regionFg;		
 		
	QVector<double> patchWeights;

	CmGMM3D *fgGMM;
	CmGMM3D *bgGMM;

};

// 		Mat strokeMask;
// 		Mat sourceMask;
// 		Mat sourceMatte;
// 		Mat backgroundMask;
// 		Mat foregroundMask;
// 		QImage qSourceImage;
// 		QImage qStrokeMask;
// 		QImage qSourceMask;	
// 		Mat source3f;
// 		Mat sourceSegIndex;
// 		QImage sourceSegShow;
// 
// 		int widthImage;
// 		int heightImage;
// 
// 		QMap<int, QRgb> colorMap;
// 
// 		QTime timeCount;

// 		bool isSegByMeanshift;
// 		bool isPreSegOK;
// 
// 		bool isPaintFg;
// 
// 		// for segment
// 		double sigma;
// 		double threshold;
// 		double minSize;
// 
// 		// for meanshift
// 		double cth;	
// 
// 		int sigmaS;
// 		float sigmaR;
// 		QPen strokePen;

//		bool isMeanshiftOK;

// for graphcut

// 		GraphType *graphcut;
// 
// 		double beta;
// 		double weight;
// 		double lamda;	




#endif // IMAGECUT_H
