#include "mywindow.h"
#include "ui_mywindow.h"
#include "ui_form.h"
#include <QFileDialog>
#include <QDebug>
#include "zz/graphCut/graph.h"
#include <set>
#include "multilevel.h"

//template<class T> T sqr(T a) {return a*a;}

#define FG_COMPONENTS 4
#define BG_COMPONENTS 8

MyWindow::MyWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MyWindow), fgGMM(FG_COMPONENTS), bgGMM(BG_COMPONENTS)
{
    this->setMouseTracking(true);
    ui->setupUi(this);
    ui->menubar->setNativeMenuBar(false);
    draw_mask.resize(1000,1000,0x00000000);
    k = 30;
    form.show();
    form.window = this;
    is_press = false;

    connect(form.ui->horizontalSlider, &QSlider::valueChanged,
            this, &MyWindow::colorDistanceChange);
    connect(this, SIGNAL(selection_changed()), this, SLOT(on_selection_changed()));
    //on_actionOpen_file_triggered();
    open("/home/cjld/Pictures/1405918-mountain_2.jpg");
    xmin = ymin = 0;
    xmax = ymax = 40;
    prev_pos = QPoint(0,0);
    each_selection = 100000;
    is_shift_press = false;
    worker = std::make_unique<std::thread>([&] {
        while (1) {
            unique_lock<mutex> lk(seed_lock);
            seed_cv.wait(lk, [&]{return seed.size() != 0;});
            seed_copy = seed;
            seed.clear();
            seed_lock.unlock();
            updateSeed();
        }
    });
}

void MyWindow::colorDistanceChange(int c) {
    k = c;
    form.update();
    qDebug() << c;
}

void MyWindow::drawCircle(MyImage &image, int x, int y, int r, int c) {
    int lx = x, rx = x;
    int ly = y-r, ry = y+r;
    ry = min(ry,image.h-1);
    ly = max(ly, 0);
    for (int yy = ly; yy<=ry; yy++) {
        if (yy<=y)
            while (sqr(lx-x)+sqr(yy-y) <= r*r) lx++;
        else
            while (sqr(lx-x-1)+sqr(yy-y) > r*r) lx--;
        rx = x+x-lx;
        int llx = min(lx,image.w);
        int rrx = max(rx+1, 0);
        for (int xx=rrx; xx<llx; xx++) {
            image.get(xx,yy) = c;
            checkSeed(xx,yy);
        }
    }
}

void MyWindow::drawLine(MyImage &image, int x1, int y1, int x2, int y2, int w, int c) {
    if (x1==x2 && y1==y2) return;
    double dx = x2-x1, dy = y2-y1;
    double norm = sqrt(dx*dx+dy*dy);
    dx /= norm, dy /= norm;
    double of1 = x1*dx + y1*dy;
    double of2 = x2*dx + y2*dy;
    double dx2 = dy, dy2 = -dx;

    double of3 = x1*dx2 + y1*dy2 - w;
    double of4 = x1*dx2 + y1*dy2 + w;

    double tmp = abs(dy2 * w);
    double py1, py2;
    if (y1>y2) {
        py1 = y1+tmp;
        py2 = y2-tmp;
    } else {
        py1 = y2+tmp;
        py2 = y1-tmp;
    }
    py2 = max(py2, 0.0);
    py1 = min(py1, image.h*1.0);
    for (int yy = floor(py2); yy<py1; yy++) {
        double xa,xb,xc,xd;
        if (dx == 0) {
            xa = -1e30;
            xb = 1e30;
        } else {
            xa = (of1 - yy*dy)/dx;
            xb = (of2 - yy*dy)/dx;
            if (xa>xb) swap(xa,xb);
        }

        if (dx2 == 0) {
            xc = -1e30;
            xd = 1e30;
        } else {
            xc = (of3 - yy*dy2)/dx2;
            xd = (of4 - yy*dy2)/dx2;
            if (xc>xd) swap(xc,xd);
        }
        xa = max(xa,xc);
        xb = min(xb,xd);
        xa = max(xa, 0.0);
        xb = min(xb, image.w*1.0);

        for (int xx=ceil(xa); xx<xb; xx++) {
            image.get(xx,yy) = c;
            checkSeed(xx,yy);
        }
    }
}

