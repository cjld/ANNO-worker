#include "CmCv.h"
#include "CmUFSet.h"
#include <queue>
#include <list>

/************************************************************************/
/* AbsAngle: Calculate magnitude and angle of vectors.					*/
/************************************************************************/
void CmCv::AbsAngle(CMat& cmplx32FC2, Mat& mag32FC1, Mat& ang32FC1)
{
	CV_Assert(cmplx32FC2.type() == CV_32FC2);
	mag32FC1.create(cmplx32FC2.size(), CV_32FC1);
	ang32FC1.create(cmplx32FC2.size(), CV_32FC1);

	for (int y = 0; y < cmplx32FC2.rows; y++)	{
		const float* cmpD = cmplx32FC2.ptr<float>(y);
		float* dataA = ang32FC1.ptr<float>(y);
		float* dataM = mag32FC1.ptr<float>(y);
		for (int x = 0; x < cmplx32FC2.cols; x++, cmpD += 2)	{
			dataA[x] = atan2(cmpD[1], cmpD[0]);
			dataM[x] = sqrt(cmpD[0] * cmpD[0] + cmpD[1] * cmpD[1]);
		}
	}
}

/************************************************************************/
/* GetCmplx: Get a complex value image from it's magnitude and angle    */
/************************************************************************/
void CmCv::GetCmplx(CMat& mag32F, CMat& ang32F, Mat& cmplx32FC2)
{
	CV_Assert(mag32F.type() == CV_32FC1 && ang32F.type() == CV_32FC1 && mag32F.size() == ang32F.size());
	cmplx32FC2.create(mag32F.size(), CV_32FC2);
	for (int y = 0; y < mag32F.rows; y++){
		float* cmpD = cmplx32FC2.ptr<float>(y);
		const float* dataA = ang32F.ptr<float>(y);
		const float* dataM = mag32F.ptr<float>(y);
		for (int x = 0; x < mag32F.cols; x++, cmpD += 2) {
			cmpD[0] = dataM[x] * cos(dataA[x]);
			cmpD[1] = dataM[x] * sin(dataA[x]);
		}
	}
}

// Mat2GrayLog: Convert and arbitrary mat to [0, 1] for display.
// The result image is in 32FCn format and range [0, 1.0].
// Mat2GrayLinear(log(img+1), newImg). In place operation is supported.
void CmCv::Mat2GrayLog(CMat& img, Mat& newImg)
{
	img.convertTo(newImg, CV_32F);
	newImg += 1;
	cv::log(newImg, newImg);
	cv::normalize(newImg, newImg, 0, 1, NORM_MINMAX);
}

// Low frequency part is always been move to the central part:
//				 -------                          -------	
//				| 1 | 2 |                        | 3 | 4 |	
//				 -------            -->           -------	
//				| 4 | 3 |                        | 2 | 1 |	
//				 -------                          -------	
void CmCv::FFTShift(Mat& img)
{
	int w = img.cols / 2, h = img.rows / 2;
    int cx2 = img.cols - w, cy2 = img.rows - h;
	Swap(img(Rect(0, 0, w, h)), img(Rect(cx2, cy2, w, h)));  // swap 1, 3
	Swap(img(Rect(cx2, 0, w, h)), img(Rect(0, cy2, w, h)));  // swap 2, 4
}

/************************************************************************/
/* Swap the content of two Mat with same type and size                  */
/************************************************************************/
void CmCv::Swap(Mat a, Mat b)
{
	CV_Assert(a.type() == b.type() && a.size() == b.size());
	Mat t;
	a.copyTo(t);
	b.copyTo(a);
	t.copyTo(b);
}

Rect CmCv::GetMaskRange(CMat &mask1u, int ext)
{
	int maxX = INT_MIN, maxY = INT_MIN, minX = INT_MAX, minY = INT_MAX, rows = mask1u.rows, cols = mask1u.cols;
	for (int r = 0; r < rows; r++)	{
		const byte* data = mask1u.ptr<byte>(r);
		for (int c = 0; c < cols; c++)
			if (data[c] > 10) {
				maxX = max(maxX, c);
				minX = min(minX, c);
				maxY = max(maxY, r);
				minY = min(minY, r);
			}
	}

	maxX = maxX + ext + 1 < cols ? maxX + ext + 1 : cols;
	maxY = maxY + ext + 1 < rows ? maxY + ext + 1 : rows;
	minX = minX - ext > 0 ? minX - ext : 0;
	minY = minY - ext > 0 ? minY - ext : 0;

	return Rect(minX, minY, maxX - minX, maxY - minY);
}

