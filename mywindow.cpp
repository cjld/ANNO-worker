#include "mywindow.h"
#include "ui_mywindow.h"
#include "ui_form.h"
#include <QFileDialog>
#include <QDebug>
#include "graphCut/graph.h"
#include <set>
#include "multilevel.h"

//template<class T> T sqr(T a) {return a*a;}

void findContourTest() {
    MyImage image(10,10), selection(10,10);
    for (int y=0; y<image.h; y++)
        for (int x=0; x<image.w; x++)
            if (((x/3 ^ y/3)&1) == 0) {
                image.get(x,y) = 1;
                if (x%3 == 1 && y %3 == 1)
                    image.get(x,y) = 0;
            }
    Multilevel::print(image);
    vector<vector<pair<int,int>>> contours;
    findContours(image, contours);
    for (auto v : contours) {
        cerr << "[ ";
        for (auto xy : v) cerr << "[" << xy.first << "," << xy.second << "], ";
        cerr << " ]" << endl;
    }

    QPainterPath cutSelectPath = QPainterPath();
    for (int i = 0; i < (int)contours.size(); i++) {
        cutSelectPath.moveTo(QPoint(contours[i][0].first, contours[i][0].second));
        for (auto xy: contours[i])
            cutSelectPath.lineTo(QPoint(xy.first, xy.second));
        cutSelectPath.lineTo(QPoint(contours[i][0].first, contours[i][0].second));
    }
    QImage qStrokeMask = QImage(image.w, image.h, QImage::Format_ARGB32_Premultiplied);
    qStrokeMask.fill(0);
    QPainter pt;
    pt.begin(&qStrokeMask);
    pt.setBrush(Qt::white);
    pt.setPen(Qt::NoPen);
    pt.drawPath(cutSelectPath);
    int segs=0;
    for (int y=0; y<image.h; y++) {
        QRgb* scanelineStrokeMask = (QRgb*)qStrokeMask.scanLine(y);
        for (int x=0; x<image.w; x++) {
            int grayValue = qGray(scanelineStrokeMask[x]);
            if (grayValue>100) {
                selection.get(x,y) = 255;
                segs ++;
            } else
                selection.get(x,y) = 0;
        }
    }
    Multilevel::print(selection);
}

void diluteTest() {
    MyImage image(10,10), img1(10,10), img2(10,10);
    for (int y=0; y<image.h; y++)
        for (int x=0; x<image.w; x++)
            if (((x/3 ^ y/3)&1) == 0) {
                image.get(x,y) = 0;
                if (x%3 == 1 && y %3 == 1)
                    image.get(x,y) = 255;
            }
    for (int y=0; y<image.h; y++)
        for (int x=0; x<image.w; x++) {
            image.get(x,y) = (~image.get(x,y))&255;
        }
    Multilevel::print(image);
    Multilevel::dilute(image, 1);
    for (int y=0; y<image.h; y++)
        for (int x=0; x<image.w; x++) {
            auto c = image.get(x,y) >> 16;
            img1.get(x,y) = (~c&1)*255;
            img2.get(x,y) = ((c>>1)&1)*255;
        }
    Multilevel::print(img1);
    Multilevel::print(img2);
}

