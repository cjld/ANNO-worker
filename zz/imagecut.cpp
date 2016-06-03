#include "imagecut.h"
#include <QDebug>
#include "alphamatting.h"

ImageCutOut::~ImageCutOut()
{
	if (pMsSeg)
	{
		delete pMsSeg;
		pMsSeg = NULL;
	}

	if (fgGMM)
	{
		delete fgGMM;
		fgGMM = NULL;
	}

	if (bgGMM)
	{
		delete bgGMM;
		bgGMM = NULL;
	}

}

ImageCutOut::ImageCutOut(const QImage &_image)
{

	pMsSeg = NULL;
	isPreSegOK = false;
	isSelectionOK = false;
	isImageLoad = false;
	isFg = true;

	isImage2Video = false;

    qSourceImage = _image.convertToFormat(QImage::Format_RGB888);
    sourceImage = Mat(qSourceImage.height(), qSourceImage.width(), CV_8UC3, qSourceImage.bits(), qSourceImage.bytesPerLine());

	widthImage = sourceImage.size().width;
	heightImage = sourceImage.size().height;				

	sourceSegShow = QImage(widthImage, heightImage, QImage::Format_RGB888);
	
   	for (int i = 0; i < 256; ++i) 
		colorTable.push_back(qRgb(i, i, i));	
	
	fgMask = Mat(sourceImage.size(), CV_8UC1);
 	qFgMask = QImage(fgMask.data, widthImage, heightImage, fgMask.step, QImage::Format_Indexed8);
 	qFgMask.fill(0);
	   	
   	qFgMask.setColorTable(colorTable);
		
	qStrokeMask = QImage(widthImage, heightImage, QImage::Format_ARGB32_Premultiplied);
	qStrokeMask.fill(0);
	strokePainter.begin(&qStrokeMask);

	alphaMatte = Mat(sourceImage.size(), CV_8UC1);
	alphaMatte = alphaMatte.zeros(sourceImage.size(), CV_8UC1);
	
	qBgStrokeMask = QImage(widthImage, heightImage, QImage::Format_ARGB32_Premultiplied);
	qBgStrokeMask.fill(0);

	widthStroke = 10;
	strokePen.setCapStyle(Qt::RoundCap);
	strokePen.setColor(Qt::white);
	strokePen.setWidth(widthStroke);

	fgGMM = new CmGMM3D(FG_GMM_COMP);
	bgGMM = new CmGMM3D(BG_GMM_COMP);
 		
	graphcut = NULL;		
	paraSetting = NULL;

	isEdgeRefined = false;
	regionFg.clear();

}

void ImageCutOut::PreSegment()
{
	if (!isImageLoad)
	{
		qDebug() << "the image has not been loaded";
		return;
	}

	if (isPreSegOK)
	{
		qDebug() << "the image has been pre-segmented";
		return;
	}

	if (pMsSeg)
	{
		delete pMsSeg;
		pMsSeg = NULL;			
	}
	
	pMsSeg = new CAllSegs();
// 	cv::Mat sourceLab;
// 	cvtColor(sourceImage, sourceLab, CV_BGR2Lab);
	pMsSeg->InitSegs(sourceImage, pMsSeg->MS_MODE);
	
// 	qDebug() << "xx paraSetting->sigmaS = " << paraSetting->sigmaS
// 			<< "xx paraSetting->sigmaR = " << paraSetting->sigmaR
// 			<< "xx paraSetting->minSize = " << paraSetting->minSize
// 			<< "xx paraSetting->cth = " << paraSetting->cth;

 	pMsSeg->SetParas(paraSetting->sigmaS, paraSetting->sigmaR, 
 					 paraSetting->minSize, 0, 0, 0, paraSetting->cth, 0.0);

    pMsSeg->RunSeg();

	//LoadSegment();
	pMsSeg->GetSegs(sourceSegShow);
	qDebug() << "num of seg is: " << pMsSeg->regCount; 
	InitBackgroundSamples();
		
	timeCount.start();
		
	InitGraphCut();

	qDebug() << "InitGraphCut() cost = " << timeCount.elapsed();

	patchWeights.clear();
	int pixNum = widthImage * heightImage;
	for (int i = 0; i < pMsSeg->regCount; ++i)
	{
		int numPt = pMsSeg->regCluster[i].size();
		double wt = (double)numPt / (double)(pixNum);
		patchWeights.append(wt);
	}		
		
	isPreSegOK = true;		
}