MyWindow::~MyWindow()
{
    delete ui;
}

void MyWindow::checkSeed(int x, int y) {
    if ((bool)selection.get(x,y) == (bool)is_delete)
        seed.push_back(make_pair(x,y));
}

int dx[4] = {0,1,0,-1};
int dy[4] = {1,0,-1,0};

Vec3d color2vec(unsigned int a) {
    int a1 = (a & 0xff0000) >> 16;
    int a2 = (a & 0x00ff00) >> 8;
    int a3 = a & 0x0000ff;
    return Vec3d(a1,a2,a3);
}

double colorDis(unsigned int a, unsigned int b) {
    int a1 = (a & 0xff0000) >> 16;
    int a2 = (a & 0x00ff00) >> 8;
    int a3 = a & 0x0000ff;
    int b1 = (b & 0xff0000) >> 16;
    int b2 = (b & 0x00ff00) >> 8;
    int b3 = b & 0x0000ff;
    return sqr(a1-b1) + sqr(a2-b2) + sqr(a3-b3);
}

bool MyWindow::colorDistance(unsigned int a, unsigned int b) {
    int a1 = (a & 0xff0000) >> 16;
    int a2 = (a & 0x00ff00) >> 8;
    int a3 = a & 0x0000ff;
    int b1 = (b & 0xff0000) >> 16;
    int b2 = (b & 0x00ff00) >> 8;
    int b3 = b & 0x0000ff;
    return sqr(a1-b1) + sqr(a2-b2) + sqr(a3-b3) <= cdis*cdis;
}

void MyWindow::seedDFS(int x, int y) {
    selection.get(x,y) = 0x5000ff00;
    for (int i=0; i<4; i++) {
        int xx = dx[i]+x;
        int yy = dy[i]+y;
        if (xx<0 || xx>=selection.w || yy<0 || yy>=selection.h)
            continue;
        if (selection.get(xx,yy) == 0 && colorDistance(image.get(x,y), image.get(xx,yy)))
            seedDFS(xx,yy);
    }
}


