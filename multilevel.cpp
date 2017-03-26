#include "multilevel.h"
#include "graphCut/graph.h"
#include <iostream>
#include <QDebug>
#include <QtNetwork/QtNetwork>
#include <QPainter>

#define FG_COMPONENTS 1
#define BG_COMPONENTS 8
#define LOCAL_EXPAND 5
#define PROP_MULTIPLY 10 //32
#define EDGE_POW 1.1
#define EDGE_MULTIPLY 60.0
#define COLOR_DIV 255.0f
#define EDGE_TH 5
void tictoc(int ts, string msg);

Vec3d color2vec(unsigned int a) {
    int a1 = (a & 0xff0000) >> 16;
    int a2 = (a & 0x00ff00) >> 8;
    int a3 = a & 0x0000ff;
    return Vec3d(a1,a2,a3) / COLOR_DIV;
}

double colorDis(unsigned int a, unsigned int b) {
    int a1 = (a & 0xff0000) >> 16;
    int a2 = (a & 0x00ff00) >> 8;
    int a3 = a & 0x0000ff;
    int b1 = (b & 0xff0000) >> 16;
    int b2 = (b & 0x00ff00) >> 8;
    int b3 = b & 0x0000ff;
    return sqr(a1-b1) + sqr(a2-b2) + sqr(a3-b3);
}


Multilevel::Multilevel()
{
    min_size = 200;
    th = EDGE_TH;
    th_low = 255*th/100;
    th_high = 255-th_low;
}

void Multilevel::set_image(MyImage img) {
    imgs.clear();
    imgs.push_back(img);
    while ((int)imgs[imgs.size()-1].buffer.size() > sqr(min_size)) {
        imgs.push_back(down_sample(imgs[imgs.size()-1]));
    }
}

void Multilevel::set_selection(MyImage selection) {
    selections.clear();
    selections.push_back(selection);
    while ((int)selections[selections.size()-1].buffer.size() > sqr(min_size)) {
        selections.push_back(down_sample(selections[selections.size()-1]));
    }

}


