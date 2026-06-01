#pragma once

#include <string>

struct Config {
    int width = 900;
    int height = 900;
    double slopeThreshold = 2.0;
    double noiseLevel = 5.0;
    int kmeansK = 2;
    int componentMinSize = 2;
    int emK = 3;
    int maxK = 5;
    int clientConnectRetries = 30;
    int clientConnectRetryDelayMs = 250;

    void load(const std::string& filename);
};