void ImageCutOut::InitGraphCut()
{
 	if (graphcut)
 	{
 		delete graphcut;
 		graphcut = NULL;
 	}

	int nodeNum = pMsSeg->regCount;
	graphcut = new GraphType(nodeNum, nodeNum * 4);
	if (!graphcut)
	{
		qDebug() << "fail to creat graph for graphcut";
	}
}

void ImageCutOut::GraphCutSegment()
{		
 	graphcut->reset(); // delete all the edges and nodes
  	graphcut->add_node(pMsSeg->regCount);
  	
	AddSmoothTerms();
	//InitGraphCut();
	AddDataTerms();

 	graphcut->maxflow();	

}

void ImageCutOut::AddDataTerms()
{		
// 		seedFg.clear();
// 		seedBg.clear();
// 		
// 		QRectF pathRect = cutSelectPath.boundingRect();	
// 		int _top = (pathRect.top() - widthStroke) >= 0 ? (pathRect.top() - widthStroke) : 0;
// 		int _bottom = (pathRect.bottom() + widthStroke) < heightImage ?  (pathRect.bottom() + widthStroke) : heightImage - 1;
// 		int _left = (pathRect.left() - widthStroke) >= 0  ? (pathRect.left() - widthStroke) : 0;
// 		int _right = (pathRect.right() + widthStroke) < widthImage ? (pathRect.right() + widthStroke) : widthImage - 1;
// 		
// 		qStrokeMask.copy(_left, _top, _right - _left, _bottom - _top).save(imageDir + "xxx.bmp");
// 
// 		for (int h = _top; h <= _bottom; ++h)
// 		{
// 			QRgb* scanelineStrokeMask = (QRgb*)qStrokeMask.scanLine(h);
// 
// 			for (int w = _left; w <= _right; ++w)
// 			{
// 				int index = h * widthImage + w;
// 
// 				int grayValue = qGray(scanelineStrokeMask[w]);				
// 
// 				if (200 == grayValue)
// 				{					
// 					seedFg.insert(pMsSeg->labels[index]);
// 				}
// 				else if (100 == grayValue)
// 				{
// 					seedBg.insert(pMsSeg->labels[index]);
// 				}				
// 
// 			} // for w
// 
// 		} // for h

#if 1

	ConstructGMM();

	//definite background
	if (!seedBg.isEmpty())
	{
		foreach (const int &valuebg, seedBg)
		{
			graphcut->add_tweights(valuebg, 0, DBL_MAX);
		}
	}		
		
	// definite foreground
	foreach (const int &values, seedRegion)
	{			
		//if (isFg)
		graphcut->add_tweights(values, DBL_MAX, 0);	
		// 			else
		// 			{
		// 				//qDebug() << "bg values = " << values;
		// 				graphcut->add_tweights(values, 0, DBL_MAX);
		// 			}
	}
		
	// other area
	for (int i = 0; i < pMsSeg->regCount; ++i)
	{
// 			if (sampleBg.contains(i) || seedRegion.contains(i))
// 			{
// 				graphcut->add_tweights(i, 0, 0);
// 				continue;
// 			}

		if (seedRegion.contains(i) || seedBg.contains(i))
		{
			continue;
		}

		double fProp = fgGMM->P(pMsSeg->regAverage[i]);
		double bProp = bgGMM->P(pMsSeg->regAverage[i]);

        graphcut->add_tweights(i, -log(bProp), -log(fProp));
        //qDebug() << "add tweights" << i << -log(bProp) << -log(fProp) << bProp << fProp;

	}

#endif
	

// 		foreach (const int &values, seedBg)
// 		{
// 			//egy->add_term1(gcResult[values], 0, DBL_MAX);
// 			graphcut->add_tweights(values, 0, DBL_MAX);			
// 		}

}