void MyWindow::seedAPGraphcut() {
    if (seed.size() == 0) return;
    int s=0;
    int color = 0x5000ff00;
    fgGMM.Reset();
    vector<double> vecWeight;
    vector<Vec3d> Seeds;
    Mat compli;
    auto myseed = seed;

    for (int x=xmin; x<xmax; x++) {
        for (int y=ymin; y<ymax; y++) {
            if (x<0 || x>=selection.w || y<0 || y>=selection.h) continue;
            bool is_edge = false;
            if (selection.get(x,y) == 0) continue;
            for (int i=0; i<4; i++) {
                int xx = dx[i]+x;
                int yy = dy[i]+y;
                if (xx<0 || xx>=selection.w || yy<0 || yy>=selection.h) continue;
                if (selection.get(xx,yy) == 0)  {
                    is_edge = true;
                    break;
                }
            }
            if (is_edge)
                myseed.push_back({x,y});
        }
    }

    for (int i=0; i<myseed.size(); i++) {
        int x = myseed[i].first;
        int y = myseed[i].second;
        selection.get(x,y) = color;
        vecWeight.push_back(1.0/myseed.size());
        Seeds.push_back(color2vec(image.get(x,y)));
    }

    Mat matWeightFg(1, Seeds.size(), CV_64FC1, &vecWeight[0]);
    Mat SeedSamples(1, Seeds.size(), CV_64FC3, &Seeds[0]);
    fgGMM.BuildGMMs(SeedSamples, compli, matWeightFg);
    fgGMM.RefineGMMs(SeedSamples, compli, matWeightFg);

    //int limit = (xmax - xmin)*(ymax - ymin);
    int limit = 1<<30;

    int d = 100;
    int imax = xmax+d, imin = xmin-d;
    int jmax = ymax+d, jmin = ymin-d;
    imax = min(selection.w, imax);
    jmax = min(selection.h, jmax);
    imin = max(imin, 0);
    jmin = max(jmin, 0);

    map<int,double> f;
    set<tuple<double, int, int>> pq;
    /*
    for (int i=0; i<myseed.size(); i++) {
        int x = myseed[i].first;
        int y = myseed[i].second;
        int id1 = y*selection.w + x;
        f[id1] = {0,1e30};
    }
    */

    double max_prop = 0;

    // get max_prop
    for (int j=jmin; j<jmax; j++)
        for (int i=imin; i<imax; i++) {
            int id1 = j*selection.w + i;
            int c1 = image.get(i,j);
            double fProp = fgGMM.P(color2vec(c1));
            max_prop = max(fProp, max_prop);
        }
    auto calc_prop_from_color = [&] (unsigned int c) -> double {
        double fProp = fgGMM.P(color2vec(c));
        fProp = -log(fProp / max_prop) * 16; //27
        if (fProp >= 1e30) fProp = 1e30;
        return fProp;
    };
    auto tune_dis = [&] (unsigned int c1, unsigned int c2) -> double {
        double dis = colorDis(c1,c2);
        //dis = exp(-dis/(255*255))*this->k*300;
        dis = 1 / (dis/255./255.+0.01);// * pow(this->k/30.0,3);
        return dis;
    };

    for (int i=0; i<myseed.size(); i++) {
        int x = myseed[i].first;
        int y = myseed[i].second;
        selection.get(x,y) = color;

        for (int d=0; d<4; d++) {
            int xx = dx[d]+x;
            int yy = dy[d]+y;
            if (xx<0 || xx>=selection.w || yy<0 || yy>=selection.h) continue;
            if (selection.get(xx,yy) != 0) continue;
            //int ii=x, jj=y, dd=d;
            //if (d>1) {ii=xx,jj=yy,dd=d-2;}

            int id2 = yy*selection.w + xx;
            if (f.find(id2) == f.end()) {
                int c2 = image.get(xx,yy);
                double fProp = calc_prop_from_color(c2);

                for (int d2=0; d2<4; d2++) {
                    int xx2 = dx[d2]+xx;
                    int yy2 = dy[d2]+yy;
                    if (xx2<0 || xx2>=selection.w || yy2<0 || yy2>=selection.h) continue;
                    int c1 = image.get(xx2,yy2);
                    double dis = tune_dis(c1,c2);
                    if (selection.get(xx2,yy2) != 0) {
                        fProp -= dis;
                    } else
                        fProp += dis;
                }
                f[id2] = fProp;

                pq.insert(make_tuple(fProp, xx, yy));
            }
        }
    }

    double maxflow = 0;

    vector<pair<int,int>> new_selection;

    while (!pq.empty()) {
        auto iter = pq.begin();
        auto top = *iter;
        pq.erase(top);
        int x = get<1>(top);
        int y = get<2>(top);
        int id = y*selection.w + x;
        double w = get<0>(top);
        //if (maxflow > 0) break;
        //qDebug() << maxflow;
        if (w != f[id]) {
            pq.insert(make_tuple(f[id], x, y));
            continue;
        }
        new_selection.push_back({x,y});
        selection.get(x,y) |= color;
        maxflow += w;
        if (maxflow > 10000) break;
        if (new_selection.size() > each_selection) break;

        for (int d=0; d<4; d++) {
            int xx = dx[d]+x;
            int yy = dy[d]+y;
            if (xx<0 || xx>=selection.w || yy<0 || yy>=selection.h) continue;
            if (selection.get(xx,yy) != 0) continue;
            //int ii=x, jj=y, dd=d;
            //if (d>1) {ii=xx,jj=yy,dd=d-2;}

            int id2 = yy*selection.w + xx;
            int c2 = image.get(xx,yy);
            double fProp = calc_prop_from_color(c2);
            if (f.find(id2) == f.end()) {

                for (int d2=0; d2<4; d2++) {
                    int xx2 = dx[d2]+xx;
                    int yy2 = dy[d2]+yy;
                    if (xx2<0 || xx2>=selection.w || yy2<0 || yy2>=selection.h) continue;
                    int c1 = image.get(xx2,yy2);
                    double dis = tune_dis(c1,c2);
                    if (selection.get(xx2,yy2) != 0) {
                        fProp -= dis;
                    } else
                        fProp += dis;
                }
                f[id2] = fProp;

                pq.insert(make_tuple(fProp, xx, yy));
            } else {
                int c1 = image.get(x,y);
                double dis = tune_dis(c1,c2);
                auto iter = pq.find(make_tuple(f[id2], xx, yy));
                pq.erase(iter);
                f[id2] -= 2*dis;
                pq.insert(make_tuple(f[id2], xx, yy));
            }
        }
    }
    /*
    for (auto i : new_selection) {
        selection.get(i.first,i.second) |= color;
    }
    */
    qDebug() << maxflow << new_selection.size();
    //qDebug() << f[10+24*selection.w];

}

