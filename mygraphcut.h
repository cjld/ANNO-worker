#ifndef MYGRAPHCUT_H
#define MYGRAPHCUT_H

#include "graphCut/graph.cpp"
#include "graphCut/maxflow.cpp"
#include <map>
#include <tuple>
#include <vector>
using namespace std;

class MyGraphCut
{
public:
    MyGraphCut();
    ~MyGraphCut() {if (g) delete g;}
    Graph<double,double,double> *g;
    void add_tweights(int,double,double);
    void add_edge(int,int,double,double);
    double maxflow();
    int what_segment(int id);
    map<int,int> imap;
    vector<tuple<int,double,double>> q1;
    vector<tuple<int,double,double,double>> q2;
    int get_id(int id) {
        auto a = imap.find(id);
        if (a != imap.end())
            return a->second;
        int t = imap.size();
        return imap[id] = t;
    }

};

#endif // MYGRAPHCUT_H
