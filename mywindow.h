#ifndef MYWINDOW_H
#define MYWINDOW_H

#include <QMainWindow>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPainter>

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
        buffer.resize(w*h, fill);
    }
    unsigned int &get(int i=0, int j=0) {return buffer[i+j*w];}
};

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


    QPainterPath fg_path,bg_path, *current_path;
    MyImage draw_mask;
    QPoint prev_pos;


private slots:
    void on_actionOpen_file_triggered();

private:
    Ui::MyWindow *ui;
};

#endif // MYWINDOW_H
