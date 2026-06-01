
#pragma once

#include "GaussianBell.h"
#include "LandscapeMap.h"

#include <vector>
#include <string>
#include <utility>

class TerrainEngine
{
public:
    TerrainEngine(int width, int height);

    void addGaussian(const GaussianBell& bell);

    void generate();

    void saveBMP(const std::string& filename) const;

    void saveTerrainData(const std::string& filename) const;

    void render3DGnuplot() const;

    void render2DGnuplot() const;

    std::pair<double, double> gradient(int x, int y) const;

    const std::vector<std::vector<double>>& data() const;

    LandscapeMap& map();
    const LandscapeMap& map() const;

private:
    void normalize();

    LandscapeMap map_;
};
