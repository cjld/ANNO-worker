#include "CmShow.h"

#include "CmLog.h"
#include "CmGMM_.h"
#include "CmFile.h"

void CmGMM3D::View(const CmGMM3D &gmm, CStr &title, bool decreaseShow)
{
	int K = gmm.K();
	Mat_<Vec3f> color3f(1, K);
	Mat_<double> weight1d(1, K), cov1d(1, K);
	for (int i = 0; i < K; i++)	
	{
		color3f(0, i) = gmm.getMean(i);
		weight1d(0, i) = gmm.getWeight(i);
		cov1d(0, i) = sqrt(gmm.GetGaussians()[i].det);

	}
	
	CmShow::HistBins(color3f, weight1d, title, decreaseShow, cov1d);

}

double CmGMM3D::ViewFrgBkgProb(const CmGMM3D &fGMM, const CmGMM3D &bGMM, CStr &title)
{
	double Pf = fGMM.getSumWeights(), Pb = bGMM.getSumWeights(), sumN = Pf + Pb;
	Pf /= sumN, Pb /= sumN;
	CmLog::LogLine("#ForeSample = %.2g%%, ", Pf);	
	int n = 6 * 6 * 6;
	Mat color3f(1, n, CV_32FC3);
	Vec3f* colors = (Vec3f*)(color3f.data);
	for (int i = 0; i < n; i++){
		colors[i][0] = ((float)(i / 36) + 0.5f)/6.f;
		int idx = i % 36;
		colors[i][1] = ((float)(idx / 6) + 0.5f)/6.f;
		colors[i][2] = ((float)(idx % 6) + 0.5f)/6.f;
	}

	Mat fCount1d(1, n, CV_64F), bCount1d(1, n, CV_64F), rCount1d;
	double *fCount = (double*)(fCount1d.data), *bCount = (double*)(bCount1d.data);
	for (int i = 0; i < n; i++)
		fCount[i] = fGMM.P(colors[i]), bCount[i] = bGMM.P(colors[i]);
	double fMax, fMin, bMax, bMin;
	minMaxLoc(fCount1d, &fMin, &fMax);
	minMaxLoc(bCount1d, &bMin, &bMax);
	CmLog::LogLine("minF = %.2g, maxF = %.2g, minB = %.2g maxB = %.2g\n", fMin, fMax, bMin, bMax);
	double epsilon = max(fMax, bMax) * 1e-2;
	Mat res = (fCount1d*Pf + 0.5*epsilon)/(fCount1d*Pf + bCount1d*Pb + epsilon) - 0.5;
	CmShow::HistBins(color3f, res, title, true);
	return epsilon;
}

void CmGMM3D::GetGMMs(CStr &smplW, CStr &annoExt, CmGMM3D &fGMM, CmGMM3D &bGMM)
{
	vector<Vec3d> frg, bkg;
	frg.reserve(10000000);
	bkg.reserve(10000000);
	vecS namesNE;
	string dir, ext = CmFile::GetExtention(smplW);
	int num = CmFile::GetNamesNE(smplW, namesNE, dir);
	num = min(50, num);
	int foreBinN = 0, backBinN = 0;
	for (int i = 0; i < num; i++)	{
		Mat mask1u = imread(dir + namesNE[i] + annoExt, CV_LOAD_IMAGE_GRAYSCALE);
		Mat img = imread(dir + namesNE[i] + ext);
		CV_Assert(mask1u.data != NULL && img.data != NULL);
		img.convertTo(img, CV_64FC3, 1.0/255);
		for (int r = 0; r < mask1u.rows; r++)	{
			byte *mVal = mask1u.ptr<byte>(r);
			Vec3d *imgVal = img.ptr<Vec3d>(r);
			for (int c = 0; c < mask1u.cols; c++)
				if (mVal[c] > 200)
					frg.push_back(imgVal[c]);
				else if (mVal[c] < 100)
					bkg.push_back(imgVal[c]);
		}
	}

	Mat fore3d(1, frg.size(), CV_64FC3, &frg[0]);
	Mat bg3d(1, bkg.size(), CV_64FC3, &bkg[0]);
		

	Mat fComp1i, bComp1i; // foreground and background components
	fGMM.BuildGMMs(fore3d, fComp1i);
	fGMM.RefineGMMs(fore3d, fComp1i);
	fore3d.release();
	fComp1i.release();
	bGMM.BuildGMMs(bg3d, bComp1i);
	bGMM.RefineGMMs(bg3d, bComp1i);
}

void CmGMM3D::Demo(CStr &wkDir)
{
	CStr inDir = wkDir + "In/", outDir = wkDir + "Out/";
	vecS namesNE;
	int imgNum = CmFile::GetNamesNE(inDir + "*.png", namesNE);
	CmFile::MkDir(outDir);

	int numOfComp = 8;

	string fileName = wkDir + "data.yml";
	FileStorage fsW(fileName, FileStorage::WRITE);

	for (int i = 0; i < imgNum; i++){
		Mat idx1i, img = imread(inDir + namesNE[i] + ".png");
		CV_Assert(img.data != NULL);
		CmGMM3D gmm(numOfComp);
		img.convertTo(img, CV_64F, 1/255.0);
		gmm.BuildGMMs(img, idx1i);
		gmm.RefineGMMs(img, idx1i);
		fsW<<format("GMM%d", i)<<gmm.SaveToMat();

		Mat showComp(img.size(), CV_64FC3);
		for (int y = 0; y < img.rows; y++) {
			Vec3d *showC = showComp.ptr<Vec3d>(y);
			const int *idx = idx1i.ptr<int>(y);
			for (int x = 0; x < img.cols; x++)
				showC[x] = gmm.getMean(idx[x]);
		}
		imwrite(outDir + namesNE[i] + "_C.png", showComp*255);

		CmGMM3D::View(gmm, "GMM test");

	}
	fsW.release();
	FileStorage fsR(fileName, FileStorage::READ);

	for (int i = 0; i < imgNum; i++){
		Mat img = imread(inDir + namesNE[i] + ".png");
		CV_Assert(img.data != NULL);
		CmGMM3D gmm(numOfComp);
		img.convertTo(img, CV_64F, 1/255.0);
		CmFile::Copy(inDir + namesNE[i] + ".png", outDir + namesNE[i] + ".jpg");
		Mat gmmData1d;
		fsR[format("GMM%d", i)]>>gmmData1d;
		gmm.ReadFromMat(gmmData1d);

		vecM probs;
		gmm.GetProbs(img, probs);
		for (int c = 0; c < gmm.K(); c++)
			imwrite(outDir + namesNE[i] + format("_C%d.png", c), probs[c]*255);

	}
	fsR.release();
}

void CmGMM3D::DemoGroup(CStr &wkDir)
{
	CmGMM3D fGMM(10), bGMM(10);
	GetGMMs(wkDir + "Sort1/*.jpg", ".png", fGMM, bGMM);
	View(bGMM, wkDir + "Statis/2bGMM.png");
	View(fGMM, wkDir + "Statis/2fGMM.png");
	ViewFrgBkgProb(fGMM, bGMM, wkDir + "Statis/2fProb.png");
}
