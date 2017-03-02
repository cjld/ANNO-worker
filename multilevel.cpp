#include "multilevel.h"
#include "zz/graphCut/graph.h"
#include <iostream>
#include <QDebug>

#define FG_COMPONENTS 4
#define BG_COMPONENTS 8

Multilevel::Multilevel()
{
    min_size = 200;
    th = 5;
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


MyImage Multilevel::update_seed(vector<pair<int,int>> seeds, CmGMM3D &fgGMM, double max_prop) {
    int dx[] = {1,0,-1,0};
    int dy[] = {0,1,0,-1};
    MyImage trimap;

    for (int i=imgs.size()-1; i>=0; i--) {
        qDebug() << "build graph";
        MyImage &img = imgs[i];
        MyImage &sel = selections[i];
        MyImage seedimg(img.w, img.h);
        if (i == (int)imgs.size()-1)
            trimap.resize(img.w, img.h, 128);
        int pow3 = (int)pow(3, i);
        for (auto x : seeds) {
            seedimg.get(x.first/pow3, x.second/pow3) = 255;
        }
        int n = img.w*img.h;
        Graph<double,double,double> g(n, n*4);
        g.add_node(n);
        for (int y=0; y<img.h; y++)
            for (int x=0; x<img.w; x++) {
                int lowc = trimap.get(x,y)&255;
                if (lowc>=th_low && lowc<=th_high) {
                    trimap.get(x,y) |= 1<<8;
                    for (int d=0; d<4; d++) {
                        int xx = dx[d]+x;
                        int yy = dy[d]+y;
                        if (xx<0 || xx>=img.w || yy<0 || yy>=img.h) continue;
                        trimap.get(xx,yy) |= 1<<8;
                    }
                }
            }
        for (int y=0; y<img.h; y++)
            for (int x=0; x<img.w; x++) {
                int lowc = trimap.get(x,y)&255;
                if (!(trimap.get(x,y) >> 8)) continue;

                int id1 = x+y*img.w;
                int c1 = img.get(x,y);
                double fProp = fgGMM.P(color2vec(c1));
                fProp = -log(fProp / max_prop) * 16 * pow3 * pow3; //27
                bool is_seed = false;
                if (seedimg.get(x,y) || lowc > th_high) {
                    g.add_tweights(id1, DBL_MAX, 0);
                    is_seed = true;
                }
                if (!sel.get(x,y) && fProp>0 && !is_seed)
                    g.add_tweights(id1, 0, fProp);
                if (lowc < th_low && !is_seed)
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
                    dis = 1 / (dis/255./255.+0.001) * 60 * pow3;// * pow(this->k/30.0,3);
                    //dis=0;
                    // TODO: normalize it
                    g.add_edge(id1, id2, dis, dis);
                }
            }
        qDebug() << "calc maxflow" << i;
        double flow = g.maxflow();
        qDebug() << "maxflow" << i << flow;
        for (int y=0; y<img.h; y++)
            for (int x=0; x<img.w; x++) {
                int id1 = x+y*img.w;
                // 4 byte, abcd, c means is not passed, d means pre trimap
                int lowd = trimap.get(x,y)&255;
                if (!(trimap.get(x,y) >> 8)) {
                    if (lowd < th_low)
                        trimap.get(x,y) = lowd << 8;
                    else
                        trimap.get(x,y) = (lowd << 8) | 255;
                    continue;
                }
                trimap.get(x,y) = lowd<<8;
                if (g.what_segment(id1) == Graph<double,double,double>::SOURCE) {
                    trimap.get(x,y) |= 255;
                }
            }
        if (i) {
            qDebug() << "up sampleing";
            // 4 byte, abcd, c means pre trimap, d means graphcut result
            trimap = up_sample(trimap, imgs[i-1]);
        } else {
            for (auto &x : trimap.buffer) x &= 255;
        }
    }
    return trimap;
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

MyImage Multilevel::up_sample(MyImage &low, MyImage &high) {
    MyImage img(high.w, high.h);
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

            int pre_color = low.get(lowx,lowy) >> 8;
            if (pre_color < th_low || pre_color > th_high) {
                img.get(x,y) = pre_color;
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