void ImageCutOut::AddSmoothTerms()
{
	//int nEdges = 0;

	//qDebug() << "paraSetting->lamda = " << paraSetting->lamda;

	for (int rIdx = 0; rIdx < pMsSeg->regCount; rIdx++)
	{
		//nEdges += pMsSeg->regNeighborhood[rIdx].size();
		cv::Vec3d c = pMsSeg->regAverage[rIdx];
		for (std::set<int>::iterator it = pMsSeg->regNeighborhood[rIdx].begin(); 
			it != pMsSeg->regNeighborhood[rIdx].end(); it++)
		{
			if (*it > rIdx)//·ÀÖ¹ÖØ¸´¼Ó±ß
			{
				cv::Vec3d nd = pMsSeg->regAverage[*it];
				double dis = std::sqrt(pow(c[0]-nd[0], 2) + pow(c[1]-nd[1], 2) + pow(c[2]-nd[2], 2));
				//float w = paraSetting->lamda*pow(CutOutSettings::myE, -paraSetting->beta*dis + CutOutSettings::gcEPSILON);

				float w = paraSetting->lamda * 1.0 / (paraSetting->beta*dis + CutOutSettings::gcEPSILON);

				//egy->add_term2__(gcResult[rIdx], gcResult[*it], w, w);

				//egy->add_term2(gcResult[rIdx], gcResult[*it], 0, w, w, 0);
				//egy->add_edge(rIdx, *it, w, w);
				graphcut->add_edge(rIdx, *it, w, w);
                //qDebug() << "add edge" << rIdx << *it << w << dis;
				
			}

		}

	}

}

void ImageCutOut::SaveCutoutResults()
{

}