void MyWindow::seedGraphcut() {
    if (seed.size() == 0) return;
    int s=0;
    int color = 0x500000ff;
    fgGMM.Reset();
    vector<double> vecWeight;
    vector<Vec3d> Seeds;
    Mat compli;

    for (int x=xmin; x<xmax; x++) {
        for (int y=ymin; y<ymax; y++) {
            if (x<0 || x>=selection.w || y<0 || y>=selection.h) continue;
            bool is_edge = false;
            if ((selection.get(x,y)&color) == 0) continue;
            for (int i=0; i<4; i++) {
                int xx = dx[i]+x;
                int yy = dy[i]+y;
                if (xx<0 || xx>=selection.w || yy<0 || yy>=selection.h) continue;
                if ((selection.get(xx,yy)&color) == 0)  {
                    is_edge = true;
                    break;
                }
            }
            if (is_edge)
                seed.push_back({x,y});
        }
    }

    for (int i=0; i<seed.size(); i++) {
        int x = seed[i].first;
        int y = seed[i].second;
        selection.get(x,y) |= color;
        vecWeight.push_back(1.0/seed.size());
        Seeds.push_back(color2vec(image.get(x,y)));
    }

    Mat matWeightFg(1, Seeds.size(), CV_64FC1, &vecWeight[0]);
    Mat SeedSamples(1, Seeds.size(), CV_64FC3, &Seeds[0]);
    fgGMM.BuildGMMs(SeedSamples, compli, matWeightFg);
    fgGMM.RefineGMMs(SeedSamples, compli, matWeightFg);

    //int limit = (xmax - xmin)*(ymax - ymin);
    int limit = 1<<30;

    int d = 100;
    int imax = xmax+d, imin = xmin-d;
    int jmax = ymax+d, jmin = ymin-d;
    imax = min(selection.w, imax);
    jmax = min(selection.h, jmax);
    imin = max(imin, 0);
    jmin = max(jmin, 0);


    int n = (imax-imin)*(jmax-jmin);
    Graph<double,double,double> g(n, n*4);
    g.add_node(n);
    double max_prop = 0;

    for (int j=jmin; j<jmax; j++)
        for (int i=imin; i<imax; i++) {
            int id1 = (j-jmin)*(imax-imin) + i-imin;
            int c1 = image.get(i,j);
            double fProp = fgGMM.P(color2vec(c1));
            max_prop = max(fProp, max_prop);
        }

    for (int j=jmin; j<jmax; j++)
        for (int i=imin; i<imax; i++) {
            int id1 = (j-jmin)*(imax-imin) + i-imin;
            int c1 = image.get(i,j);
            double fProp = fgGMM.P(color2vec(c1));
            fProp = -log(fProp / max_prop) * 16; //27
            //Prop = 0.01 * k*k;
            if ((selection.get(i,j)&color) == 0 && fProp>0) {
                g.add_tweights(id1, 0, fProp);
            }

            for (int d=0; d<4; d++) {
                int xx = dx[d]+i;
                int yy = dy[d]+j;
                if (xx<imin || xx>=imax || yy<jmin || yy>=jmax) continue;
                int id2 = (yy-jmin)*(imax-imin) + xx-imin;
                int c2 = image.get(xx,yy);
                double dis = colorDis(c1,c2);
                //dis = exp(-dis/(255*255))*this->k*300;
                dis = 1 / (dis/255./255.+0.001) * 60;// * pow(this->k/30.0,3);
                //dis=0;
                // TODO: normalize it
                g.add_edge(id1, id2, dis, dis);
            }
        }
    for (int i=0; i<seed.size(); i++) {
        int x = seed[i].first;
        int y = seed[i].second;
        int id1 = (y-jmin)*(imax-imin) + x-imin;
        g.add_tweights(id1, DBL_MAX, 0);
    }
    double flow = g.maxflow();
    qDebug() << "maxflow" << flow;
    for (int j=jmin; j<jmax; j++)
        for (int i=imin; i<imax; i++) {
            int id1 = (j-jmin)*(imax-imin) + i-imin;
            if (g.what_segment(id1) == Graph<double,double,double>::SOURCE) {
                selection.get(i,j) |= color;
            }
        }
}


