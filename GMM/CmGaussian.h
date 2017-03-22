#pragma once

#include "CmDefinition.h"

template <int D> class CmGaussian_
{
public: // Data associated with this Gaussian
	typedef Vec<double, D> VecT; 
	typedef const VecT CVecT;
	typedef Matx<double, D, D> MatT;

	double w;	// Weighting of this Gaussian in the GMM.
	double det;	// Determinant of the covariance matrix
	VecT mean;	// Mean value
	MatT inv;	// Inverse of the covariance matrix

	// These are only needed during Orchard and Bouman clustering.
	VecT eVals; // Eigenvalues of covariance matrix
	MatT eVecs; // Eigenvectors of the covariance matrix

public:	
	inline void Add(CVecT &sample, const double weight);
	inline void Add(CVecT &sample);
	inline double DistC(const CmGaussian_<D> g) {return vecDist(mean, g.mean);}
	void BuildGuassian(double totalCount, bool computeEigens = false); // Build the Gaussian out of all the added samples
	static void MergeSimilar(vector<CmGaussian_<D>> gs, double totalCount, double thrC, double thrV);

	inline double Count() {return ct;}
	void Reset() {w = det = 0; mean = 0; inv = 0; eVals = 0; eVecs = 0; ResetDataForGuassian(); }
	void ResetDataForGuassian() {ct = 0; s = 0; p = 0;}
	void ReadFromMat(const Mat_<double> &m1d); // read from a 1 x N Mat with N = 2 + D + D*D
	Mat_<double> SaveToMat(); // Save to a 1 x N Mat with N = 2 + D + D*D
	CmGaussian_() {Reset();}

private: // Data for constructing this Gaussian using data samples
	double ct; //Count of color samples added to the Gaussian
	VecT s;		// Sum of r, g, and b
	MatT p;		// matrix of products (i.e. r*r, r*g, r*b for D = 3 colors).

	void Merge(CmGaussian_<D> &g);
};
typedef CmGaussian_<3> CmGaussian3;

template<int D> void CmGaussian_<D>::ReadFromMat(const Mat_<double> &m1d)
{
	double* d = (double*)m1d.data;
	int num = 2 + D + D * D;
	if (!(m1d.type() == CV_64F && m1d.rows == 1 && m1d.cols == num))
		CV_Error(CV_StsUnsupportedFormat, "Invalidate model data in ReadFromMat");
	w = m1d(0);
	det = m1d(1);
	memcpy(&mean(0), &m1d(2), sizeof(double)*D);
	memcpy(&inv(0, 0), &m1d(2+D), sizeof(double)*D*D);

	eVecs = 0, eVals = 0;
	ResetDataForGuassian();
}

template<int D> Mat_<double> CmGaussian_<D>::SaveToMat()
{
	Mat_<double> m1d(1, 2 + D + D*D);
	m1d(0) = w;
	m1d(1) = det;
	memcpy(&m1d(2), &mean(0), sizeof(double)*D);
	memcpy(&m1d(2+D), &inv(0, 0), sizeof(double)*D*D);
	return m1d;
}

template<int D> void CmGaussian_<D>::Add(CVecT &sample)
{
	s += sample;
	for (int i = 0; i < D; i++)	
		for (int j = i; j < D; j++)
			p(i, j) += sample(i)*sample(j);
	ct++;
}

template<int D> void CmGaussian_<D>::Add(CVecT &sample, const double weight)
{
	s += sample * weight;
	for (int i = 0; i < D; i++)	
		for (int j = i; j < D; j++)
			p(i, j) += sample(i)*sample(j) * weight;
	ct += weight;
}

// Build the Gaussian out of all the added samples
template<int D> void CmGaussian_<D>::BuildGuassian(double totalCount, bool computeEigens)
{
	// Running into a singular covariance matrix is problematic. So we'll add a small epsilon
	// value to the diagonal elements to ensure a positive definite covariance matrix.
	const double Epsilon = 0.001;
	if (ct < Epsilon)
		w = 0;
	else {// Compute mean of Gaussian and covariance matrix		
		MatT covar; // Covariance matrix of the Gaussian
		mean = s / ct;
		for (int i = 0; i < D; i++)	{
			for (int j = i; j < D; j++)
				covar(j,i) = covar(i,j) = p(i, j)/ct - mean(i) * mean(j);
			covar(i, i) += Epsilon;
		}
		invert(covar, inv, CV_LU); // Compute determinant and inverse of covariance matrix
		det = determinant(covar);
		w = ct/totalCount; // Weight is percentage of this Gaussian
		MatT tmp;
		if (computeEigens)
			SVD::compute(covar, eVals, eVecs, tmp);
	}
}

template<int D> void CmGaussian_<D>::Merge(CmGaussian_<D> &g)
{
	s += g.s;
	p += g.p;
	ct += g.ct;
	g.Reset();
}

template<int D> void CmGaussian_<D>::MergeSimilar(vector<CmGaussian_<D>> gs, double totalCount, double thrC, double thrV)
{
	int numG = (int)gs.size();
	for (int i = 0; i < numG; i++)
		gs[i].BuildGuassian(totalCount, true);

	for (int i = 0; i < numG; i++) {
		if (gs[i].ct < EPS || gs[i].eVals[0] > thrV)
			continue;
		int mostIdx = 0;
		double mostValue = numeric_limits<double>::max();
		for (int j = 0; j < i; j++) {
			if (gs[j].ct < EPS)
				continue;
			double crnt = gs[i].DistC(gs[j]);
			if (crnt < mostValue)
				mostValue = crnt, mostIdx = j;
		}
		if (mostValue < thrC)
			gs[i].Merge(gs[mostIdx]);
	}
}