void Multilevel::update_seed(vector<pair<int,int>> seeds, CmGMM3D &fgGMM, double max_prop, vector<pair<int,int>> &bseeds) {
    int dx[] = {1,0,-1,0};
    int dy[] = {0,1,0,-1};

    minx = 0, maxx = imgs[imgs.size()-1].w;
    miny = 0, maxy = imgs[imgs.size()-1].h;
    for (int i=imgs.size()-1; i>=0; i--) {
        tictoc(-(i*5+5)-0, "build graph");
        MyImage &img = imgs[i];
        MyImage &sel = selections[i];
        MyImage seedimg(img.w, img.h);
        int pow3 = (int)pow(3, i);
        for (auto x : seeds) {
            seedimg.get(x.first/pow3, x.second/pow3) = 1;
        }
        for (auto x : bseeds) {
            seedimg.get(x.first/pow3, x.second/pow3) |= 2;
        }
        int n = img.w*img.h;
        Graph<double,double,double> g(n, n*4);
        g.add_node(n);
        int off = 1;
        if (i == imgs.size()-1)
            for (int y=max(miny-off,0); y<min(maxy+off,img.h); y++)
                for (int x=max(minx-off,0); x<min(maxx+off,img.w); x++) {
                    sel.get(x,y) |= 128<<16;
                }
        for (int y=max(miny-off,0); y<min(maxy+off,img.h); y++)
            for (int x=max(minx-off,0); x<min(maxx+off,img.w); x++) {
                int lowc = (sel.get(x,y)>>16)&255;
                if (lowc>=th_low && lowc<=th_high) {
                    sel.get(x,y) |= 1<<24;
                    for (int d=0; d<4; d++) {
                        int xx = dx[d]+x;
                        int yy = dy[d]+y;
                        if (xx<0 || xx>=img.w || yy<0 || yy>=img.h) continue;
                        sel.get(xx,yy) |= 1<<24;
                    }
                }
            }
        for (int y=max(miny-off,0); y<min(maxy+off,img.h); y++)
            for (int x=max(minx-off,0); x<min(maxx+off,img.w); x++) {
                int lowc = (sel.get(x,y)>>16)&255;
                if (!(sel.get(x,y) >> 24)) continue;

                int id1 = x+y*img.w;
                int c1 = img.get(x,y);
                double fProp = fgGMM.P(color2vec(c1));
                fProp = -log(fProp / max_prop) * PROP_MULTIPLY * pow3 * pow3; //27
                bool is_seed = false;
                if ((seedimg.get(x,y)&1) || lowc > th_high) {
                    g.add_tweights(id1, DBL_MAX, 0);
                    is_seed = true;
                }
                bool is_fg = (sel.get(x,y)&255)>127;
                bool is_bg = ((sel.get(x,y)>>8)&255)>127;
                if (!is_bg && !is_fg && fProp>0 && !is_seed)
                    g.add_tweights(id1, 0, fProp);
                if ((lowc < th_low || (seedimg.get(x,y)>>1) || is_bg) && !is_seed)
                    g.add_tweights(id1, 0, DBL_MAX);
                // TODO: optimise
                //if (sel.get(x,y) && !is_seed) continue;
                for (int d=0; d<2; d++) {
                    int xx = dx[d]+x;
                    int yy = dy[d]+y;
                    if (xx<0 || xx>=img.w || yy<0 || yy>=img.h) continue;
                    int id2 = xx+yy*img.w;
                    int c2 = img.get(xx,yy);
                    double dis = colorDis(c1,c2);
                    //dis = exp(-dis/(255*255))*this->k*300;
                    dis = 1 / (pow(dis/255./255., EDGE_POW)+0.001) * EDGE_MULTIPLY * pow3;// * pow(this->k/30.0,3);
                    //dis=0;
                    // TODO: normalize it
                    g.add_edge(id1, id2, dis, dis);
                }
            }
        tictoc(-(i*5+5)-0, "build graph");
        tictoc(-(i*5+5)-1, "maxflow");
        double flow = g.maxflow();
        cerr << "maxflow " << flow << endl;
        tictoc(-(i*5+5)-1, "maxflow");
        tictoc(-(i*5+5)-2, "upsampleing");
        for (int y=miny; y<maxy; y++)
            for (int x=minx; x<maxx; x++) {
                int id1 = x+y*img.w;
                // 4 byte, abcd, c means is not passed, d means pre trimap
                int lowd = (sel.get(x,y)>>16)&255;
                bool ispass = !(sel.get(x,y) >> 24);
                sel.get(x,y) &= (1<<16)-1;
                if (ispass) {
                    if (lowd < th_low)
                        {}
                    else {
                        sel.get(x,y) |= 255<<16;
                    }
                    continue;
                }
                if (g.what_segment(id1) == Graph<double,double,double>::SOURCE) {
                    sel.get(x,y) |= 255<<16;
                }
            }
        next_minx = 1<<30, next_maxx = 0;
        next_miny = 1<<30, next_maxy = 0;
        if (i == imgs.size()-1) {
            for (int y=miny; y<maxy; y++)
                for (int x=minx; x<maxx; x++) {
                    bool is_fg = (sel.get(x,y)&255)>127;
                    bool is_bg = ((sel.get(x,y)>>8)&255)>127;
                    bool is_seed = seedimg.get(x,y)&1;
                    if (is_seed || (((sel.get(x,y)>>16) & 255) && !is_fg && !is_bg)) {
                        next_minx = min(next_minx, x);
                        next_miny = min(next_miny, y);
                        next_maxx = max(next_maxx, x);
                        next_maxy = max(next_maxy, y);
                    }
                }
            next_maxx++;
            next_maxy++;
        } else {
            next_minx = minx;
            next_maxx = maxx;
            next_miny = miny;
            next_maxy = maxy;
        }
        if (i) {
            int dd = 2;
            minx = max(0,(next_minx-dd));
            maxx = min(img.w, (next_maxx+dd));
            miny = max(0,(next_miny-dd));
            maxy = min(img.h, (next_maxy+dd));

            next_minx = max(0,minx*3);
            next_maxx = min(imgs[i-1].w, maxx*3);
            next_miny = max(0, miny*3);
            next_maxy = min(imgs[i-1].h, maxy*3);

            // 4 byte, abcd, c means pre trimap, d means graphcut result
            up_sample_fast(sel, imgs[i-1], selections[i-1]);

            minx = next_minx;
            maxx = next_maxx;
            miny = next_miny;
            maxy = next_maxy;

        } else {
            for (auto &x : sel.buffer) x &= (1<<24)-1;
        }
        tictoc(-(i*5+5)-2, "upsampleing");
    }
}