void MyWindow::seedMultiGraphcut() {
    vector< pair<int,int> > &seed = seed_copy;
    if (seed.size() == 0) return;
    int s=0;
    int color = 0x500000ff;
    fgGMM.Reset();
    vector<double> vecWeight;
    vector<Vec3d> Seeds;
    Mat compli;

#define check_selection(selection, x, y) (bool)(selection.get(x,y)&color) == (bool)is_delete

    for (int x=xmin; x<xmax; x++) {
        for (int y=ymin; y<ymax; y++) {
            if (x<0 || x>=selection.w || y<0 || y>=selection.h) continue;
            bool is_edge = false;
            if (check_selection(selection, x, y)) continue;
            for (int i=0; i<4; i++) {
                int xx = dx[i]+x;
                int yy = dy[i]+y;
                if (xx<0 || xx>=selection.w || yy<0 || yy>=selection.h) continue;
                if (check_selection(selection, xx, yy))  {
                    is_edge = true;
                    break;
                }
            }
            if (is_edge)
                seed.push_back({x,y});
        }
    }

    for (int i=0; i<seed.size(); i++) {
        int x = seed[i].first;
        int y = seed[i].second;
        if (is_delete)
            selection.get(x,y) = 0;
        else
            selection.get(x,y) |= color;
        vecWeight.push_back(1.0/seed.size());
        Seeds.push_back(color2vec(image.get(x,y)));
    }

    Mat matWeightFg(1, Seeds.size(), CV_64FC1, &vecWeight[0]);
    Mat SeedSamples(1, Seeds.size(), CV_64FC3, &Seeds[0]);
    fgGMM.BuildGMMs(SeedSamples, compli, matWeightFg);
    fgGMM.RefineGMMs(SeedSamples, compli, matWeightFg);

    //int limit = (xmax - xmin)*(ymax - ymin);
    int limit = 1<<30;

    int d = 100;
    int imax = xmax+d, imin = xmin-d;
    int jmax = ymax+d, jmin = ymin-d;
    imax = min(selection.w, imax);
    jmax = min(selection.h, jmax);
    imin = max(imin, 0);
    jmin = max(jmin, 0);


    int n = (imax-imin)*(jmax-jmin);
    double max_prop = 0;

    for (int j=jmin; j<jmax; j++)
        for (int i=imin; i<imax; i++) {
            int id1 = (j-jmin)*(imax-imin) + i-imin;
            int c1 = image.get(i,j);
            double fProp = fgGMM.P(color2vec(c1));
            max_prop = max(fProp, max_prop);
        }
    Multilevel mt;
    mt.set_image(image);
    if (is_delete) {
        MyImage selection_reverse = selection;
        for (auto &c : selection_reverse.buffer) c ^= color;
        mt.set_selection(selection_reverse);
    } else
        mt.set_selection(selection);
    MyImage trimap = mt.update_seed(seed, fgGMM, max_prop);
    int c = color;
    if (!is_delete) c=0;
    for (int y=0; y<trimap.h; y++)
        for (int x=0; x<trimap.w; x++)
            if ((trimap.get(x,y)&255) > 255*0.75) {
                if (is_delete)
                    selection.get(x,y) = 0;
                else
                    selection.get(x,y) |= color;
            }
    emit selection_changed();
}

