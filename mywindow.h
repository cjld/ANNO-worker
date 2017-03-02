#ifndef MYWINDOW_H
#define MYWINDOW_H

#include <QMainWindow>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPainter>
#include "form.h"
#include "zz/GMM/CmGMM_.h"
#include <mutex>
#include <condition_variable>
#include <thread>

namespace Ui {
class MyWindow;
}

#include <vector>

using namespace std;
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
    unsigned int &get(int i=0, int j=0) {return buffer[i+j*w];}
};

Vec3d color2vec(unsigned int a);
double colorDis(unsigned int a, unsigned int b);

class MyWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyWindow(QWidget *parent = 0);
    ~MyWindow();

    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);
    int last_key;


    void drawCircle(MyImage &image, int x, int y, int r, int c);
    void drawLine(MyImage &image, int x1, int y1, int x2, int y2, int w, int c);
    void draw(QPoint s, QPoint t);
    void checkSeed(int x, int y);
    void updateSeed();
    void seedDFS(int x, int y);
    void seedBFS();
    void seedGraphcut();
    void seedAPGraphcut();
    bool colorDistance(unsigned int a, unsigned int b);
    void colorDistanceChange(int);
    void open(QString);
    void seedMultiGraphcut();

    QPainterPath fg_path,bg_path, *current_path;
    MyImage draw_mask, image, selection;
    int each_selection;
    QPoint prev_pos;
    vector< pair<int,int> > seed, contour, seed_copy;
    double cdis, stroke_len, k;
    Form form;
    bool is_press;
    int offset;
    CmGMM3D fgGMM,bgGMM;
    int xmin,xmax,ymin,ymax, brush_size;
    int is_shift_press, is_delete;

    mutex seed_lock, selection_lock;
    condition_variable seed_cv;
    unique_ptr<std::thread> worker;

signals:
    void selection_changed();

private slots:
    void on_actionOpen_file_triggered();
    void on_selection_changed();

private:
    Ui::MyWindow *ui;
};

#endif // MYWINDOW_H