MyImage Multilevel::down_sample(MyImage &img) {
    int w = (img.w+2)/3;
    int h = (img.h+2)/3;
    MyImage img2(w,h);
    for (int y=0; y<h; y++) {
        for (int x=0; x<w; x++) {
            int r=0,g=0,b=0;
            for (int dy=0;dy<=2;dy++)
                for (int dx=0;dx<=2;dx++) {
                    int yy = min(img.h-1,dy+y*3);
                    int xx = min(img.w-1,dx+x*3);
                    unsigned int c = img.get(xx,yy);
                    r += (c>>16)&255;
                    g += (c>>8)&255;
                    b += (c)&255;
                }
            r /= 9;
            g /= 9;
            b /= 9;
            img2.get(x,y) = (r<<16) | (g<<8) | (b);
        }
    }
    return img2;
}

#define RGB
#undef RGB


void Multilevel::dilute(MyImage &low, int size) {
    for (int y=0; y<low.h; y++) {
        for (int x=0; x<low.w; x++) {
            auto &c1 = low.get(x,y);
            c1 |= (c1&3^1)<<16;
        }
    }
    for (int i=0; i<size; i++) {
        // dilute x
        for (int y=0; y<low.h; y++) {
            for (int x=0; x<low.w-1; x++) {
                auto &c1 = low.get(x,y);
                auto &c2 = low.get(x+1,y);
                auto c3 = ((c1 | c2) & (1 << 16)) << 2;
                auto c4 = ((c1 | c2) & (1 << 17)) << 2;
                c1 |= c3;
                c2 |= c3;
                c1 |= c4;
                c2 |= c4;
            }
        }
        // dilute y
        for (int y=0; y<low.h-1; y++) {
            for (int x=0; x<low.w; x++) {
                auto &c1 = low.get(x,y);
                auto &c2 = low.get(x,y+1);
                auto c3 = ((c1 | c2) & (1 << 18)) << 2;
                auto c4 = ((c1 | c2) & (1 << 19)) << 2;
                c1 |= c3;
                c2 |= c3;
                c1 |= c4;
                c2 |= c4;
            }
        }
        for (int y=0; y<low.h; y++) {
            for (int x=0; x<low.w; x++) {
                auto &c1 = low.get(x,y);
                int mask = ((1 << 16)-1);
                c1 = (c1 & mask) | ((c1 >> 4) & ~mask);
            }
        }
    }

}



void Multilevel::dilute_fast(MyImage &low, int size) {
    // bit 0(is 0) bit 1(is 1)
    for (int y=miny; y<maxy; y++) {
        for (int x=minx; x<maxx; x++) {
            auto &c1 = low.get(x,y);
            c1 |= ((c1>>16)&3^1)<<24;
        }
    }
    for (int i=0; i<size; i++) {
        // dilute x
        for (int y=miny; y<maxy; y++) {
            for (int x=minx; x<maxx-1; x++) {
                auto &c1 = low.get(x,y);
                auto &c2 = low.get(x+1,y);
                auto c3 = ((c1 | c2) & (1 << 24)) << 2;
                auto c4 = ((c1 | c2) & (1 << 25)) << 2;
                c1 |= c3;
                c2 |= c3;
                c1 |= c4;
                c2 |= c4;
            }
        }
        // dilute y
        for (int y=miny; y<maxy-1; y++) {
            for (int x=minx; x<maxx; x++) {
                auto &c1 = low.get(x,y);
                auto &c2 = low.get(x,y+1);
                auto c3 = ((c1 | c2) & (1 << 26)) << 2;
                auto c4 = ((c1 | c2) & (1 << 27)) << 2;
                c1 |= c3;
                c2 |= c3;
                c1 |= c4;
                c2 |= c4;
            }
        }
        for (int y=miny; y<maxy; y++) {
            for (int x=minx; x<maxx; x++) {
                auto &c1 = low.get(x,y);
                int mask = ((1 << 24)-1);
                c1 = (c1 & mask) | ((c1 >> 4) & ~mask);
            }
        }
    }

}

