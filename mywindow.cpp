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
    open("/home/cjld/Pictures/1405918-mountain_2.jpg");
    prev_pos = QPoint(0,0);
    is_shift_press = false;
    //findContourTest();
    diluteTest();
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
    QPainter painter(this);

    offset = 30;
    QImage qimage((uchar *)&ctl.image.get(), ctl.image.w, ctl.image.h, QImage::Format_RGB32);
    painter.drawImage(0,offset,qimage);
    QImage mask((uchar *)&ctl.draw_mask.get(), ctl.draw_mask.w, ctl.draw_mask.h, QImage::Format_ARGB32);
    painter.drawImage(0,offset,mask);
    {
        //lock_guard<mutex> lock(ctl.selection_lock);
        QImage qselection((uchar *)&ctl.selection.get(), ctl.draw_mask.w, ctl.draw_mask.h, QImage::Format_ARGB32);
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
