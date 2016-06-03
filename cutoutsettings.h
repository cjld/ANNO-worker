#ifndef CUTOUTSETTINGS_H
#define CUTOUTSETTINGS_H


class CutOutSettings
{
public:
    CutOutSettings();

    // for meanShift
    int minSize;
    double cth;
    float sigmaS;
    float sigmaR;

    // for graphcut
    double lamda;
    double beta;


    // select a video clip
    int startFrame;
    int endFrame;
    int frameNum;

    static const double myE;
    static const double gcEPSILON;

    int widthLabel;
    int heightLabel;
};

#endif // CUTOUTSETTINGS_H