// Get continuous components for same label regions. Return region index mat,
// index-counter pair (Number of pixels for each index), and label of each idx
int CmCv::GetRegions(const Mat_<byte> &label1u, Mat_<int> &regIdx1i, vecI &idxCount, vecB idxLabel, bool noZero)
{
	vector<pair<int, int>> counterIdx;
	int _w = label1u.cols, _h = label1u.rows, maxIdx = -1;
	regIdx1i.create(label1u.size());
	regIdx1i = -1;
	vecB labels;

	for (int y = 0; y < _h; y++){		
		int *regIdx = regIdx1i.ptr<int>(y);
		for (int x = 0; x < _w; x++) {
			if (regIdx[x] != -1) // If already assigned to a region
				continue;
			if (noZero && label1u(y, x) == 0)
				continue;
			
			byte crntVal = label1u(y, x);
			pair<int, int> counterReg(0, ++maxIdx); // Number of pixels in region with index maxIdx
			Point pt(x, y);
			queue<Point, list<Point>> neighbs;
			regIdx[x] = maxIdx;
			neighbs.push(pt);

			// Repeatably add pixels to the queue to construct neighbor regions
			while(neighbs.size()){
				// Mark current pixel
				pt = neighbs.front();
				neighbs.pop();
				counterReg.first++;

				for (int i = 0; i < 8; i++)	{
					Point nPt = pt + DIRECTION8[i];
					if (CHK_IND(nPt) && regIdx1i(nPt) == -1 && label1u(nPt) == crntVal)
						regIdx1i(nPt) = maxIdx, neighbs.push(nPt);  
				}		
			}

			// Add current region to regions
			counterIdx.push_back(counterReg);
			labels.push_back(crntVal);
		}
	}
	sort(counterIdx.begin(), counterIdx.end(), greater<pair<int, int>>());
	int idxNum = (int)counterIdx.size();
	vector<int> newIdx(idxNum);
	idxCount.resize(idxNum);
	idxLabel.resize(idxNum);
	for (int i = 0; i < idxNum; i++){
		idxCount[i] = counterIdx[i].first;
		newIdx[counterIdx[i].second] = i;
		idxLabel[i] = labels[counterIdx[i].second];
	}
	
	for (int y = 0; y < _h; y++){
		int *regIdx = regIdx1i.ptr<int>(y);
		for (int x = 0; x < _w; x++)
			if (!noZero || label1u(y, x) != 0)
				regIdx[x] = newIdx[regIdx[x]];
	}

	/* Test GetRegions
	{
		Mat showImg = Mat::zeros(_h, _w * 2, CV_8UC3);
		Mat showReg = showImg(Rect(_w, 0, _w, _h));
		Mat showLabel = showImg(Rect(0, 0, _w, _h));
		cvtColor(label1u, showLabel, CV_GRAY2BGR);
		Mat mask1u;
		for (size_t i = 0; i < idxCount.size(); i++)
		{
			compare(regIdx1i, i, mask1u, CMP_EQ);
			showReg.setTo(Scalar(rand() % 128 + 128, rand()%128 + 128, i%255), mask1u);
			imshow("Regions", showImg);
			CmLog::LogLine("%d pixels in region %d, label = %d\n", idxCount[i], i, (int)idxLabel[i]);
			waitKey(1);
		}
		waitKey(0);
	}
	//*/
	return idxNum;
}

