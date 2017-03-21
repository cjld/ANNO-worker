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
