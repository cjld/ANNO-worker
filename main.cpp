#include "mywindow.h"
#include <QApplication>
// json library from https://github.com/danielaparker/jsoncons
#include "jsoncons/json.hpp"
#include "config.h"
#include <iostream>
#include <ctime>
#include <QElapsedTimer>
#include <mutex>
using jsoncons::json;

#include <QtNetwork/QtNetwork>
#include "multilevel.h"


std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != NULL)
            result += buffer.data();
    }
    return result;
}

void tictoc(int ts, string msg="") {
    if (!Config::timeEvaluate) return;
    static map<int, QElapsedTimer> ct;
    if (ct.find(ts) != ct.end()) {
        cerr << ts << ": " << ct[ts].elapsed() << "ms " << msg << endl;
        ct.erase(ts);
    } else {
        ct[ts] = QElapsedTimer();
        ct[ts].start();
    }
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
        while (getline(cin, line)) {
            bool async = false;
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
                    tictoc(ts);
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
                if (cmdstr == "open-base64") {
                    string data = cmd["data"].as<string>();
                    //res["data"].set("url", url);
                    res["data"].set("return", ctl.load_base64(data));
                } else
                if (cmdstr == "config") {
                    json data = cmd["data"];
                    Config::load(data);
                    //res["data"].set("url", url);
                    //res["data"].set("return", ctl.load_base64(data));
                } else
                if (cmdstr == "paint") {
                    json data = ctl.paint(cmd["data"]);
                    if (data.has_member("async") && data["async"] == true)
                        async = true;
                    //res["data"].set("contours", data);
                } else
                if (cmdstr == "load-region") {
                    json data = ctl.load_region(cmd["data"]);
                    res["data"].set("return", data);
                } else {
                    res["status"] = "error";
                    res["error"] = string("command '")+cmdstr+("' not found");
                }
                //cerr << cmdstr << ' ' << res.to_string() <<  endl;
            } catch (const exception &e) {
                res["status"] = "error";
                res["error"] = (string("Error occur: ") + e.what()).c_str();
                cerr << res["error"] << endl;
            }
            //cerr << res.to_string().size() << ' ' << ts <<  endl;
            if (!async || res["status"] == "error") {
                output_lock.lock();
                cout << res << endl;
                cout.flush();
                output_lock.unlock();
            }
        }
    } else {
        MyWindow w;
        w.show();

        return a.exec();
    }
}