// Get continuous components for non-zero labels. Return region index mat (region index 
// of each mat position) and sum of label values in each region
int CmCv::GetNZRegions(const Mat_<byte> &label1u, Mat_<int> &regIdx1i, vecI &idxSum)
{
	vector<pair<int, int>> counterIdx;
	int _w = label1u.cols, _h = label1u.rows, maxIdx = -1;
	regIdx1i.create(label1u.size());
	regIdx1i = -1;

	for (int y = 0; y < _h; y++){		
		int *regIdx = regIdx1i.ptr<int>(y);
		const byte *label = label1u.ptr<byte>(y);
		for (int x = 0; x < _w; x++) {
			if (regIdx[x] != -1 || label[x] == 0)
				continue;
			
			pair<int, int> counterReg(0, ++maxIdx); // Number of pixels in region with index maxIdx
			Point pt(x, y);
			queue<Point, list<Point>> neighbs;
			regIdx[x] = maxIdx;
			neighbs.push(pt);

			// Repeatably add pixels to the queue to construct neighbor regions
			while(neighbs.size()){
				// Mark current pixel
				pt = neighbs.front();
				neighbs.pop();
				counterReg.first += label1u(pt);

				// Mark its unmarked neighbor pixels if similar
				Point nPt(pt.x, pt.y - 1); //Upper 
				if (nPt.y >= 0 && regIdx1i(nPt) == -1 && label1u(nPt) > 0){
					regIdx1i(nPt) = maxIdx;
					neighbs.push(nPt);  
				}

				nPt.y = pt.y + 1; // lower
				if (nPt.y < _h && regIdx1i(nPt) == -1 && label1u(nPt) > 0){
					regIdx1i(nPt) = maxIdx;
					neighbs.push(nPt);  
				}

				nPt.y = pt.y, nPt.x = pt.x - 1; // Left
				if (nPt.x >= 0 && regIdx1i(nPt) == -1 && label1u(nPt) > 0){
					regIdx1i(nPt) = maxIdx;
					neighbs.push(nPt);  
				}

				nPt.x = pt.x + 1;  // Right
				if (nPt.x < _w && regIdx1i(nPt) == -1 && label1u(nPt) > 0)	{
					regIdx1i(nPt) = maxIdx;
					neighbs.push(nPt);  
				}				
			}

			// Add current region to regions
			counterIdx.push_back(counterReg);
		}
	}
	sort(counterIdx.begin(), counterIdx.end(), greater<pair<int, int>>());
	int idxNum = (int)counterIdx.size();
	vector<int> newIdx(idxNum);
	idxSum.resize(idxNum);
	for (int i = 0; i < idxNum; i++){
		idxSum[i] = counterIdx[i].first;
		newIdx[counterIdx[i].second] = i;
	}
	
	for (int y = 0; y < _h; y++){
		int *regIdx = regIdx1i.ptr<int>(y);
		for (int x = 0; x < _w; x++)
			if (regIdx[x] >= 0)
				regIdx[x] = newIdx[regIdx[x]];
	}
	return idxNum;
}

Mat CmCv::GetNZRegionsLS(CMat &mask1u, double ignoreRatio)
{
	CV_Assert(mask1u.type() == CV_8UC1 && mask1u.data != NULL);
	ignoreRatio *= mask1u.rows * mask1u.cols * 255;
	Mat_<int> regIdx1i;
	vecI idxSum;
	Mat resMask;
	CmCv::GetNZRegions(mask1u, regIdx1i, idxSum);
	if (idxSum.size() >= 1 && idxSum[0] > ignoreRatio)
		compare(regIdx1i, 0, resMask, CMP_EQ);
	return resMask;
}

int CmCv::GetBorderPnts(Size sz, double ratio, vector<Point> &bdPnts)
{
	int w = sz.width, h = sz.height;
	int wGap = cvRound(w * ratio), hGap = cvRound(h * ratio);
	int idx = 0, bdCount = 2 * (hGap * w + wGap * h - 2 * hGap * wGap);
	bdPnts.resize(bdCount);

	ForPoints2(pnt, 0, 0, w, hGap) // Top region
		bdPnts[idx++] = pnt;
	ForPoints2(pnt, 0, h - hGap, w, h) // Bottom region
		bdPnts[idx++] = pnt;
	ForPoints2(pnt, 0, hGap, wGap, h - hGap) // Left center region
		bdPnts[idx++] = pnt;
	ForPoints2(pnt, w - wGap, hGap, w, h-hGap)
		bdPnts[idx++] = pnt;
	return bdCount;
}

