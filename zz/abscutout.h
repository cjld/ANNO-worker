#ifndef ABSCUTOUT_H
#define ABSCUTOUT_H

#include "graphCut/graph.h"
#include <QTime>
#include <QImage>
#include <QPainter>

typedef Graph<double, double, double> GraphType;
// 	const double DBL_BIG = 10.0;
// 	const double e = 2.71828183;

class AbsCutOut
{
public:
	AbsCutOut();
	~AbsCutOut();

	virtual void PreSegment() = 0;
	virtual void GraphCutSegment() = 0;		
	virtual void InitGraphCut() = 0;
	virtual void AddDataTerms() = 0;
	virtual void AddSmoothTerms() = 0;
	virtual void SaveCutoutResults() = 0;
		
public:
	QTime timeCount;
	QImage qStrokeMask;

	QPainterPath cutPaintPath;
	QPen strokePen;
	int widthStroke;
	GraphType *graphcut;

//Mat strokeMask;		
// 		Mat fgMask;		
// 		QImage qFgMask;
		
// 		bool isPaintFg;
// 		bool isPreSegOK;	
	/*QPainterPath cutSelectPath;	*/
	// for meanShift
// 		double minSize;
// 		double cth;
// 		float sigmaS;
// 		float sigmaR;
	// for graphcut
// 		Energy::Var *gcResult;
// 		Energy *egy;
	//double weight;
// 		double beta;		
// 		double lamda;
// 
// 		double DBL_BIG;
// 		double e;
// 		double gcEPSILON;
	//QMap<int, QRgb> colorMap;

};





#endif // ABSCUTOUT_H
