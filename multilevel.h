#ifndef MULTILEVEL_H
#define MULTILEVEL_H
#include "mywindow.h"

class Multilevel
{
public:
    int min_size;
    int th, th_low, th_high; // 25 means 25%

    Multilevel();
    void set_image(MyImage img);
    void set_selection(MyImage selection);
    MyImage update_seed(vector<pair<int,int>> seeds, CmGMM3D &fgGMM, double max_prop);
    // 3*3 avg down sample
    MyImage down_sample(MyImage &img);
    // low is the mask, high is the origin image
    // Joint Bilateral Upsampling (JBU) [Kopf et al. 2007]
    MyImage up_sample(MyImage &low, MyImage &high);
    vector<MyImage> imgs;
    vector<MyImage> selections;
    void test();
    void print(MyImage &a);
};

#endif // MULTILEVEL_H