Mat CmCv::GetBorderRegC(CMat &img3u, Mat &idx1i, vecI &idxCount)
{
	Mat img, edgC;
	GaussianBlur(img3u, img, Size(3, 3), 0);
	Mat edg = Mat::zeros(img.size(), CV_8U);
	vecM imgs;
	split(img, imgs);
	for (int c = 0; c < 3; c++)	{
		Canny(imgs[c], edgC, 100, 400, 5, true);
		edg += edgC;
	}
	compare(edg, 150, edg, CMP_LE);

	int _h = img.rows, _w = img.cols;
	vecI xCount(_w, 0), yCount(_h, 0);
	for (int r = 0; r < _h; r++)	{
		const byte* edgP = edg.ptr<byte>(r);
		for (int c = 0; c < _w; c++)
			if (edgP[c] == 0)
				xCount[c]++, yCount[r]++;
	}
	bool b1 = true, b2 = true, b3 = true, b4 = true;
	for (int t = 0; t < 8; t++){  // Remove frame
		if (t >= 3)
			b1 = b2 = b3 = b4 = false;
		if (b1 && xCount[t] > 0.6 * _h || xCount[t] > 0.8 * _h)
			edg.col(t) = 255, b1 = false;
		if (b2 && xCount[_w - 1 - t] > 0.6 * _h || xCount[_w - 1 - t] > 0.8 * _h)
			edg.col(_w - 1 - t) = 255, b2 = false;
		if (b3 && yCount[t] > 0.6 * _w || yCount[t] > 0.8 * _w)
			edg.row(t) = 255, b3 = false;
		if (b4 && yCount[_h - 1 - t] > 0.6 * _w || yCount[_h - 1 - t] > 0.8 * _w)
			edg.row(_h - 1 - t) = 255, b4 = false;
	}
	for (int r = 0; r < _h; r++)	{ // Remove single isolated points
		byte* edgP = edg.ptr<byte>(r);
		for (int c = 0; c < _w; c++)
			if (edgP[c] == 0){
				int count = 0;
				for (int t = 0; t < 8; t++){
					Point p = Point(c, r) + DIRECTION8[t];
					if (CHK_IND(p) && edg.at<byte>(p) == 0)
						count ++; 
				}
				if (count == 0)
					edgP[c] = 255;
			}
	}

	erode(edg, edg, Mat(), Point(-1, -1), 2);
	Mat_<int> idx1iT;
	int regNum = CmCv::GetRegions(edg, idx1iT, idxCount, vecB(), true);
	while(regNum > 1 && idxCount[regNum - 1] < 300)
		regNum--;
	Mat bdCMask = CmCv::GetBorderReg(idx1iT, regNum), ignoreMask;
	dilate(bdCMask, bdCMask, Mat(), Point(-1, -1), 3);
	erode(bdCMask, bdCMask, Mat(), Point(-1, -1), 2);
	compare(idx1iT, regNum, ignoreMask, CMP_GE);
	idx1iT.setTo(-1, ignoreMask);
	idx1i = idx1iT;
	idxCount.resize(regNum);
	return bdCMask;
}