void Multilevel::up_sample_fast(MyImage &low, MyImage &high, MyImage &store) {
    dilute_fast(low, 2);

    for (int y=next_miny; y<next_maxy; y++) {
        for (int x=next_minx; x<next_maxx; x++) {
            float lowxf = x/3.0, lowyf = y/3.0;
            int lowx = x/3, lowy = y/3;
            #ifdef RGB
            float rs=0,gs=0;
            #endif
            float bs=0,k=0;
            unsigned int c = high.get(x, y);
            float r = ((c>>16)&255)/255.;
            float g = ((c>>8)&255)/255.;
            float b = ((c)&255)/255.;

            /*
            int pre_color = (low.get(lowx,lowy) >> 8) & 255;
            if (pre_color < th_low || pre_color > th_high) {
                img.get(x,y) = pre_color;
                cc2++;
                pass = true;
            }
            */
            int tc = low.get(lowx, lowy) >> 24;
            if (tc == 1 || tc == 2) {
                store.get(x,y) = low.get(lowx,lowy)&(255<<16) | (store.get(x,y) & ((1<<16)-1));
                continue;
            }

            for (int dy=-2;dy<=2;dy++)
                for (int dx=-2;dx<=2;dx++) {
                    //int lowyy = min(maxy-1, max(miny, dy+lowy));
                    //int lowxx = min(maxx-1, max(minx, dx+lowx));
                    //int highyy = min(next_maxy-1, max(next_miny, lowyy*3+1));
                    //int highxx = min(next_maxx-1, max(next_minx, lowxx*3+1));
                    int lowyy = min(low.h-1, max(0, dy+lowy));
                    int lowxx = min(low.w-1, max(0, dx+lowx));
                    int highyy = min(high.h-1, max(0, lowyy*3+1));
                    int highxx = min(high.w-1, max(0, lowxx*3+1));

                    unsigned int highc = high.get(highxx, highyy);
                    float rr = ((highc>>16)&255)/255.;
                    float gg = ((highc>>8)&255)/255.;
                    float bb = ((highc)&255)/255.;

                    unsigned int lowc = low.get(lowxx, lowyy);
                    float lbb = ((lowc>>16)&255)/255.;
                    float cdis = sqr(bb-b);
                    cdis += sqr(rr-r)+sqr(gg-g);
                    float rdis = sqr(lowxf-lowxx) + sqr(lowyf-lowyy);
                    float normc = 0.1;
                    float normr = 0.5;

                    float w = exp(-rdis/2/normr - cdis/2/normc);
                    bs += lbb*w;
                    k += w;
                }
            bs /= k;
            int ib = min(255,max(0,(int)round(bs*255)));
            /*
            if (tc == 1 || tc == 2) {
                if (ib != (store.get(x,y)>>16)) {
                    cerr << "impossible " << ib << ' ' << (store.get(x,y)>>16) << ' '
                         << ((store.get(x,y)>>8)&255) << ' '
                         << ((store.get(x,y))&255) << ' ' << tc << endl;
                }
            }
            */
            store.get(x,y) = (ib<<16)|(store.get(x,y)&((1<<16)-1));
        }
    }
}

