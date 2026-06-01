
#pragma once

#include <string>

class TerrainEngine;
struct Config;

class CommandProcessor
{
public:
    explicit CommandProcessor(TerrainEngine& engine);
    CommandProcessor(TerrainEngine& engine, const Config& config);

    std::string executeLine(const std::string& line);

    void executeFile(const std::string& filename);

private:
    TerrainEngine& engine_;
    double slopeThreshold_ = 2.0;
    int kmeansK_ = 2;
    int componentMinSize_ = 2;
    int emK_ = 3;
};