Mat CmCv::GetBorderReg(CMat &idx1i, int regNum, double ratio, double thr)
{
	// Variance of x and y
	vecD vX(regNum), vY(regNum);
	int w = idx1i.cols, h = idx1i.rows;{
		vecD mX(regNum), mY(regNum), n(regNum); // Mean value of x and y, pixel number of region
		for (int y = 0; y < idx1i.rows; y++){
			const int *idx = idx1i.ptr<int>(y);
			for (int x = 0; x < idx1i.cols; x++, idx++)
				if (*idx >= 0 && *idx < regNum)
					mX[*idx] += x, mY[*idx] += y, n[*idx]++;
		}
		for (int i = 0; i < regNum; i++)
			mX[i] /= n[i], mY[i] /= n[i];
		for (int y = 0; y < idx1i.rows; y++){
			const int *idx = idx1i.ptr<int>(y);
			for (int x = 0; x < idx1i.cols; x++, idx++)
				if (*idx >= 0 && *idx < regNum)
					vX[*idx] += abs(x - mX[*idx]), vY[*idx] += abs(y - mY[*idx]);
		}
		for (int i = 0; i < regNum; i++)
			vX[i] = vX[i]/n[i] + EPS, vY[i] = vY[i]/n[i] + EPS;
	}

	// Number of border pixels in x and y border region
	vecI xbNum(regNum, 0), ybNum(regNum, 0); 
	int wGap = cvRound(w * ratio), hGap = cvRound(h * ratio);
	vector<Point> bPnts; { 
		ForPoints2(pnt, 0, 0, w, hGap) {// Top region
			int idx = idx1i.at<int>(pnt);
			if (idx >= 0 && idx < regNum)
				ybNum[idx]++, bPnts.push_back(pnt);
		}
		ForPoints2(pnt, 0, h - hGap, w, h){ // Bottom region
			int idx = idx1i.at<int>(pnt);
			if (idx >= 0 && idx < regNum)
			ybNum[idx]++, bPnts.push_back(pnt);
		}
		ForPoints2(pnt, 0, 0, wGap, h) {// Left region
			int idx = idx1i.at<int>(pnt);
			if (idx >= 0 && idx < regNum)
				xbNum[idx]++, bPnts.push_back(pnt);
		}
		ForPoints2(pnt, w - wGap, 0, w, h){
			int idx = idx1i.at<int>(pnt);
			if (idx >= 0 && idx < regNum)
				xbNum[idx]++, bPnts.push_back(pnt);
		}
	}

	Mat bReg1u = Mat::zeros(idx1i.size(), CV_8U);{  // likelihood map of border region
		double xR = 1.0/(4*wGap), yR = 1.0/(4*hGap);
		vector<byte> regL(regNum); // likelihood of each region belongs to border background
		for (int i = 0; i < regNum; i++) {
			double lk = xbNum[i] * xR / vY[i] + ybNum[i] * yR / vX[i];
			if (xbNum[i] < 40 * wGap && ybNum[i] < 40 * hGap)
				lk /= 2;
			regL[i] = lk/thr > 1 ? 255 : 0; //saturate_cast<byte>(255 * lk / thr);
		}

		for (int r = 0; r < h; r++)	{
			const int *idx = idx1i.ptr<int>(r);
			byte* maskData = bReg1u.ptr<byte>(r);
			for (int c = 0; c < w; c++, idx++)
				if (*idx >= 0 && *idx < regNum)
					maskData[c] = regL[*idx];
		}
	}
	return bReg1u;
}

