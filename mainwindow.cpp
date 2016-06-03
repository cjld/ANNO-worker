#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QPainterPathStroker>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QFileDialog>
#include <QDataStream>
//#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QtNetwork>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    cutout = NULL;
    show_seg = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *e) {
    QPainter painter(this);
    QPainterPathStroker st;
    st.setWidth(10);
    st.setJoinStyle(Qt::RoundJoin);
    st.setCapStyle(Qt::RoundCap);

    QPainterPath spath = st.createStroke(fg_path);
    QPainterPath spath2 = st.createStroke(bg_path);

    if (cutout) {
        QVector<QImage*> imgs;
        imgs << &cutout->qSourceImage << &cutout->sourceSegShow
             << &cutout->qFgMask << &cutout->qStrokeMask << &cutout->qBgStrokeMask;
        int imid = show_seg % imgs.length();
        painter.drawImage(0,0,*imgs[imid]);
    } else
        painter.drawImage(0,0,image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0,255,0,100));
    painter.drawPath(spath);
    painter.setBrush(QColor(255,0,0,100));
    painter.drawPath(spath2);

    if (cutout) {
        cutout->SlotGetPainterPath(fg_path, true);
        cutout->SlotGetPainterPath(bg_path, false);
        painter.setPen(Qt::red);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(cutout->cutSelectPath);

        //painter.setPen(Qt::green);
        //painter.drawPath(cutout->innerSelectPath);
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *e) {
    //qDebug() << e->pos() << endl;
    current_path->lineTo(e->pos());
    update();
}

void MainWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button()==Qt::LeftButton) {
        current_path = &fg_path;
    } else {
        current_path = &bg_path;
    }
    //mousepath = QPainterPath();
    current_path->moveTo(e->pos());
}

void MainWindow::mouseReleaseEvent(QMouseEvent *) {

}

void MainWindow::prepare() {
    if (cutout) {
        delete cutout;
    }
    cutout = new ImageCutOut(image);
    cutout->isImageLoad = true;
    cutout->paraSetting = &setting;
    cutout->PreSegment();
    this->resize(image.size());
}

void MainWindow::on_actionOpen_triggered() {
    QString fname = QFileDialog::getOpenFileName(this, "open image","",
                                 "Images (*.png *.xpm *.jpg)");
    image = QImage(fname);
    qDebug() << image.size();
    prepare();

    //image = cutout->sourceSegShow;
    update();
}

void MainWindow::test_loadurl() {
    //QString myURL = "http://tankr.net/s/medium/KTE0.jpg";
    QString myURL = "https://www.google.com/images/branding/googlelogo/1x/googlelogo_color_272x92dp.png";
    QNetworkAccessManager manager;

    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(myURL)));

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &loop, SLOT(quit()));
    loop.exec();

    QByteArray bts = reply->readAll();
    image.loadFromData(bts);

    delete reply;

    qDebug() << image.size();
    update();
    prepare();
    update();

    //return str;
}

void MainWindow::on_actionSegshow_triggered()
{
    show_seg++;
    update();
}

QSet<int> bk;

void save_bk() {
    QFile file("/tmp/bk-regionFG");
    file.open(QIODevice::WriteOnly);
    QDataStream s(&file);
    s << bk;
}

void load_bk() {
    QFile file("/tmp/bk-regionFG");
    file.open(QIODevice::ReadOnly);
    QDataStream s(&file);
    s >> bk;
    qDebug() << "load bk size: " << bk.size();
}

void MainWindow::on_actionClean_triggered()
{
    fg_path = QPainterPath();
    bg_path = QPainterPath();
    bk = cutout->regionFg;
    save_bk();
    cutout->SetSeedRegion(cutout->regionFg, false);
    update();
}

void MainWindow::on_actionTestseed_triggered()
{
    load_bk();
    cutout->SetFgRegion(bk);
    update();
}

void MainWindow::on_actionLoadurl_triggered()
{
    test_loadurl();
}