MyImage Multilevel::up_sample(MyImage &low, MyImage &high) {
    MyImage img(high.w, high.h);
    dilute(low, 2);

    for (int y=0; y<img.h; y++) {
        for (int x=0; x<img.w; x++) {
            float lowxf = x/3.0, lowyf = y/3.0;
            int lowx = x/3, lowy = y/3;
            #ifdef RGB
            float rs=0,gs=0;
            #endif
            float bs=0,k=0;
            unsigned int c = high.get(x, y);
            float r = ((c>>16)&255)/255.;
            float g = ((c>>8)&255)/255.;
            float b = ((c)&255)/255.;

            int tc = low.get(lowx, lowy) >> 16;
            if (tc == 1 || tc == 2) {
                img.get(x,y) = low.get(lowx,lowy)&255;
                continue;
            }

            for (int dy=-2;dy<=2;dy++)
                for (int dx=-2;dx<=2;dx++) {
                    int lowyy = min(low.h-1, max(0, dy+lowy));
                    int lowxx = min(low.w-1, max(0, dx+lowx));
                    int highyy = min(high.h-1, max(0, lowyy*3+1));
                    int highxx = min(high.w-1, max(0, lowxx*3+1));

                    unsigned int highc = high.get(highxx, highyy);
                    float rr = ((highc>>16)&255)/255.;
                    float gg = ((highc>>8)&255)/255.;
                    float bb = ((highc)&255)/255.;

                    unsigned int lowc = low.get(lowxx, lowyy);
                    #ifdef RGB
                    float lrr = ((lowc>>16)&255)/255.;
                    float lgg = ((lowc>>8)&255)/255.;
                    #endif
                    float lbb = ((lowc)&255)/255.;
                    float cdis = sqr(bb-b);
                    cdis += sqr(rr-r)+sqr(gg-g);
                    float rdis = sqr(lowxf-lowxx) + sqr(lowyf-lowyy);
                    float normc = 0.1;
                    float normr = 0.5;

                    float w = exp(-rdis/2/normr - cdis/2/normc);
                    #ifdef RGB
                    rs += lrr*w;
                    gs += lgg*w;
                    #endif
                    bs += lbb*w;
                    k += w;
                }
            #ifdef RGB
            rs /= k;
            gs /= k;
            #endif
            bs /= k;
            #ifdef RGB
            int ir = min(255,max(0,(int)round(rs*255)));
            int ig = min(255,max(0,(int)round(gs*255)));
            #endif
            int ib = min(255,max(0,(int)round(bs*255)));
            img.get(x,y) =
                    #ifdef RGB
                    (ir<<16) | (ig<<8) |
                    #endif
                    ib;
        }
    }
    return img;
}



void Multilevel::print(MyImage &a) {
    for (int y=0;y<a.h;y++) {
        for (int x=0;x<a.w;x++) {
            cout << a.get(x,y) << "\t";
        }
        cout << endl;
    }
    for (int x=0;x<a.w;x++) {
        cout << "=";
    }
    cout << endl;
}

void Multilevel::test() {
    MyImage a(15,15);
    for (int y=0; y<15; y++) {
        for (int x=0; x<15; x++) {
            if (x<9)
                a.get(x,y) = 200;
            else if (y<9)
                a.get(x,y) = 201;
            else
                a.get(x,y) = 0;
        }
    }
    print(a);
    MyImage b = down_sample(a);
    print(b);
    MyImage c = down_sample(b);
    print(c);

    c.get(0,0) = c.get(0,1) = 0;
    print(c);
    MyImage bb = up_sample(c,b);
    print(bb);
    MyImage aa = up_sample(bb,a);
    print(aa);
}


void MultilevelController::updateSeed() {
    //seedAPGraphcut();
    //seedGraphcut();
    seedMultiGraphcut();
}

MultilevelController::MultilevelController(): QObject(),  fgGMM(FG_COMPONENTS), bgGMM(BG_COMPONENTS) {
    current_stroke_id = 0;
    xmin = ymin = 0;
    xmax = ymax = 40;
    brush_size = 5;
    is_delete = false;
    output_lock = 0;
    worker = std::make_unique<std::thread>([&] {
        while (1) {
            unique_lock<mutex> lk(seed_lock);
            seed_cv.wait(lk, [&]{return seed.size() != 0;});
            is_changed = true;
            seed_copy = seed;
            seed.clear();
            lk.unlock();
            {
                lock_guard<mutex> lg(selection_lock);
                tictoc(-1, "updateSeed");
                updateSeed();
                tictoc(-1, "updateSeed");
            }
            if (output_lock) {
                tictoc(res_header["data"]["ts"].as<int>(), "graphcut");
                printContours(res_header);
            }
        }
    });
}
void MultilevelController::checkSeed(int x, int y) {
    if ((bool)selection.get(x,y) == (bool)is_delete)
        seed.push_back(make_pair(x,y));
    unsigned int &c = stroke_id.get(x,y);
    if (c && ((c^is_delete)&1)) {
        unsigned int sid = c>>1;
        auto &v = stroke_pixels[sid];
        for (auto xy : v) {
            auto &id = stroke_id.get(xy.first, xy.second);
            if ((id>>1) == sid) {
                id = 0;
                draw_mask.get(xy.first, xy.second) = 0;
            }
        }
        stroke_pixels.erase(sid);
    }
    if (!c) {
        c = (current_stroke_id<<1)|is_delete;
        current_pixel->push_back(make_pair(x,y));
    }
}
void MultilevelController::drawCircle(MyImage &image, int x, int y, int r, int c) {
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
        for (int xx=rrx; xx<llx; xx++) {
            image.get(xx,yy) = c;
            checkSeed(xx,yy);
        }
    }
}

