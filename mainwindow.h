#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPaintEvent>
#include <QPainterPath>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);

    void prepare();
    void test_loadurl();

    QPainterPath fg_path,bg_path, *current_path;
    QImage image;
    ImageCutOut *cutout;

private slots:
    void on_actionOpen_triggered();

    void on_actionSegshow_triggered();

    void on_actionClean_triggered();

    void on_actionTestseed_triggered();


    void on_actionLoadurl_triggered();

private:
    Ui::MainWindow *ui;
    CutOutSettings setting;
    int show_seg;
};

#endif // MAINWINDOW_H