void MyWindow::keyPressEvent(QKeyEvent *e) {
    last_key = e->key();
    if (e->key() == Qt::Key_Shift) {
        is_shift_press = true;
    }
}

void MyWindow::keyReleaseEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Shift) {
        is_shift_press = false;
    }
}

void MyWindow::seedBFS() {
    if (seed.size() == 0) return;
    int s=0;
    int color = 0x5000ff00;
    fgGMM.Reset();
    vector<double> vecWeight;
    vector<Vec3d> Seeds;
    Mat compli;

    for (int x=xmin; x<xmax; x++) {
        for (int y=ymin; y<ymax; y++) {
            if (x<0 || x>=selection.w || y<0 || y>=selection.h) continue;
            bool is_edge = false;
            if (selection.get(x,y) == 0) continue;
            for (int i=0; i<4; i++) {
                int xx = dx[i]+x;
                int yy = dy[i]+y;
                if (xx<0 || xx>=selection.w || yy<0 || yy>=selection.h) continue;
                if (selection.get(xx,yy) == 0)  {
                    is_edge = true;
                    break;
                }
            }
            if (is_edge)
                seed.push_back({x,y});
        }
    }
/*
    for (int i=0; i<contour.size(); i++) {
        int x = contour[i].first;
        int y = contour[i].second;
        if (x<xmin || x>xmax || y<ymin || y>ymax) continue;
        seed.push_back(contour[i]);
    }
    */
    for (int i=0; i<seed.size(); i++) {
        int x = seed[i].first;
        int y = seed[i].second;
        selection.get(x,y) = color;
        vecWeight.push_back(1.0/seed.size());
        Seeds.push_back(color2vec(image.get(x,y)));
    }

    Mat matWeightFg(1, Seeds.size(), CV_64FC1, &vecWeight[0]);
    Mat SeedSamples(1, Seeds.size(), CV_64FC3, &Seeds[0]);
    fgGMM.BuildGMMs(SeedSamples, compli, matWeightFg);
    fgGMM.RefineGMMs(SeedSamples, compli, matWeightFg);

    //int limit = (xmax - xmin)*(ymax - ymin);
    int limit = 1<<30;

    while (s < seed.size() && s < limit) {
        int x = seed[s].first;
        int y = seed[s].second;
        selection.get(x,y) = color;
        for (int i=0; i<4; i++) {
            int xx = dx[i]+x;
            int yy = dy[i]+y;
            if (xx<0 || xx>=selection.w || yy<0 || yy>=selection.h)
                continue;
            if (selection.get(xx,yy) == 0) {
                double fProp = fgGMM.P(color2vec(image.get(xx,yy)));
                fProp = - log(fProp);
                if (fProp < k) {
                    selection.get(xx,yy) = color;
                    seed.push_back(make_pair(xx,yy));
                }
            }
            /*
            if (selection.get(xx,yy) == 0 && colorDistance(image.get(x,y), image.get(xx,yy))) {
                selection.get(xx,yy) = color;
                seed.push_back(make_pair(xx,yy));
            }
            */
        }
        s++;
    }

    contour.clear();
    for (int i=0; i<seed.size(); i++) {
        int x = seed[i].first;
        int y = seed[i].second;
        selection.get(x,y) = color;
        bool is_contour = false;
        for (int i=0; i<4; i++) {
            int xx = dx[i]+x;
            int yy = dy[i]+y;
            if (xx<0 || xx>=selection.w || yy<0 || yy>=selection.h)
                continue;
            if (selection.get(xx,yy) == 0) {
                is_contour = true;
                break;
            }
        }
        if (is_contour)
            contour.push_back(seed[i]);
    }
}