void MultilevelController::drawLine(MyImage &image, int x1, int y1, int x2, int y2, int w, int c) {
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

        for (int xx=ceil(xa); xx<xb; xx++) {
            image.get(xx,yy) = c;
            checkSeed(xx,yy);
        }
    }
}

void MultilevelController::draw(QPoint s, QPoint t) {
    //seed.clear();
    lock_guard<mutex> lock(seed_lock);
    //is_delete = is_shift_press;
    int c = 0x50cc0000;
    if (!is_delete) c = 0xaa00cc00;
    drawCircle(draw_mask, t.x(), t.y(), brush_size, c);
    drawLine(draw_mask, s.x(), s.y(), t.x(), t.y(), brush_size, c);
    xmin = t.x(), xmax = s.x();
    if (xmin>xmax) swap(xmin,xmax);
    ymin = t.y(), ymax = s.y();
    if (ymin>ymax) swap(ymin,ymax);
    xmin -= brush_size * LOCAL_EXPAND;
    xmax += brush_size * LOCAL_EXPAND;
    ymin -= brush_size * LOCAL_EXPAND;
    ymax += brush_size * LOCAL_EXPAND;
    xmin = max(xmin, 0);
    ymin = max(ymin, 0);
    xmax = min(selection.w, xmax);
    ymax = min(selection.h, ymax);
    seed_cv.notify_one();
    //updateSeed();
}


void MultilevelController::seedMultiGraphcut() {
    tictoc(-2,"build gmm1");
    int dx[4] = {0,1,0,-1};
    int dy[4] = {1,0,-1,0};
    vector< pair<int,int> > &seed = seed_copy;
    if (seed.size() == 0) return;
    int color = 0x000000ff;
    fgGMM.Reset();
    vector<double> vecWeight;
    vector<Vec3d> Seeds;
    Mat compli;

#define check_selection(selection, x, y) (bool)(selection.get(x,y)&color) == (bool)is_delete

    for (int x=xmin; x<xmax; x++) {
        for (int y=ymin; y<ymax; y++) {
            if (x<0 || x>=selection.w || y<0 || y>=selection.h) continue;
            bool is_edge = false;
            if (check_selection(selection, x, y)) continue;
            for (int i=0; i<4; i++) {
                int xx = dx[i]+x;
                int yy = dy[i]+y;
                if (xx<0 || xx>=selection.w || yy<0 || yy>=selection.h) continue;
                if (check_selection(selection, xx, yy))  {
                    is_edge = true;
                    break;
                }
            }
            if (is_edge)
                seed.push_back({x,y});
        }
    }

    for (int i=0; i<(int)seed.size(); i++) {
        int x = seed[i].first;
        int y = seed[i].second;
        if (is_delete)
            // clean last byte
            selection.get(x,y) &= ~color;
        else
            selection.get(x,y) |= color;
        vecWeight.push_back(1.0/seed.size());
        Seeds.push_back(color2vec(image.get(x,y)));
    }
    tictoc(-2,"build gmm1");
    tictoc(-2,"build gmm2");

    Mat matWeightFg(1, Seeds.size(), CV_64FC1, &vecWeight[0]);
    Mat SeedSamples(1, Seeds.size(), CV_64FC3, &Seeds[0]);
    fgGMM.BuildGMMs(SeedSamples, compli, matWeightFg);
    fgGMM.RefineGMMs(SeedSamples, compli, matWeightFg);
    tictoc(-2,"build gmm2");
    tictoc(-2,"build gmm3");

    double max_prop = 0;

    for (auto &xy : seed) {
        int c1 = image.get(xy.first, xy.second);
        double fProp = fgGMM.P(color2vec(c1));
        max_prop = max(fProp, max_prop);
    }

    tictoc(-2, "build gmm3");
    tictoc(-3, "build multilevel");
    if (is_delete) {
        MyImage selection_reverse = selection;
        for (auto &c : selection_reverse.buffer) c ^= color;
        mt.set_selection(selection_reverse);
    } else
        mt.set_selection(selection);
    vector<pair<int,int>> bseeds;
    for (auto &kv : stroke_pixels)
        if (stroke_isbg[kv.first] ^ is_delete)
            bseeds.insert(bseeds.end(), kv.second.begin(), kv.second.end());
    tictoc(-3, "build multilevel");
    tictoc(-4, "update seed");
    mt.update_seed(seed, fgGMM, max_prop, bseeds);
    MyImage &sel = mt.selections[0];
    for (int y=0; y<sel.h; y++)
        for (int x=0; x<sel.w; x++)
            if (((sel.get(x,y)>>16)&255) > 255*0.75) {
                if (is_delete)
                    selection.get(x,y) &= ~color;
                else
                    selection.get(x,y) |= color;
            }
    tictoc(-4, "update seed");
    emit selection_changed();
}

