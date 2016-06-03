#include "alphamatting.h"

using namespace cv;

#define epsilon	(1e-7)
#define TRI_WIDTH 8

void alphamatting::GetTriMap(Mat& trimap) {
    Mat tmpImage = trimap.clone();

    Mat element(TRI_WIDTH, TRI_WIDTH, CV_8UC1);

    dilate(tmpImage, tmpImage, Mat(), cv::Point(-1, -1), 3);
    erode(trimap, trimap, Mat(), cv::Point(-1, -1), 3);

    for (int h = 0; h < trimap.size().height; ++h)
    {
        uchar* rowTemp = tmpImage.ptr<uchar>(h);
        uchar* rowTrimap = trimap.ptr<uchar>(h);

        for (int w = 0; w < trimap.size().width; ++w)
        {
            if (rowTemp[w] > 200 && rowTrimap[w] < 100)
            {
                rowTrimap[w] = 128;
            }
        }
    }
}