void MyWindow::updateSeed() {
    //seedAPGraphcut();
    //seedGraphcut();
    seedMultiGraphcut();
}

void MyWindow::draw(QPoint s, QPoint t) {
    //seed.clear();
    lock_guard<mutex> lock(seed_lock);
    brush_size = 5;
    is_delete = is_shift_press;
    int c = 0x50cc0000;
    if (!is_delete) c = 0xaa00cc00;
    drawCircle(draw_mask, t.x(), t.y(), brush_size, c);
    drawLine(draw_mask, s.x(), s.y(), t.x(), t.y(), brush_size, c);
    xmin = t.x(), xmax = s.x();
    if (xmin>xmax) swap(xmin,xmax);
    ymin = t.y(), ymax = s.y();
    if (ymin>ymax) swap(ymin,ymax);
    xmin -= brush_size * 5;
    xmax += brush_size * 5;
    ymin -= brush_size * 5;
    ymax += brush_size * 5;
    seed_cv.notify_one();
    //updateSeed();
}

void MyWindow::on_selection_changed() {
    repaint();
}

void MyWindow::paintEvent(QPaintEvent *e) {
    QPainter painter(this);

    offset = 30;
    QImage qimage((uchar *)&image.get(), image.w, image.h, QImage::Format_RGB32);
    painter.drawImage(0,offset,qimage);
    QImage mask((uchar *)&draw_mask.get(), draw_mask.w, draw_mask.h, QImage::Format_ARGB32);
    painter.drawImage(0,offset,mask);
    {
        lock_guard<mutex> lock(selection_lock);
        QImage qselection((uchar *)&selection.get(), draw_mask.w, draw_mask.h, QImage::Format_ARGB32);
        painter.drawImage(0,offset,qselection);
    }
/*
    QPainterPathStroker st;
    st.setWidth(10);
    st.setJoinStyle(Qt::RoundJoin);
    st.setCapStyle(Qt::RoundCap);

    QPainterPath spath = st.createStroke(fg_path);
    QPainterPath spath2 = st.createStroke(bg_path);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0,255,0,100));
    painter.drawPath(spath);
    painter.setBrush(QColor(255,0,0,100));
    painter.drawPath(spath2);
*/
    form.update();
}

void MyWindow::mouseMoveEvent(QMouseEvent *e) {
    QPoint pp = e->pos();
    pp.setY(pp.y()-offset);
    if (is_press) {
        QPoint d = pp - prev_pos;
        stroke_len += sqrt(d.x()*d.x() + d.y()*d.y());
        cdis = k;
        current_path->lineTo(pp);
        draw(prev_pos, pp);
        prev_pos = pp;
        //this->repaint();
        update();
    } else {
        prev_pos = pp;
        form.update();
    }
}

void MyWindow::mousePressEvent(QMouseEvent *e) {
    QPoint pp = e->pos();
    pp.setY(pp.y()-offset);
    stroke_len = 0;
    cdis = k;
    prev_pos = pp;
    if (e->button()==Qt::LeftButton) {
        current_path = &fg_path;
        is_press = true;
        draw(prev_pos, pp);
    } else {
        current_path = &bg_path;
    }
    //mousepath = QPainterPath();
    current_path->moveTo(pp);
    update();
}

void MyWindow::mouseReleaseEvent(QMouseEvent *) {
    is_press = false;
    contour.clear();
}

void MyWindow::open(QString fileName) {
    QImage img(fileName);
    img = img.convertToFormat(QImage::Format_RGB32);
    draw_mask.resize(img.width(), img.height());
    selection.resize(img.width(), img.height());
    image.resize(img.width(), img.height());
    memcpy(&image.buffer[0], img.bits(), 4*img.width()*img.height());
    seed.clear(); contour.clear();
    this->resize(img.width(), img.height()+60);
    update();
}

void MyWindow::on_actionOpen_file_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                 "/home",
                                                 tr("Images (*.png *.xpm *.jpg)"));
    open(fileName);
}
