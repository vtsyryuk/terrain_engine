
#pragma once

#include <string>

class TerrainEngine;

class CommandProcessor
{
public:
    explicit CommandProcessor(TerrainEngine& engine);

    void executeFile(const std::string& filename);

private:
    TerrainEngine& engine_;
};