void MultilevelController::setImage(QImage img) {
    img = img.convertToFormat(QImage::Format_RGB32);
    draw_mask.resize(img.width(), img.height());
    selection.resize(img.width(), img.height());
    stroke_id.resize(img.width(), img.height());
    image.resize(img.width(), img.height());
    memcpy(&image.buffer[0], img.bits(), 4*img.width()*img.height());
    seed.clear();
    mt.set_image(image);
}

void findContours(MyImage &image, vector<vector<pair<int,int>>> &contours) {
    int dx[] = {1,0,-1,0};
    int dy[] = {0,1,0,-1};
    int rdx[] = {0,-1,-1,0};
    int rdy[] = {0,0,-1,-1};
    MyImage o(image.w, image.h);
    for (int y=0; y<image.h; y++) {
        for (int x=0; x<image.w; x++) {
            auto c1 = image.getlastb(x,y);
            auto c2 = 0;
            if (y) c2 = image.getlastb(x,y-1);
            if (!o.get(x,y) && c1 && !c2) {
                int sx = x, sy = y, d=0, pred=-1;
                vector<pair<int,int>> v;
                while (!(d==0 && o.get(sx,sy))) {
                    if (!d) o.get(sx,sy) = 1;
                    if (pred != d) {
                        v.push_back(make_pair(sx,sy));
                        pred = d;
                    }
                    int fc1 = 0, fc2 = 0;
                    int rx = sx+rdx[d];
                    int ry = sy+rdy[d];
                    int fx1 = rx+dx[d];
                    int fy1 = ry+dy[d];
                    int fx2 = fx1+dx[(d+3)&3];
                    int fy2 = fy1+dy[(d+3)&3];
                    if (fx1>=0 && fx1<image.w && fy1>=0 && fy1<image.h)
                        fc1 = image.getlastb(fx1, fy1);
                    if (fx2>=0 && fx2<image.w && fy2>=0 && fy2<image.h)
                        fc2 = image.getlastb(fx2, fy2);
                    sx += dx[d];
                    sy += dy[d];
                    if (!fc1)
                        d = (d+1)&3;
                    else if (fc2)
                        d = (d+3)&3;
                }
                contours.push_back(v);
            }
        }
    }
}

void MultilevelController::printContours(json header) {
    if (is_changed) {
        vector<vector<pair<int,int>> > contours;
        findContours(selection, contours);

        json arr1 = json::array();
        for (int i=0; i<(int)contours.size(); i++) {
            json arr2 = json::array();
            for (int j=0; j<(int)contours[i].size(); j++) {
                json point;
                point["x"] = contours[i][j].first;
                point["y"] = contours[i][j].second;
                arr2.add(point);
            }
            arr1.add(arr2);
        }
        header["data"].set("contours", arr1);
        is_changed = false;
        output_lock->lock();
        cout << header << endl;
        cout.flush();
        output_lock->unlock();
    }
}

void MultilevelController::new_stroke() {
    current_stroke_id++;
    stroke_isbg[current_stroke_id] = is_delete;
    stroke_pixels[current_stroke_id] = {};
    current_pixel = &stroke_pixels[current_stroke_id];
}

