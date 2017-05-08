#include "mygraphcut.h"
#include <iostream>

using namespace std;
MyGraphCut::MyGraphCut() :g(0) {
}


void MyGraphCut::add_tweights(int i, double fa, double fb) {
    q1.push_back(make_tuple(get_id(i), fa, fb));
}

void MyGraphCut::add_edge(int i, int j, double fa, double fb) {
    q2.push_back(make_tuple(get_id(i), get_id(j), fa, fb));
}

double MyGraphCut::maxflow() {
    cerr << "graphsize [" << imap.size() << ',' << q2.size() << "]" << endl;
    if (imap.size() == 0)
        return 0;
    g = new Graph<double,double,double>(imap.size(), q2.size());
    g->add_node(imap.size());
    for (auto v : q1)
        g->add_tweights(get<0>(v), get<1>(v), get<2>(v));
    for (auto v : q2) {
        g->add_edge(get<0>(v), get<1>(v), get<2>(v), get<3>(v));
    }
    return g->maxflow();
}

int MyGraphCut::what_segment(int id) {
    auto a = imap.find(id);
    if (a != imap.end())
        return g->what_segment(a->second);
    return Graph<double,double,double>::SOURCE;
}
