#ifndef MULTILEVEL_H
#define MULTILEVEL_H
#include "GMM/CmGMM_.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <QObject>
#include <QPoint>
#include <QImage>
#include <QDebug>
#include "jsoncons/json.hpp"
using jsoncons::json;


class MyImage {
public:
    int w,h;
    vector<unsigned int> buffer;
    MyImage(int _w=0, int _h=0) : w(_w), h(_h) {
        buffer.resize(w*h, 0);
    }

    void resize(int _w, int _h, unsigned int fill=0) {
        w = _w, h = _h;
        buffer.resize(w*h);
        for (int i=0; i<buffer.size(); i++)
            buffer[i] = fill;
    }
    unsigned int &get(int i=0, int j=0) {
        //if (i+j*w>=buffer.size() || i+j*w<0)
        //    qDebug() << "out of range";
        return buffer[i+j*w];
    }
    unsigned int getlastb(int i=0, int j=0) {return buffer[i+j*w]&255;}
    void dump(string fname, int id) {
        fname = fname + (char)('0' + id);
        cerr << "dump " << fname << endl;
        ofstream fout(fname);
        fout<< w << ' ' << h << endl;
        for (auto &x : buffer)
            for (int i=0; i<4;i++)
                fout << ((x>>(i*8))&255) << ' ';
    }

    static MyImage background;
    static void set_dump_img(MyImage &img);
    void dump_image(string fname, int id, int channel, bool composite=true);
};

Vec3d color2vec(unsigned int a);
double colorDis(unsigned int a, unsigned int b);
void findContours(MyImage &image, vector<vector<pair<int,int>>> &contours);

class Multilevel
{
public:
    int min_size, minx, miny, maxx, maxy;
    int th, th_low, th_high; // 25 means 25%
    int next_minx, next_maxx;
    int next_miny, next_maxy;

    Multilevel();
    void set_image(MyImage img);
    void set_selection(MyImage selection);
    void update_seed(vector<pair<int,int>> seeds, CmGMM3D &fgGMM, double max_prop, vector<pair<int, int> > &bseeds);
    // 3*3 avg down sample
    MyImage down_sample(MyImage &img);
    // low is the mask, high is the origin image
    // Joint Bilateral Upsampling (JBU) [Kopf et al. 2007]
    MyImage up_sample(MyImage &low, MyImage &high);
    void up_sample_fast(MyImage &low, MyImage &high, MyImage &store);
    void up_sample_dilute(MyImage &low, MyImage &high, MyImage &store);
    static void dilute(MyImage &low, int size);
    void dilute_fast(MyImage &low, int size);
    vector<MyImage> imgs;
    vector<MyImage> selections;
    void test();
    static void print(MyImage &a);
};


class MultilevelController : public QObject {
    Q_OBJECT

public:
    MultilevelController();

    void drawCircle(MyImage &image, int x, int y, int r, int c);
    void drawLine(MyImage &image, int x1, int y1, int x2, int y2, int w, int c);
    void draw(QPoint s, QPoint t);
    void updateSeed();
    void checkSeed(int x, int y);

    void seedMultiGraphcut();
    void setImage(QImage);
    void printContours(json header);
    void new_stroke();

    json load_url(string);
    json load_base64(string);
    json paint(json);
    json load_region(json);


    MyImage draw_mask, image, selection;
    MyImage stroke_id;
    unsigned int current_stroke_id;
    std::vector<pair<int,int>> *current_pixel;
    std::map<unsigned int,int> stroke_isbg;
    std::map<unsigned int,std::vector<pair<int,int>>> stroke_pixels;
    vector< pair<int,int> > seed, contWour, seed_copy;
    double cdis, stroke_len, k;
    CmGMM3D fgGMM,bgGMM;
    Multilevel mt;
    int xmin,xmax,ymin,ymax, brush_size;
    int is_delete;

    mutex seed_lock, selection_lock;
    condition_variable seed_cv;
    unique_ptr<std::thread> worker;
    json res_header;
    mutex *output_lock;
    bool is_changed;

signals:
    void selection_changed();
};

#endif // MULTILEVEL_H
