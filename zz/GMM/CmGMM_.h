#pragma once
#include "CmGaussian.h"
#include "CmDefinition.h"
#include "CmLog.h"

template <int D> class CmGMM_
{
public:
	typedef const Vec<double, D> CVecT;
	CmGMM_(int K, double thrV = 0.02, double thrC = 0.1);
	inline int K() const {return _K;}
	inline const vector<CmGaussian_<D>>& GetGaussians() const {return _Guassians;}

	double P(CVecT &s) const; // Probability density of sample s in this GMM
	double P(int i, CVecT &s) const; // Probability density of sample s in just Gaussian i
	inline CVecT getMean(int k) const {return _Guassians[k].mean;}
	inline double getWeight(int k) const {return _Guassians[k].w; }
	inline double getSumWeights() const {return _sumW;}

	void GetProbs(CMat& sDd, vector<Mat> &ps1d); // K probability maps 
    void Reset() {for (int i=0; i<_K; i++) _Guassians[i].Reset();}

	// Build the initial GMMs using the Orchard and Bouman clustering algorithm
	// sDd contains D dimensional double type samples, idx1i are resulted component index of these samples
	// w: CV_8UC1 to indicate mask or CV64FC1 to indicate sample weights
	void BuildGMMs(CMat& sDd, Mat& idx1i, CMat& w1d = Mat()); 
	void RefineGMMs(CMat& sDd, Mat& idx1i, CMat& w1d = Mat(), bool reAssign = true); // Iteratively refine GMM

	void ReadFromMat(const Mat_<double> &m1d); // read from a _K x N Mat with N = 2 + D + D*D
	Mat_<double> SaveToMat(); // Save to a _K x N Mat with N = 2 + D + D*D


private:
	int _K; // Number of Guassians
	double _sumW; // Sum of sample weight. For typical weights it's the number of pixels
	double _ThrV; // The lowest variations of Guassians
	double _ThrC; // Threshold for similar color
	vector<CmGaussian_<D>> _Guassians; // An array of K Guassians

	void AssignEachSample(CMat& sDd, Mat &idx1i, CMat &w1d = Mat());
};

struct CmGMM3D : public CmGMM_<3> {
	CmGMM3D(int K, double thrV = 0.02, double thrC = 0.1):CmGMM_<3>(K, thrV, thrC) { }

	static void View(const CmGMM3D &gmm, CStr &title, bool decreaseShow = true);
	static double ViewFrgBkgProb(const CmGMM3D &fGMM, const CmGMM3D &bGMM, CStr &title); // Show foreground probabilities represented by the GMMs
	static void GetGMMs(CStr &smplW, CStr &annoExt, CmGMM3D &fGMM, CmGMM3D &bGMM);

	static void Demo(CStr &wkDir); // 
	static void DemoGroup(CStr &wkDir); // "C:/WkDir/ObjectRetrieval/DogJump/"
};


template<int D> CmGMM_<D>::CmGMM_(int K, double thrV, double thrC)
	: _K(K), _ThrV(thrV), _ThrC(thrC)
{
	_Guassians.resize(K);
}

template<int D> double CmGMM_<D>::P(CVecT &s) const 
{
	double r = 0;
	if (_Guassians.size() == _K)
		for (int i = 0; i < _K; i++)
			r += _Guassians[i].w * P(i, s);
	return r;
}

template<int D> double CmGMM_<D>::P(int i, CVecT &s) const 
{
	double r = 0;
	const CmGaussian_<D> &guassian = _Guassians[i];
	if (guassian.w < EPS)
		return r;
	CV_Assert_(guassian.det > EPS, ("zero determinate in %dth Gaussian at line %d: %s", i, __LINE__, __FILE__));

	CVecT ns = s - guassian.mean; // Normalized sample
	const Matx<double, D, D> &inv = guassian.inv;
	double d = (ns.t() * inv * ns)(0);
	return 0.0635 / sqrt(guassian.det) * exp(-0.5 * d);   // 1/(2*pi)^1.5 = 0.0635;
}

template<int D> void CmGMM_<D>::BuildGMMs(CMat& sDd, Mat& idx1i, CMat& w1d)
{
	// Preparing common data
	bool weighted = w1d.data != NULL; 
	int rows = sDd.rows, cols = sDd.cols; { 
		CV_Assert(sDd.data != NULL && sDd.type() == CV_MAKETYPE(CV_64F,D));
		idx1i = Mat::zeros(sDd.size(), CV_32S);
		CV_Assert(!weighted || w1d.type() == CV_64FC1 && w1d.size == sDd.size);
		if (sDd.isContinuous() && idx1i.isContinuous() && (!weighted || w1d.isContinuous()))
			cols *= sDd.rows, rows = 1;
		_sumW = weighted ? sum(w1d).val[0] : rows * cols; // Finding sum weight
		for (int i = 0; i < _K; i++)
			_Guassians[i].Reset();
	}

	// Initial first clusters
	for (int y = 0; y < rows; y++){
		CVecT *s = sDd.ptr<CVecT>(y);
		const double *w = weighted ? w1d.ptr<double>(y) : NULL;
		if (weighted){
			for (int x = 0; x < cols; x++)
				if (w[x] > EPS )
					_Guassians[0].Add(s[x], w[x]);
		}else
			for (int x = 0; x < cols; x++)
				_Guassians[0].Add(s[x]);
	}
	_Guassians[0].BuildGuassian(_sumW, true);

	// Compute Clusters
	int nSplit = 0; // Which cluster will be split
	for (int i = 1; i < _K; i++){
		CmGaussian_<D>& sG = _Guassians[nSplit]; // Get the reference for the splitting Gaussian
		sG.ResetDataForGuassian();
		Matx<double, D, 1> crntEVale = sG.eVecs.col(0);
		double split = crntEVale.dot(sG.mean); // Compute splitting point

		// Split clusters nSplit, place split portion into cluster i
		for (int y = 0; y < rows; y++) {
			int *idx = idx1i.ptr<int>(y);
			CVecT *s = sDd.ptr<CVecT>(y);
			const double *w = weighted ? w1d.ptr<double>(y) : NULL;
			if (weighted){
				for (int x = 0; x < cols; x++){
					if (w[x] <= EPS || idx[x] != nSplit)
						continue;
					if (crntEVale.dot(s[x]) > split)
						_Guassians[i].Add(s[x], w[x]), idx[x] = i;
					else
						sG.Add(s[x], w[x]);
				}	
			}else{
				for (int x = 0; x < cols; x++){
					if (idx[x] != nSplit)
						continue;
					if (crntEVale.dot(s[x]) > split)
						_Guassians[i].Add(s[x]), idx[x] = i;
					else
						sG.Add(s[x]);
				}	
			}
		}

		// Compute new split Gaussian by finding clusters with highest eigenvalue
		sG.BuildGuassian(_sumW, true);
		_Guassians[i].BuildGuassian(_sumW, true);
		nSplit = 0;
		for (int j = 1; j <= i; j++)
			if (_Guassians[j].eVals[0] > _Guassians[nSplit].eVals[0])
				nSplit = j;
	}
}

