#ifndef MYWINDOW_H
#define MYWINDOW_H

#include <QMainWindow>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPainter>
#include "form.h"
#include "GMM/CmGMM_.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include "multilevel.h"

namespace Ui {
class MyWindow;
}

#include <vector>

using namespace std;


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

    void open(QString);

    QPoint prev_pos;
    Form form;
    int offset;
    int is_shift_press;
    bool is_press;
    MultilevelController ctl;

signals:
    void selection_changed();

private slots:
    void on_actionOpen_file_triggered();
    void on_selection_changed();

private:
    Ui::MyWindow *ui;
};

#endif // MYWINDOW_H