//int CmCv::MergeSimiReg(CMat &img3f, Mat idx1i, int regNum, double thr)
//{
//	Mat colorIdx1i, tmp, color3fv;
//	//int clrNums1[3] = {9, 9, 9}, clrNums2[3] = {11, 11, 11};
//	int colorNum = CmColorQua::D_Quantize(img3f, colorIdx1i, color3fv, tmp); //, 0.95, clrNums1);
//	//regNum = MergeSimiReg(colorIdx1i, colorNum, idx1i, regNum, thr);
//	//colorNum = CmColorQua::D_Quantize(img3f, colorIdx1i, color3fv, tmp, 0.95, clrNums2);
//	return MergeSimiReg(colorIdx1i, colorNum, idx1i, regNum, thr);
//}
//
//// Merge similar regions
//int CmCv::MergeSimiReg(CMat &colorIdx1i, int colorNum, Mat idx1i, int regNum, double thr)
//{
//	CV_Assert(colorIdx1i.type() == CV_32SC1 && idx1i.type() == CV_32SC1 && colorIdx1i.size == idx1i.size);
//	int _w = idx1i.cols, _h = idx1i.rows;
//	vecI count(regNum);
//
//	map<pair<int, int>, int> adjacent; // region i, j have adjacent[(i,j)] connection points and i < j
//	Mat_<int> regColorFre1i = Mat_<int>::zeros(regNum, colorNum); {// region color frequency 
//		for (int r = 0; r < _h; r++){
//			const int *idx = idx1i.ptr<int>(r);
//			const int *colorIdx = colorIdx1i.ptr<int>(r);
//			for (int c = 0; c < _w; c++){
//				count[idx[c]] ++;
//				regColorFre1i(idx[c], colorIdx[c])++;
//				int a = idx[c];
//				if (c+1 < _w){
//					int b = idx[c+1];
//					if (a != b)
//						adjacent[make_pair(min(a,b), max(a,b))]++;
//				}
//				if (r+1 < _h){
//					int b = idx1i.at<int>(r+1, c);
//					if (a != b)
//						adjacent[make_pair(min(a,b), max(a,b))]++;
//				}
//			}
//		}
//	}
//
//	// Normalized region histogram
//	vector<SparseMat> hists(regNum); { 
//		Mat regColorFre1f;
//		regColorFre1i.convertTo(regColorFre1f, CV_32F);
//		for (int i = 0; i < regNum; i++){
//			hists[i] = SparseMat(regColorFre1f.row(i));
//			normalize(hists[i], hists[i], 1, NOR_L1);
//		}
//	}
//
//	// Merge similar adjacent regions
//	CmUFSet ufset(regNum);
//	for (auto it = adjacent.begin(); it != adjacent.end(); it++){
//		int i = it->first.first, j = it->first.second;
//		double dist = compareHist(hists[i], hists[j], CV_COMP_CHISQR);
//		//double dist = HistDist(regCFre[i], regCFre[j], color3fv);
//		//CmLog::LogLine("Region(%d, %d): %g\n", i, j, dist);
//		if (dist < thr)
//			ufset.Union(i, j);
//	}
//
//	// Rename region index
//	map<int, int> rootCount;{
//		vecI idxOld2Root(regNum, -1), idxRoot2New(regNum, -1), idxOld2New(regNum, -1);
//		for (int i = 0; i < regNum; i++)
//			rootCount[ufset.Find(i)] += count[i];
//		vector<CostiIdx> cIdx;
//		for (auto it = rootCount.begin(); it != rootCount.end(); it++)
//			cIdx.push_back(make_pair(it->second, it->first));
//		sort(cIdx.begin(), cIdx.end(), greater<CostiIdx>());
//		for (size_t i = 0; i < cIdx.size(); i++)
//			idxRoot2New[cIdx[i].second] = i;
//		for (size_t i = 0; i < idxOld2New.size(); i++)
//			idxOld2New[i] = idxRoot2New[ufset.Find(i)];
//		for (int r = 0; r < _h; r++){
//			int *idx = idx1i.ptr<int>(r);
//			for (int c = 0; c < _w; c++)
//				idx[c] = idxOld2New[idx[c]];
//		}
//	}
//
//	return rootCount.size();
//}

