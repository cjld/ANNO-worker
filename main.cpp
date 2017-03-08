#include "mainwindow.h"
#include "mywindow.h"
#include <QApplication>
// json library from https://github.com/danielaparker/jsoncons
#include "jsoncons/json.hpp"
#include <iostream>
#include <ctime>
#include <QElapsedTimer>
#include <mutex>
using jsoncons::json;

#include <QtNetwork/QtNetwork>
#include "multilevel.h"

struct ImageCutController : public QObject {

    QImage image;
    ImageCutOut *cutout;
    CutOutSettings setting;

    ImageCutController() {
        cutout = NULL;
    }

    json load_url(string url) {
        QNetworkAccessManager manager;

        QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url.c_str())));

        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &loop, SLOT(quit()));
        loop.exec();

        QByteArray bts = reply->readAll();
        image.loadFromData(bts);

        delete reply;

        qDebug() << image.size();
        if (image.size().width() == 0) throw "image-load-failed";
        prepare();
        json res;
        res.set("regCount", cutout->pMsSeg->regCount);
        return res;
    }

    void prepare() {
        if (cutout) {
            delete cutout;
        }
        cutout = new ImageCutOut(image);
        cutout->isImageLoad = true;
        cutout->paraSetting = &setting;
        cutout->PreSegment();
    }

    json paint(json data) {
        int size = data["size"].as<int>();
        // to config
        if (size < 1) size=1;
        if (size > 200) size=200;
        json stroke = data["stroke"];
        bool is_bg = data["is_bg"].as<bool>();
        int n = stroke.size();
        QPainterPath path;
        path.moveTo(stroke[0]["x"].as<double>(), stroke[0]["y"].as<double>());
        for (int i=1; i<n; i++) {
            path.lineTo(stroke[i]["x"].as<double>(), stroke[i]["y"].as<double>());
        }
        if (n==1) {
            path.lineTo(stroke[0]["x"].as<double>()+0.01, stroke[0]["y"].as<double>());
        }
        //cerr << "widthStroke " << size << endl;
        cutout->widthStroke = size*2;
        cutout->strokePen.setWidth(size*2);
        bool is_changed = cutout->SlotGetPainterPath(path, !is_bg);
        //cutout->qStrokeMask.save("/tmp/haha.png");
        json res;
        if (is_changed) {
            json arr1 = json::array();
            for (int i=0; i<cutout->contours.size(); i++) {
                json arr2 = json::array();
                for (int j=0; j<cutout->contours[i].size(); j++) {
                    json point;
                    point["x"] = cutout->contours[i][j].x;
                    point["y"] = cutout->contours[i][j].y;
                    arr2.add(point);
                }
                arr1.add(arr2);
            }
            res.set("contours", arr1);
        }
        return res;
    }

    /*
     * data: { contours: [[]] }
     * */
    json load_region(json data) {
        vector< vector<Point> > contours;
        json contours_json = data["contours"];
        contours.resize(contours_json.size());
        for (int i=0; i<contours.size(); i++) {
            json tmp = contours_json[i];
            for (int j=0; j<tmp.size(); j++)
                contours[i].push_back(Point(tmp[j]["x"].as<double>(),tmp[j]["y"].as<double>()));
        }
        QPainterPath cutSelectPath = QPainterPath();
        for (int i = 0; i < contours.size(); i++) {
            cutSelectPath.moveTo(QPoint(contours[i][0].x, contours[i][0].y));
            for (vector<Point>::iterator it = contours[i].begin(); it != contours[i].end(); it++)
                cutSelectPath.lineTo(QPoint((*it).x, (*it).y));
            cutSelectPath.lineTo(QPoint(contours[i][0].x, contours[i][0].y));
        }
        cutout->qStrokeMask.fill(0);
        QPainter &pt = cutout->strokePainter;
        pt.setBrush(Qt::white);
        pt.setPen(Qt::NoPen);
        pt.drawPath(cutSelectPath);
        //pt.drawRect(0,0,100,100);
        //cutout->qStrokeMask.save("/tmp/check-region.png");
        QSet<int> rg;

        for (int i = 0; i <cutout->pMsSeg->regCount; ++i)
        {
            double s=0;
            for (Point p : cutout->pMsSeg->regCluster[i]) {
                QColor color(cutout->qStrokeMask.pixel(p.x,p.y));
                s += color.red();
            }
            if (s > 128.0 * cutout->pMsSeg->regCluster[i].size()) {
                rg.insert(i);
            }
        }
        cutout->SetFgRegion(rg);
        json res = json::object();
        res.set("segCount", rg.size());
        return res;
    }
};


void tictoc(string msg="") {
    return;
    static QElapsedTimer ct;
    static bool first = true;
    if (first) ct.start();
    else {
        cerr << ct.elapsed() << "ms " << msg << endl;
        ct.restart();
    }
    first = false;
}

int main(int argc, char *argv[])
{
    /*
     * http://stackoverflow.com/questions/17979185/qt-5-1-qapplication-without-display-qxcbconnection-could-not-connect-to-displ
     * if not going to use gui, add -platform offscreen command line option
     * */
    QApplication a(argc, argv);
    std::mutex output_lock;
    if (argc >= 2 && string(argv[1])=="server") {
        MultilevelController ctl;
        ctl.output_lock = &output_lock;
        ios_base::sync_with_stdio(false);
        json cmd;
        /* command json contain
         * cmd: command str
         * data: command arguments
         * ts: time stamp
         * */
        json res;
        /* res json contain
         * status: status of execution [ok, error]
         * error: error message if occur
         * data: data that need to return to client
         * */
        string line;
        int ts=0;
        tictoc();
        while (getline(cin, line)) {
            bool async = false;
            tictoc("wait cmd");
            cmd.clear();
            res.clear();
            cmd = json::parse(line);
            res["status"] = "ok";
            res.set("data", json::parse("{}"));
            try {
                string cmdstr = cmd["cmd"].as<string>();
                res["data"].set("pcmd", cmdstr);
                if (cmd.has_member("data") && cmd["data"].has_member("ts")) {
                    res["data"].set("ts", cmd["data"]["ts"]);
                    ts = cmd["data"]["ts"].as<int>();
                }
                ctl.res_header = res;
                if (cmdstr == "exit") {
                    cout << res;
                    cout.flush();
                    exit(0);
                } else
                if (cmdstr == "open-session") {
                    string url = cmd["data"]["url"].as<string>();
                    res["data"].set("url", url);
                    res["data"].set("return", ctl.load_url(url));
                } else
                if (cmdstr == "paint") {
                    json data = ctl.paint(cmd["data"])["contours"];
                    if (data.has_member("async") && data["async"] == true)
                        async = true;
                    res["data"].set("contours", data);
                } else
                if (cmdstr == "load-region") {
                    json data = ctl.load_region(cmd["data"]);
                    res["data"].set("return", data);
                } else {
                    res["status"] = "error";
                    res["error"] = string("command '")+cmdstr+("' not found");
                }
                //cerr << cmdstr << ' ' << res.to_string() <<  endl;
            } catch (...) {
                res["status"] = "error";
                res["error"] = "fatal error occur";
            }
            //cerr << res.to_string().size() << ' ' << ts <<  endl;
            if (!async) {
                output_lock.lock();
                cout << res << endl;
                cout.flush();
                output_lock.unlock();
            }
            tictoc("calc cmd");
        }
    } else {
        MyWindow w;
        w.show();

        return a.exec();
    }
}