template<int D> void CmGMM_<D>::RefineGMMs(CMat& sDd, Mat& idx1i, CMat& w1d, bool reAssign)
{
	// Preparing common data
	bool weighted = w1d.data != NULL; 
	int rows = sDd.rows, cols = sDd.cols; { 
		CV_Assert(sDd.type() == CV_MAKETYPE(CV_64F, D) && idx1i.type() == CV_32S && sDd.size == idx1i.size);
		CV_Assert(!weighted || w1d.type() == CV_64FC1 && w1d.size == sDd.size);
		if (sDd.isContinuous() && idx1i.isContinuous() && (!weighted || w1d.isContinuous()))
			cols *= sDd.rows, rows = 1;
		CV_Assert(weighted && abs(_sumW - sum(w1d).val[0]) < EPS || !weighted && abs(_sumW - rows * cols) < EPS);
	}
	if (reAssign)
		AssignEachSample(sDd, idx1i, w1d);

	//  Relearn GMM from new component assignments
	for (int i = 0; i < _K; i++)
		_Guassians[i].ResetDataForGuassian();
	for (int y = 0; y < rows; y++)	{
		CVecT *s = sDd.ptr<CVecT>(y);
		const int *idx = idx1i.ptr<int>(y);
		const double *w = weighted ? w1d.ptr<double>(y) : NULL;
		if (weighted){
			for (int x = 0; x < cols; x++)
				if (w[x] > EPS)
					_Guassians[idx[x]].Add(s[x], w[x]);
		}else{
			for (int x = 0; x < cols; x++)
				_Guassians[idx[x]].Add(s[x]);
		}
	}

	CmGaussian_<D>::MergeSimilar(_Guassians, _sumW, _ThrC, _ThrV);
	for (int i = 0; i < _K; i++)
		_Guassians[i].BuildGuassian(_sumW);
	AssignEachSample(sDd, idx1i, w1d);
}

template<int D> void CmGMM_<D>::AssignEachSample(CMat& sDd, Mat &idx1i, CMat &w1d)
{
	int rows = sDd.rows, cols = sDd.cols;
	if (sDd.isContinuous() && idx1i.isContinuous() && w1d.isContinuous())
		cols *= sDd.rows, rows = 1;
	for (int y = 0; y < rows; y++)	{
		CVecT *s = sDd.ptr<CVecT>(y); 
		int* idx = idx1i.ptr<int>(y); 
		const double* w = w1d.data == NULL ? NULL : w1d.ptr<double>(y);
		for (int x = 0; x < cols; x++)	{
			if (w != NULL && w[x] <= EPS)
				continue;			
			int k = 0;
			double maxP = 0;
			for (int i = 0; i < _K; i++) {
				double posb = P(i, s[x]);
				if (posb > maxP)
					k = i, maxP = posb;
			}
			idx[x] = k;
		}
	}
}

template<int D> void CmGMM_<D>::GetProbs(CMat& sDd, vector<Mat> &ps1d)
{
	ps1d.resize(_K);
	Mat total = Mat::zeros(sDd.size(), CV_64F);
	for (int i = 0; i < _K; i++){
		ps1d[i] = Mat::zeros(sDd.size(), CV_64F);
		if (getWeight(i) < EPS)
			continue;

		for (int y = 0; y < sDd.rows; y++)	{
			CVecT* s = sDd.ptr<CVecT>(y);
			double *p = ps1d[i].ptr<double>(y);
			for (int x = 0; x < sDd.cols; x++)	
				p[x] = P(i, s[x]);
		}
		total += ps1d[i];
	}
	for (int i = 0; i < _K; i++)
		divide(ps1d[i], total, ps1d[i]);
}

template<int D> Mat_<double> CmGMM_<D>::SaveToMat()
{
	Mat_<double> m1d(_K, 2 + D + D*D);
	for (int i = 0; i < _K; i++)
		_Guassians[i].SaveToMat().copyTo(m1d.row(i));
	return m1d;
}

template<int D> void CmGMM_<D>::ReadFromMat(const Mat_<double> &m1d)
{
	CV_Assert(m1d.type() == CV_64F && m1d.rows == _K && m1d.cols == 2 + D + D*D);
	for (int i = 0; i < _K; i++)
		_Guassians[i].ReadFromMat(m1d.row(i));
}