void GMM_Test() {
    CmGMM3D gmm(2);
    vector<double> vecWeight;
    vector<Vec3d> Seeds = {Vec3d(0,0,0), Vec3d(1,1,1), Vec3d(1,1,0.9)};
    Mat compli;
    for (int i=0; i<(int)Seeds.size(); i++) {
        vecWeight.push_back(1.0/Seeds.size());
        //Seeds.push_back(color2vec(image.get(x,y)));
    }
    Mat matWeightFg(1, Seeds.size(), CV_64FC1, &vecWeight[0]);
    Mat SeedSamples(1, Seeds.size(), CV_64FC3, &Seeds[0]);
    gmm.BuildGMMs(SeedSamples, compli, matWeightFg);
    gmm.RefineGMMs(SeedSamples, compli, matWeightFg);
    for (int i=0; i<gmm.K(); i++) {
        auto g = gmm.GetGaussians()[i];
        cerr << "mean: " << gmm.getMean(i) << " weight: " << gmm.getWeight(i) <<
                " w: " << g.w << " det: " << g.det << endl;
    }
    vector<Vec3d> pt = {Vec3d(0.5,0.5,0.5), Vec3d(1,1,0.85)};
    for (auto p : Seeds) pt.push_back(p);
    for (auto p : pt) {
        cerr << "test: " << p << " P: " << gmm.P(p) << endl;
    }
    CmGMM3D::View(gmm, "gmm");
    //exit(0);
}

MyWindow::MyWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MyWindow)
{
    this->setMouseTracking(true);
    ui->setupUi(this);
    ui->menubar->setNativeMenuBar(false);
    form.show();
    form.window = this;
    is_press = false;

    connect(&ctl, SIGNAL(selection_changed()), this, SLOT(on_selection_changed()));
    //on_actionOpen_file_triggered();
    open("/home/cjld/Pictures/_Cardinal_7_6072_60749_6074943118_6074943118.jpg");
    //open("/home/cjld/Pictures/maxresdefault.jpg");
    prev_pos = QPoint(0,0);
    is_shift_press = false;
    //findContourTest();
    //diluteTest();
    //GMM_Test();
    //return;
    ctl.new_stroke();
    ctl.brush_size = 40;
    ctl.draw(QPoint(200,200), QPoint(700,700));
    //ctl.draw(QPoint(149,230), QPoint(150,260));
    ctl.draw(QPoint(1300,600), QPoint(800,600));
}


MyWindow::~MyWindow()
{
    delete ui;
}


int dx[4] = {0,1,0,-1};
int dy[4] = {1,0,-1,0};

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


void MyWindow::on_selection_changed() {
    repaint();
}

void MyWindow::paintEvent(QPaintEvent *e) {
    QImage image(ctl.image.w, ctl.image.h, QImage::Format_ARGB32_Premultiplied);
    image.fill(qRgba(0,0,0,0));
    QPainter painter;
    painter.begin(&image);

    offset = 30;
    QImage qimage((uchar *)&ctl.image.get(), ctl.image.w, ctl.image.h, QImage::Format_RGB32);
    painter.drawImage(0,0,qimage);
    QImage mask((uchar *)&ctl.draw_mask.get(), ctl.draw_mask.w, ctl.draw_mask.h, QImage::Format_ARGB32);
    painter.drawImage(0,0,mask);
    {
        //lock_guard<mutex> lock(ctl.selection_lock);
        auto v = ctl.selection.buffer;
        for (auto &x : v)
            if (x) x |= 0x80000000;
        QImage qselection((uchar *)&v[0], ctl.draw_mask.w, ctl.draw_mask.h, QImage::Format_ARGB32);
        painter.drawImage(0,0,qselection);
    }
    painter.end();
    image.save("gmmcmp8.png");
    QPainter wpt(this);
    wpt.drawImage(0,offset,image);
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
        ctl.is_delete = is_shift_press;
        ctl.draw(prev_pos, pp);
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
    prev_pos = pp;
    if (e->button()==Qt::LeftButton) {
        is_press = true;
        ctl.is_delete = is_shift_press;
        ctl.new_stroke();
        ctl.draw(prev_pos, pp);
    } else
    update();
}

void MyWindow::mouseReleaseEvent(QMouseEvent *) {
    is_press = false;
}

void MyWindow::open(QString fileName) {
    QImage img(fileName);
    ctl.setImage(img);
    this->resize(img.width()+60, img.height()+60);
    update();
}

void MyWindow::on_actionOpen_file_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                 "/home",
                                                 tr("Images (*.png *.xpm *.jpg)"));
    open(fileName);
}