// Merge similar regions
int CmCv::MergeSimiReg(CMat img3f, Mat idx1i, int regNum, double minColor, double minVar)
{
	// Find mean color and variance of each region
	vector<Vec3f> mCl(regNum); // mean color
	vecD var(regNum); //variance
	vecI count(regNum);
	map<pair<int, int>, int> adjacent; // region i, j have adjacent[(i,j)] connection points and i < j
	int _w = img3f.cols, _h = img3f.rows;
	for (int r = 0; r < _h; r++){
		const Vec3f *img = img3f.ptr<Vec3f>(r);
		int *idx = idx1i.ptr<int>(r);
		for (int c = 0; c < _w; c++){
			mCl[idx[c]] += img[c];
			count[idx[c]] ++;
			int a = idx[c];
			if (c+1 < _w){
				int b = idx[c+1];
				if (a != b)
					adjacent[make_pair(min(a,b), max(a,b))]++;
			}
			if (r+1 < _h){
				int b = idx1i.at<int>(r+1, c);
				if (a != b)
					adjacent[make_pair(min(a,b), max(a,b))]++;
			}
		}
	}
	for (int i = 0; i < regNum; i++)
	{
		for (int n = 0; n < 3; ++n)
		{
			mCl[i][n] /= (count[i]+EPS);
		}		
	}

	for (int r = 0; r < _h; r++){
		const Vec3f *img = img3f.ptr<Vec3f>(r);
		int *idx = idx1i.ptr<int>(r);
		for (int c = 0; c < _w; c++)
			var[idx[c]] += vecSqrDist3(mCl[idx[c]], img[c]);
	}
	for (int i = 0; i < regNum; i++)
		var[i] = sqrt(var[i]/(count[i]+EPS));

	// Merge similar adjacent regions
	CmUFSet ufset(regNum);
	for (auto it = adjacent.begin(); it != adjacent.end(); it++){
		int i = it->first.first, j = it->first.second;
		double difC = abs(vecDist3(mCl[i], mCl[j])), difVar = abs(var[i] - var[j]) / (var[i] + var[j] + 0.1);
		if (difC < minColor && difVar < minVar) // Merge i,j
			ufset.Union(i, j);
		//if (difC < minColor && difVar < minVar) CmLog::LogLine("Merge: %d, %d, cD = %g, vD = %g\n", i, j, difC, difVar);
	}
	vecI idxOld2Root(regNum, -1), idxRoot2New(regNum, -1), idxOld2New(regNum, -1);
	map<int, int> rootCount;
	for (int i = 0; i < regNum; i++)
		rootCount[ufset.Find(i)] += count[i];
	vector<CostiIdx> cIdx;
	for (auto it = rootCount.begin(); it != rootCount.end(); it++)
		cIdx.push_back(make_pair(it->second, it->first));
	sort(cIdx.begin(), cIdx.end(), greater<CostiIdx>());
	for (size_t i = 0; i < cIdx.size(); i++)
		idxRoot2New[cIdx[i].second] = i;
	for (size_t i = 0; i < idxOld2New.size(); i++)
		idxOld2New[i] = idxRoot2New[ufset.Find(i)];
	for (int r = 0; r < _h; r++){
		int *idx = idx1i.ptr<int>(r);
		for (int c = 0; c < _w; c++)
			idx[c] = idxOld2New[idx[c]];
	}
	return rootCount.size();
}

//double CmCv::HistDist(const CostfIdxV &c1, const CostfIdxV &c2, CMat &color3fv)
//{
//vector<CostfIdxV> regCFre(regNum);
//{
//	regColorFre1f.row(i) /= count[i];
//	hists[i] = SparseMat(regColorFre1f.row(i));
//	float* fre1f = regColorFre1f.ptr<float>(i);
//	for (int j = 0; j < colorNum; j++)	{
//		if (fre1f[j] > EPS)
//			regCFre[i].push_back(make_pair(fre1f[j], j));
//	}
//}
//	Vec3f* pColor = (Vec3f*)color3fv.data;
//	double d = 0;
//	for (size_t m = 0; m < c1.size(); m++)
//		for (size_t n = 0; n < c2.size(); n++)
//			d += vecDist3(pColor[c1[m].second], pColor[c2[n].second]) * c1[m].first * c2[n].first;
//	return d * 0.5;
//}


//The histogram is normalized
//double CmCv::HistDist(CSMat &_h1, CSMat &_h2, CMat &color3fv)
//{
//	return compareHist(_h1, _h2, CV_COMP_CHISQR);
//}

void CmCv::fillPoly(Mat& img, const vector<PointSeti> _pnts, const Scalar& color, int lineType, int shift, Point offset)
{
	const int NUM(_pnts.size());
	const Point **pnts = new const Point *[NUM];
	int *num = new int[NUM];
	for (int i = 0; i < NUM; i++){
		pnts[i] = &_pnts[i][0];
		num[i] = (int)_pnts[i].size();
	}
	cv::fillPoly(img, pnts, num, NUM, color, lineType, shift, offset);

	delete []num;
	delete []pnts;
}

void CmCv::Demo(const char* fileName/* = "H:\\Resize\\cd3.avi"*/)
{

}