json MultilevelController::load_url(string url) {
    QNetworkAccessManager manager;

    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(url.c_str())));

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &loop, SLOT(quit()));
    loop.exec();

    QByteArray bts = reply->readAll();
    QImage image;
    image.loadFromData(bts);

    delete reply;

    qDebug() << image.size();
    if (image.size().width() == 0) throw "image-load-failed";
    setImage(image);
    json res;
    res.set("regCount", this->image.buffer.size());
    return res;
}

json MultilevelController::load_base64(string data) {
    QByteArray bts = QByteArray::fromBase64(data.c_str(), QByteArray::Base64Encoding);
    QImage image;
    image.loadFromData(bts);

    qDebug() << image.size();
    if (image.size().width() == 0) throw "image-load-failed";
    setImage(image);
    json res;
    res.set("regCount", this->image.buffer.size());
    return res;
}

json MultilevelController::paint(json data) {
    int size = data["size"].as<int>();
    // to config
    if (size < 1) size=1;
    if (size > 200) size=200;
    json stroke = data["stroke"];
    bool is_bg = data["is_bg"].as<bool>();
    is_delete = is_bg;
    int n = stroke.size();
    QPoint last(stroke[0]["x"].as<double>(), stroke[0]["y"].as<double>());
    brush_size = size;
    for (int i=1; i<n; i++) {
        QPoint cur(stroke[i]["x"].as<double>(), stroke[i]["y"].as<double>());
        draw(last, cur);
        last = cur;
    }
    if (n==1) {
        this->new_stroke();
        draw(last, last);
    }
    //cerr << "brush size" << brush_size << endl;
    json res;
    res.set("async", true);
    return res;
}

json MultilevelController::load_region(json data) {
    lock_guard<mutex> lg(selection_lock);
    {
        lock_guard<mutex> lg2(seed_lock);
        seed.clear();
    }
    for (auto &kv : stroke_pixels)
        for (auto &xy : kv.second) {
            stroke_id.get(xy.first, xy.second) = 0;
        }
    stroke_isbg.clear();
    stroke_pixels.clear();
    json contours_json = data["contours"];
    int segs=0;
    auto draw_selection = [&](json contours_json, unsigned int color) {
        vector< vector<Point> > contours;
        contours.resize(contours_json.size());
        for (int i=0; i<(int)contours.size(); i++) {
            json tmp = contours_json[i];
            for (int j=0; j<(int)tmp.size(); j++)
                contours[i].push_back(Point(tmp[j]["x"].as<double>(),tmp[j]["y"].as<double>()));
        }
        QPainterPath cutSelectPath = QPainterPath();
        for (int i = 0; i < (int)contours.size(); i++) {
            cutSelectPath.moveTo(QPoint(contours[i][0].x, contours[i][0].y));
            for (vector<Point>::iterator it = contours[i].begin(); it != contours[i].end(); it++)
                cutSelectPath.lineTo(QPoint((*it).x, (*it).y));
            cutSelectPath.lineTo(QPoint(contours[i][0].x, contours[i][0].y));
        }
        QImage qStrokeMask = QImage(image.w, image.h, QImage::Format_ARGB32_Premultiplied);
        qStrokeMask.fill(0);
        QPainter pt;
        pt.begin(&qStrokeMask);
        pt.setBrush(Qt::white);
        pt.setPen(Qt::NoPen);
        pt.drawPath(cutSelectPath);
        for (int y=0; y<image.h; y++) {
            QRgb* scanelineStrokeMask = (QRgb*)qStrokeMask.scanLine(y);
            for (int x=0; x<image.w; x++) {
                int grayValue = qGray(scanelineStrokeMask[x]);
                draw_mask.get(x,y) = 0;
                if (grayValue>100) {
                    selection.get(x,y) |= color;
                    segs ++;
                }
            }
        }
    };
    for (int y=0; y<image.h; y++)
        for (int x=0; x<image.w; x++)
            selection.get(x,y) = 0;
    draw_selection(contours_json, 255);
    if (data.has_member("bgContours"))
        draw_selection(data["bgContours"], 255<<8);
    //pt.drawRect(0,0,100,100);
    //cutout->qStrokeMask.save("/tmp/check-region.png");
    json res = json::object();
    res.set("segCount", segs);
    return res;
}
