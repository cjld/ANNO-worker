#include "cutoutsettings.h"


const double CutOutSettings::myE = 2.71828183;
const double CutOutSettings::gcEPSILON = 0.05;

CutOutSettings::CutOutSettings()
{
    // meanshift
    minSize = 30;
    cth = 4.0;
    sigmaS = 3.0;
    sigmaR = 3.0;

    //lab
    minSize = 40;
    sigmaS = 2.0;
    sigmaR = 1.0;

    //graphcut
    beta = 0.1;
    lamda = 100;

}
