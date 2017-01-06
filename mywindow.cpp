#include "mywindow.h"
#include "ui_mywindow.h"
#include <QFileDialog>

template<class T> T sqr(T a) {return a*a;}

MyWindow::MyWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MyWindow)
{
    ui->setupUi(this);
    ui->menubar->setNativeMenuBar(false);
    draw_mask.resize(1000,1000,0xff000000);
    for (int i=0; i<draw_mask.w; i++)
        for (int j=0; j<draw_mask.h; j++)
            draw_mask.get(i,j) = 0xff000000 | (j&255);
    //on_actionOpen_file_triggered();
}

void drawCircle(MyImage &image, int x, int y, int r, int c) {
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
        for (int xx=rrx; xx<llx; xx++)
            image.get(xx,yy) = c;
    }
}

void drawLine(MyImage &image, int x1, int y1, int x2, int y2, int w, int c) {
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

        for (int xx=ceil(xa); xx<xb; xx++)
            image.get(xx,yy) = c;
    }
}

MyWindow::~MyWindow()
{
    delete ui;
}


void MyWindow::paintEvent(QPaintEvent *e) {
    QPainter painter(this);

    QImage image((uchar *)&draw_mask.get(), draw_mask.w, draw_mask.h, QImage::Format_RGB32);
    painter.drawImage(0,0,image);
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
}

void MyWindow::mouseMoveEvent(QMouseEvent *e) {
    //qDebug() << e->pos() << endl;
    current_path->lineTo(e->pos());
    drawCircle(draw_mask, e->pos().x(), e->pos().y(), 5, 0xffcc0000);
    drawLine(draw_mask, prev_pos.x(), prev_pos.y(), e->pos().x(), e->pos().y(), 5, 0xffcc0000);
    prev_pos = e->pos();
    update();
}

void MyWindow::mousePressEvent(QMouseEvent *e) {
    prev_pos = e->pos();
    drawCircle(draw_mask, e->pos().x(), e->pos().y(), 5, 0xffcc0000);
    if (e->button()==Qt::LeftButton) {
        current_path = &fg_path;
    } else {
        current_path = &bg_path;
    }
    //mousepath = QPainterPath();
    current_path->moveTo(e->pos());
    update();
}

void MyWindow::mouseReleaseEvent(QMouseEvent *) {

}

void MyWindow::on_actionOpen_file_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                 "/home",
                                                 tr("Images (*.png *.xpm *.jpg)"));
    QImage img(fileName);
    img = img.convertToFormat(QImage::Format_RGB32);
    draw_mask.resize(img.width(), img.height());
    memcpy(&draw_mask.buffer[0], img.bits(), 4*img.width()*img.height());
    this->resize(img.width(), img.height());
    update();
}