void ImageCutOut::GetContours()
{
	Mat grayMask;
	fgMask.copyTo(grayMask);

	vector<Vec4i> hierarchy;
    contours.clear();

	findContours(grayMask, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

	cutSelectPath = QPainterPath();

	QPainterPath tempPath;
	int max_length = 0;
	int maxIndex = 0;
	for (int i = 0; i < contours.size(); ++i)
	{
		if (contours[i].size() > max_length)
		{
			max_length = contours[i].size();
			maxIndex = i;
		}

	}

	innerBoundary.clear();
	for (int i = 0; i < contours.size(); i++)
	{
		cutSelectPath.moveTo(QPoint(contours[i][0].x, contours[i][0].y));

		if (i == max_length)
		{
			tempPath.moveTo(QPoint(contours[i][0].x, contours[i][0].y));
		}

		for (vector<Point>::iterator it = contours[i].begin(); it != contours[i].end(); it++)
		{
			cutSelectPath.lineTo(QPoint((*it).x, (*it).y));

			if (i == maxIndex)
			{
				innerBoundary.append(QPoint((*it).x, (*it).y));
				tempPath.lineTo(QPoint((*it).x, (*it).y));
			}

		}

		if (i == maxIndex)
		{
			innerBoundary.append(QPoint(contours[maxIndex][0].x, contours[maxIndex][0].y));
			tempPath.lineTo(QPoint(contours[maxIndex][0].x, contours[maxIndex][0].y));
		}

		cutSelectPath.lineTo(QPoint(contours[i][0].x, contours[i][0].y));
	}

	innerSelectPath = tempPath;
	//OptBoundary::LinetoPoint(innerBoundary, qSourceImage.width(),  qSourceImage.height());

}

void ImageCutOut::GetCutOutResult()
{		
	for (int index = 0; index < pMsSeg->regCount; ++index)
	{

// 			if (isFg)
// 			{
// 				if (graphcut->what_segment(index) == GraphType::SOURCE)
// 				{
// 					for (std::vector<cv::Point>::iterator it = pMsSeg->regCluster[index].begin(); 
// 						it != pMsSeg->regCluster[index].end(); it++)
// 					{
// 						fgMask.at<uchar>(Point((*it).x, (*it).y)) = 255;
// 					}
// 				}
// 				else
// 				{
// 					for (std::vector<cv::Point>::iterator it = pMsSeg->regCluster[index].begin(); 
// 						it != pMsSeg->regCluster[index].end(); it++)
// 					{
// 						fgMask.at<uchar>(Point((*it).x, (*it).y)) = 255;
// 					}
// 				}
// 			}
// 			else
// 			{
// 				if (graphcut->what_segment(index) == GraphType::SOURCE)
// 				{
// 					for (std::vector<cv::Point>::iterator it = pMsSeg->regCluster[index].begin(); 
// 						it != pMsSeg->regCluster[index].end(); it++)
// 					{
// 						fgMask.at<uchar>(Point((*it).x, (*it).y)) = 255;
// 					}
// 				}
// 				else
// 				{
// 					for (std::vector<cv::Point>::iterator it = pMsSeg->regCluster[index].begin(); 
// 						it != pMsSeg->regCluster[index].end(); it++)
// 					{
// 						fgMask.at<uchar>(Point((*it).x, (*it).y)) = 0;
// 					}
// 				}
// 			}
			
		if (graphcut->what_segment(index) == GraphType::SOURCE)
		{
			// when we paint the foreground
			if (isFg)
			{
				// when the patch is not in the fg, add it
				if (!regionFg.contains(index))
				{
					regionFg.insert(index);
					for (std::vector<cv::Point>::iterator it = pMsSeg->regCluster[index].begin();
						it != pMsSeg->regCluster[index].end(); it++)
					{
						fgMask.at<uchar>(Point((*it).x, (*it).y)) = 255;
					}

				}

			}
			else // when we paint the background
			{
				// when the patch is in the fg, remove it
				if (regionFg.contains(index))
				{
					regionFg.remove(index);
					for (std::vector<cv::Point>::iterator it = pMsSeg->regCluster[index].begin(); 
						it != pMsSeg->regCluster[index].end(); it++)
					{
						fgMask.at<uchar>(Point((*it).x, (*it).y)) = 0;
					}

				}

			} // else background				

		} // if graphcut
		else
		{
			// when we paint the foreground
// 				if (isFg)
// 				{
// 					// when the patch is not in the fg, add it
// 					if (!regionFg.contains(index))
// 					{			
// 						regionFg.remove(index);
// 						for (std::vector<cv::Point>::iterator it = pMsSeg->regCluster[index].begin(); 
// 							it != pMsSeg->regCluster[index].end(); it++)
// 						{
// 							fgMask.at<uchar>(Point((*it).x, (*it).y)) = 0;
// 						}
// 
// 					}
// 
// 				}
// 				else // when we paint the background
// 				{
// 					// when the patch is in the fg, remove it
// 					if (regionFg.contains(index))
// 					{				
// 						regionFg.remove(index);
// 						for (std::vector<cv::Point>::iterator it = pMsSeg->regCluster[index].begin(); 
// 							it != pMsSeg->regCluster[index].end(); it++)
// 						{
// 							fgMask.at<uchar>(Point((*it).x, (*it).y)) = 0;
// 						}
// 
// 					}
// 
// 				} // else background
			
		}

	} // for

}

void ImageCutOut::InitBackgroundSamples()
{
	sampleBg.clear();

	int bgPatchSize = pMsSeg->regCount / 4;
		
	while (sampleBg.size() < bgPatchSize)
	{
		int index = rand() % (pMsSeg->regCount);
		sampleBg.insert(index);
	}

	//Mat background64FC3(1, NUM_BGSAMPLE_REGION, CV_64FC3, samples);

}

void ImageCutOut::UpdateBackgroundSamples()
{
	// remove the patches that become background and seeds
	foreach (const int &values, sampleBg)
	{
		if (regionFg.contains(values) || seedRegion.contains(values))
		{
			sampleBg.remove(values);
		}
	}

	// when the number is not enough, sample new points
	int seedSize = seedRegion.size();

	foreach (const int &values, seedRegion)
	{
		if (regionFg.contains(values))
		{
			--seedSize;
		}
	}

	int bgPatchSize = pMsSeg->regCount - regionFg.size() - seedSize;
	
	// when the remained background samples is not enough
	if (bgPatchSize < 50)
		return;
	
	int sampledBgSize = bgPatchSize / 3;
		
	if (sampleBg.size() >= sampledBgSize)
	{
		return;
	}
	else
	{
		while (sampleBg.size() < sampledBgSize)
		{
			int index = rand() % pMsSeg->regCount;
			if (regionFg.contains(index) || seedRegion.contains(index) || 
				sampleBg.contains(index))
			{
				continue;
			}
			else
				sampleBg.insert(index);
		}

	}	

// 	foreach (const int &values, sampleBg)
// 	{
// // 			if (!isFg) // stroke is background
// // 			{
// // 				if (seedRegion.contains(values) && !sampleBg.contains(values))
// // 				{
// // 					sampleBg.insert(values);
// // 				}
// // 			}
// // 			else // stroke is foreground
// 		
// 		if (regionFg.contains(values) || seedRegion.contains(values))
// 		{
// 			sampleBg.remove(values);
// 
// 			int index = rand() % (pMsSeg->regCount);
// 			while (regionFg.contains(index) || seedRegion.contains(index) || 
// 				   sampleBg.contains(index))
// 			{
// 				index = rand() % (pMsSeg->regCount);				
// 			}
// 
// 			sampleBg.insert(index);
// 
// 		}						
// 			
// 	}		

}

void ImageCutOut::ConstructGMM()
{		
	UpdateBackgroundSamples();

	// the background GMM is constructed by the randomly sampled regions		
	QVector<Vec3d> Seeds;
	QVector<Vec3d> BG;
	QVector<double> vecWeight;
	Mat compli;	

//		the foreground GMM is constructed by the seedRegion seeds
//		Mat weightPatchFg(1, Seeds.size(), CV_64FC1, )
// 		int sumOfPix = 0;
// 		foreach (const int &values, seedRegion)
// 		{
// 			sumOfPix += pMsSeg->regCluster[values].size();			
// 		}

	// build the foreground GMM

	vecWeight.clear();
	foreach (const int &values, seedRegion)
	{
		//double rates = (double)(pMsSeg->regCluster[values].size()) / sumOfPix; 
		vecWeight.append(patchWeights[values]);
		Seeds.append(pMsSeg->regAverage[values]);
	}

    if (vecWeight.size()) {
        Mat matWeightFg(1, Seeds.size(), CV_64FC1, &vecWeight[0]);
        Mat SeedSamples(1, Seeds.size(), CV_64FC3, &Seeds[0]);
        fgGMM->BuildGMMs(SeedSamples, compli, matWeightFg);
        fgGMM->RefineGMMs(SeedSamples, compli, matWeightFg);
    } else {
        fgGMM->Reset();
    }
			
// 		sumOfPix = 0;
// 		foreach (const int &values, sampleBg)
// 		{
// 			sumOfPix += pMsSeg->regCluster[values].size();			
// 		}

	// build the background GMM
	vecWeight.clear();
	foreach (const int &values, sampleBg)
	{
		//double rates = (double)(pMsSeg->regCluster[values].size()) / sumOfPix; 
		vecWeight.append(patchWeights[values]);
		BG.append(pMsSeg->regAverage[values]);
	}

    if (vecWeight.size()) {
        Mat matWeightBg(1, sampleBg.size(), CV_64FC1, &vecWeight[0]);
        Mat BgSamples(1, sampleBg.size(), CV_64FC3, &BG[0]);
        bgGMM->BuildGMMs(BgSamples, compli, matWeightBg);
        bgGMM->RefineGMMs(BgSamples, compli, matWeightBg);
    } else {
        bgGMM->Reset();
    }

	//CmGMM3D::View(*fgGMM, "foreground");
	//CmGMM3D::View(*bgGMM, "background");

}

void ImageCutOut::SetSeedRegion(QPainterPath& _paintPath)
{
	seedRegion.clear();

	QRectF pathRect = _paintPath.boundingRect();	
	int _top = (pathRect.top() - widthStroke) >= 0 ? (pathRect.top() - widthStroke) : 0;
	int _bottom = (pathRect.bottom() + widthStroke) < heightImage ?  (pathRect.bottom() + widthStroke) : heightImage - 1;
	int _left = (pathRect.left() - widthStroke) >= 0  ? (pathRect.left() - widthStroke) : 0;
	int _right = (pathRect.right() + widthStroke) < widthImage ? (pathRect.right() + widthStroke) : widthImage - 1;

	//qStrokeMask.copy(_left, _top, _right - _left, _bottom - _top).save(imageDir + "xxx.bmp");

	for (int h = _top; h <= _bottom; ++h)
	{
		QRgb* scanelineStrokeMask = (QRgb*)qStrokeMask.scanLine(h);

		for (int w = _left; w <= _right; ++w)
		{
			int index = h * widthImage + w;

			int grayValue = qGray(scanelineStrokeMask[w]);			

			if (grayValue > 100)
			{
				int patchIndex = pMsSeg->labels[index];

				// when we select the foreground region
				if (isFg)
				{
					if (!regionFg.contains(patchIndex))
					{							
						seedRegion.insert(patchIndex);

						foreach (const int& values, pMsSeg->regNeighborhood[patchIndex])
						{
							if (regionFg.contains(values))
							{
								seedRegion.insert(values);
							}								
						}

					}
				}
				// when we remove the unwanted foreground region
				else
				{
					if (regionFg.contains(patchIndex))
					{
						seedRegion.insert(patchIndex);

						foreach (const int& values, pMsSeg->regNeighborhood[patchIndex])
						{
							if (!regionFg.contains(values))
							{
								seedRegion.insert(values);
							}								
						}

					}

				}

			}

		} // for w

	} // for h

		

}

//////////////////////////////////////////////////////////////////////////
// public slots

void ImageCutOut::SlotGetStrokeWidth(int _width)
{
	widthStroke = _width;
	strokePen.setWidth(widthStroke);
}

void ImageCutOut::SlotGetPaintPathBg(QPainterPath& _paintPath, int _width)
{
	//qDebug() << "SlotGetPaintPathBg: ";
	//qDebug() << _paintPath;

	//qBgStrokeMask;
	QPainter _painter(&qBgStrokeMask);
		
	QPen _pen(Qt::white);
	_pen.setCapStyle(Qt::RoundCap);
	_pen.setWidth(_width);

	_painter.strokePath(_paintPath, _pen);

	//////////////////////////////////////////////////////////////////////////
	seedBg.clear();

	QRectF pathRect = _paintPath.boundingRect();	
	int _top = (pathRect.top() - widthStroke) >= 0 ? (pathRect.top() - widthStroke) : 0;
	int _bottom = (pathRect.bottom() + widthStroke) < heightImage ?  (pathRect.bottom() + widthStroke) : heightImage - 1;
	int _left = (pathRect.left() - widthStroke) >= 0  ? (pathRect.left() - widthStroke) : 0;
	int _right = (pathRect.right() + widthStroke) < widthImage ? (pathRect.right() + widthStroke) : widthImage - 1;

	//qStrokeMask.copy(_left, _top, _right - _left, _bottom - _top).save(imageDir + "xxx.bmp");

	for (int h = _top; h <= _bottom; ++h)
	{
		QRgb* scanelineStrokeMask = (QRgb*)qBgStrokeMask.scanLine(h);

		for (int w = _left; w <= _right; ++w)
		{
			int index = h * widthImage + w;

			int grayValue = qGray(scanelineStrokeMask[w]);	

			if (grayValue > 100)
			{
				int patchIndex = pMsSeg->labels[index];
				seedBg.insert(patchIndex);

			}

		} // for w

	} // for h

}

void ImageCutOut::SetSeedRegion(QSet<int> _seedRegion, bool _isFg) {
    isFg = _isFg;
    seedRegion = _seedRegion;

    GraphCutSegment();
    GetCutOutResult();
    GetContours();
}

void ImageCutOut::SetFgRegion(QSet<int> _regionFg) {
    this->regionFg = _regionFg;
    // when the patch is not in the fg, add it
    qFgMask.fill(0);
    for (auto index : regionFg) {
        for (std::vector<cv::Point>::iterator it = pMsSeg->regCluster[index].begin();
            it != pMsSeg->regCluster[index].end(); it++)
        {
            fgMask.at<uchar>(Point((*it).x, (*it).y)) = 255;
        }
    }
    GetContours();
}

bool ImageCutOut::SlotGetPainterPath(QPainterPath& _paintPath, bool _isFg)
{		
    isFg = _isFg;
				
	qStrokeMask.fill(0);
	strokePainter.strokePath(_paintPath, strokePen);

	//timeCount.start();

	SetSeedRegion(_paintPath);

	if (seedRegion.isEmpty())
	{
        return false;
	}
		
	GraphCutSegment();
	GetCutOutResult();
	GetContours();

    // using for matting
    //blendTrimap = fgMask.clone();
    //alphamatting::GetTriMap(blendTrimap);

	isSelectionOK = true;
	
//	emit SignalSendSelectionResult();

	isEdgeRefined = false;
//	emit SignalSendIsEdgeRefine(isEdgeRefined);
    return true;
}

void ImageCutOut::SlotGetDrawTrimap(QPainterPath& _paintPath)
{	
    throw "not implement";
    /*
	QImage trimap(qSourceImage);
	trimap.fill(0);

	QPainter painter(&trimap);
	painter.strokePath(_paintPath, strokePen);

	alphaMatte = fgMask.clone();
	alphamatting::GetTriMap(alphaMatte);

	QRectF rect = _paintPath.boundingRect();

	int x1 = (rect.x() - widthStroke > 0) ? (rect.x() - widthStroke) : 0;
	int y1 = (rect.y() - widthStroke > 0) ? (rect.y() - widthStroke) : 0;
	int x2 = (rect.x() + rect.width() + widthStroke >= qSourceImage.width()) ?
			 (qSourceImage.width() - 1) : (rect.x() + rect.width() + widthStroke);

	int y2 = (rect.y() + rect.height() + widthStroke >= qSourceImage.height()) ?
			 (qSourceImage.height() - 1) : (rect.x() + rect.height() + widthStroke);

	//for (int y = y1; y <= y2; ++y)
	for (int y = 0; y < heightImage; ++y)
	{
		//for (int x = x1; x <= x2; ++x)
		for (int x = 0; x < widthImage; ++x)
		{
			int val = qGray(trimap.pixel(x, y));
			if (val > 200)
			{
				alphaMatte.at<uchar>(y, x) = 128;
			}
		}
	}

	alphamatting::SolveAlpha_mat2(sourceImage, alphaMatte);
	qAlphaMatte = QImage(alphaMatte.data, widthImage,heightImage, alphaMatte.step, QImage::Format_Indexed8);
	qAlphaMatte.setColorTable(colorTable);
	isEdgeRefined = true;
	emit SignalSendIsEdgeRefine(isEdgeRefined);
    */
}

void ImageCutOut::SlotEdgeRefine()
{	
    throw "not implement";
    /*
	if (isEdgeRefined) return;	
	
	alphaMatte = fgMask.clone();	
	alphamatting::SolveAlpha_mat(sourceImage, alphaMatte);
	
	qAlphaMatte = QImage(alphaMatte.data, widthImage,heightImage, alphaMatte.step, QImage::Format_Indexed8);
	qAlphaMatte.setColorTable(colorTable);
	
	isEdgeRefined = true;

	emit SignalSendIsEdgeRefine(isEdgeRefined);

#if 0
		
	Mat src = imread("source1.png");
	Mat trimap = imread("mask1.bmp", 0);
	Mat alpha = trimap.clone();

	QTime _timeCount;
	_timeCount.start();
	alphamatting::SolveAlpha_mat(src, trimap, alpha);

	qDebug() << "alpha matting costs" << _timeCount.elapsed();

	imshow("alpha", alpha);

#endif
    */
}

