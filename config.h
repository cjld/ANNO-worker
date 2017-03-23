#ifndef CONFIG_H
#define CONFIG_H

#include "jsoncons/json.hpp"
#include <iostream>
using namespace std;

using jsoncons::json;

class Config {
public:
    static bool timeEvaluate;
    static void load(jsoncons::json json) {
        if (json.has_member("timeEvaluate")) {
            timeEvaluate = json["timeEvaluate"].as<bool>();
        }
    }
};

#endif // CONFIG_H

