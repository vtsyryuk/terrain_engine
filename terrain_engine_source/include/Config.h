#pragma once

#include <string>

struct Config {
    int width = 900;
    int height = 900;
    double slopeThreshold = 2.0;
    int kmeansK = 2;
    int componentMinSize = 4;
    int emK = 3;

    void load(const std::string& filename);
};