//
//void CmCv::AddAlpha(CvMat *img, CvMat *alpha)
//{
//#pragma omp parallel for
//	for (int r = 0; r < img->height; r++)
//	{
//		byte* imgD = img->data.ptr + img->step * r;
//		byte* alpD = alpha->data.ptr + alpha->step * r;
//		for (int c = 0; c < img->width; c++, imgD += 3)
//		{
//			double a = alpD[c] / 255.0;
//			imgD[0] = (byte)(imgD[0] * a);
//			imgD[1] = (byte)(imgD[1] * a);
//			imgD[2] = (byte)(imgD[2] * a);
//		}
//	}
//}
//
//void CmCv::AddAlpha(CvMat *img, CvMat *alpha, CvScalar bgColor)
//{
//#pragma omp parallel for
//	for (int r = 0; r < img->height; r++)
//	{
//		byte* imgD = img->data.ptr + img->step * r;
//		byte* alpD = alpha->data.ptr + alpha->step * r;
//		for (int c = 0; c < img->width; c++, imgD += 3)
//		{
//			double a = alpD[c] / 255.0;
//			imgD[0] = (byte)(imgD[0] * a + (1-a) * bgColor.val[0]);
//			imgD[1] = (byte)(imgD[1] * a + (1-a) * bgColor.val[1]);
//			imgD[2] = (byte)(imgD[2] * a + (1-a) * bgColor.val[2]);
//		}
//	}
//}
//
//CvMat* CmCv::RotateScale(IN CvMat *src, IN double angle, double factor )
//{
//	float m[6], mm[4];
//	// Matrix m looks like:
//	//
//	// [ m0  m1  m2 ] ===>  [ A11  A12   b1 ]
//	// [ m3  m4  m5 ]       [ A21  A22   b2 ]
//	//
//	CvMat M = cvMat (2, 3, CV_32F, m);
//	int w = src->width;
//	int h = src->height;
//	m[0] = (float) (1/factor * cos (angle * CV_PI / 180.));
//	m[1] = (float) (1/factor * sin (angle * CV_PI / 180.));
//	m[3] = -m[1];
//	m[4] = m[0];
//
//	mm[0] = (float)(m[0] * factor * factor);
//	mm[1] = (float)(m[1] * factor * factor);
//	mm[2] = -mm[1];
//	mm[3] = mm[0];
//	// 将旋转中心移至图像中间
//	m[2] = (float)(w*0.5);
//	m[5] = (float)(h*0.5);
//	float x[10];
//	float y[10];
//	x[0] = - m[2];
//	y[0] = - m[5];
//	x[1] = w-1-m[2];
//	y[1] = - m[5];
//	x[2] = w-1-m[2];
//	y[2] = h-1-m[5];
//	x[3] = - m[2];
//	y[3] = h-1-m[5];
//	x[4] = 100-m[2];
//	y[4] = 100-m[5];
//	for (int i=0;i<5;i++)
//	{
//		x[i+5] = mm[0]*x[i]-mm[1]*y[i];
//		y[i+5] = -mm[2]*x[i]+mm[3]*y[i];
//	}
//	float minx =min(min(x[8],x[5]),min(x[6],x[7]));
//	float miny =min(min(y[8],y[5]),min(y[6],y[7]));
//	float maxx =max(max(x[8],x[5]),max(x[6],x[7]));
//	float maxy =max(max(y[8],y[5]),max(y[6],y[7]));
//	int neww = round(maxx-minx+1);
//	int newh = round(maxy-miny+1);
//
//	CvMat *dst = cvCreateMat(newh, neww, src->type);
//	//  dst(x,y) = A * src(x,y) + b
//	cvZero (dst);
//	cvGetQuadrangleSubPix (src, dst, &M);
//	return dst;
//}
//
//void CmCv::RotateScale(IN CvMat *src, OUT CvMat *dst, double angle, double factor)
//{
//	// Matrix m looks like:
//	//
//	// [ m0  m1  m2 ] ===>  [ A11  A12   b1 ]
//	// [ m3  m4  m5 ]       [ A21  A22   b2 ]
//	//
//	float m[6];
//	CvMat M = cvMat (2, 3, CV_32F, m);
//	int w = src->width;
//	int h = src->height;
//	m[0] = (float) (1/factor * cos (angle * CV_PI / 180.));
//	m[1] = (float) (1/factor * sin (angle * CV_PI / 180.));
//	m[3] = -m[1];
//	m[4] = m[0];
//
//	// 将旋转中心移至图像中间
//	m[2] = (float)(w*0.5);
//	m[5] = (float)(h*0.5);
//
//	//  dst(x,y) = A * src(x,y) + b
//	cvZero (dst);
//	cvGetQuadrangleSubPix (src, dst, &M);
//}
//
