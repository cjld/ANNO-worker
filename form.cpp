#include "form.h"
#include "ui_form.h"
#include <QPainter>
#include <mywindow.h>
#include <QDebug>

Form::Form(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form)
{
    window = 0;
    ui->setupUi(this);
}

Form::~Form()
{
    delete ui;
}

//template<class T> T sqr(T a) {return a*a;}

bool colorDistance(unsigned int a, unsigned int b, double cdis) {
    int a1 = (a & 0xff0000) >> 16;
    int a2 = (a & 0x00ff00) >> 8;
    int a3 = a & 0x0000ff;
    int b1 = (b & 0xff0000) >> 16;
    int b2 = (b & 0x00ff00) >> 8;
    int b3 = b & 0x0000ff;
    return sqr(a1-b1) + sqr(a2-b2) + sqr(a3-b3) <= cdis*cdis;
}
//bool

void Form::paintEvent(QPaintEvent *) {
    if (!window) return;
    QRectF rectangle(10.0, 20.0, 80.0, 60.0);
    QPainter painter(this);
    painter.drawRect(rectangle);
    QBrush b;

    int bin = 10, of = 20;
    painter.setPen(Qt::NoPen);

    for (int x=window->prev_pos.x()-of,xx=0; x<window->prev_pos.x()+of; x++,xx++) {
        for (int y=window->prev_pos.y()-of, yy=0; y<window->prev_pos.y()+of; y++,yy++) {
            if (x<0 || x>=window->image.w || y<0 || y>=window->image.h)
                continue;
            b.setColor(QColor(window->image.get(x,y)));
            //b.setColor(Qt::black);
            //qDebug() << b.color();
            painter.setBrush(QColor(window->image.get(x,y)));
            painter.drawRect(xx*bin,yy*bin,bin,bin);
            QColor cl;
            cl.setRgba(window->selection.get(x,y));
            painter.setBrush(cl);
            painter.drawRect(xx*bin,yy*bin,bin,bin);
        }
    }
    painter.setPen(Qt::red);
    for (int x=window->prev_pos.x()-of,xx=0; x<window->prev_pos.x()+of; x++,xx++) {
        for (int y=window->prev_pos.y()-of, yy=0; y<window->prev_pos.y()+of; y++,yy++) {

            if (x<0 || x>=window->image.w || y<0 || y>=window->image.h)
                continue;
            if (y+1 < window->image.h) {
                if (!colorDistance(window->image.get(x,y), window->image.get(x,y+1), window->k)) {
                    painter.drawLine(xx*bin,yy*bin+bin,xx*bin+bin,yy*bin+bin);
                }
            }
            if (x+1 < window->image.w) {
                if (!colorDistance(window->image.get(x,y), window->image.get(x+1,y), window->k)) {

                    painter.drawLine(xx*bin+bin,yy*bin,xx*bin+bin,yy*bin+bin);
                }
            }
        }
    }
}
