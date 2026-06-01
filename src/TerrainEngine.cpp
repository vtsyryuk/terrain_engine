
#include "TerrainEngine.h"
#include "GnuplotRenderer.h"
#include "Scenarios.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace
{
double gaussianValue(const GaussianBell& bell, double x, double y)
{
    const double dx = x - bell.cx;
    const double dy = y - bell.cy;
    const double nx = dx / bell.sx;
    const double ny = dy / bell.sy;
    const double denom = 1.0 - bell.rho * bell.rho;
    if (std::abs(denom) < 1e-10)
    {
        return 0.0;
    }

    const double q = nx * nx + ny * ny - 2.0 * bell.rho * nx * ny;
    return bell.sign * std::exp(-0.5 * q / denom);
}
}

TerrainEngine::TerrainEngine(int width, int height)
    : map_(width, height)
{
}

void TerrainEngine::setNoisePercent(double noisePercent)
{
    noisePercent_ = std::max(0.0, noisePercent);
}

void TerrainEngine::addGaussian(const GaussianBell& bell)
{
    if (bell.sx <= 0 || bell.sy <= 0)
    {
        throw std::runtime_error("Invalid Gaussian sigma");
    }
    if (std::abs(bell.rho) >= 1.0)
    {
        throw std::runtime_error("rho must be in (-1,1)");
    }

    bells_.push_back(bell);
}

void TerrainEngine::generate()
{
    map_.clear();
    Scenarios::fieldGeneration(map_, bells_, noisePercent_);
}

void TerrainEngine::saveBMP(const std::string& filename) const
{
    map_.saveBMP(filename);
}

void TerrainEngine::saveTerrainData(const std::string& filename) const
{
    std::ofstream data(filename);
    for (int y = 0; y < map_.height(); y += 5)
    {
        for (int x = 0; x < map_.width(); x += 5)
            data << x << " " << y << " " << map_.at(x, y) << "\n";
        data << "\n";
    }
}

void TerrainEngine::saveRawTerrainData(const std::string& filename, int step) const
{
    if (step <= 0)
    {
        step = 1;
    }

    std::ofstream data(filename);
    for (int y = 0; y < map_.height(); y += step)
    {
        for (int x = 0; x < map_.width(); x += step)
        {
            double value = 0.0;
            for (const auto& bell : bells_)
            {
                value += gaussianValue(bell, x, y);
            }
            data << x << " " << y << " " << value << "\n";
        }
        data << "\n";
    }
}

void TerrainEngine::render3DGnuplot() const
{
    saveTerrainData("output/terrain_data.txt");
    std::ofstream gp("output/plot3d.gnuplot");
    gp << "set terminal pngcairo size 1200,900\n";
    gp << "set output 'output/terrain_3d.png'\n";
    gp << "set title 'Terrain Field (3D)'\n";
    gp << "set pm3d\n";
    gp << "splot 'output/terrain_data.txt' with pm3d\n";
    gp.close();
    GnuplotRenderer::executeScript("output/plot3d.gnuplot");
}

void TerrainEngine::render2DGnuplot() const
{
    saveTerrainData("output/terrain_data_2d.txt");
    std::ofstream gp("output/plot2d.gnuplot");
    gp << "set terminal pngcairo size 900,900\n";
    gp << "set output 'output/terrain_2d.png'\n";
    gp << "plot 'output/terrain_data_2d.txt' with image\n";
    gp.close();
    GnuplotRenderer::executeScript("output/plot2d.gnuplot");
}

std::pair<double,double> TerrainEngine::gradient(int x, int y) const
{
    return map_.gradient(x, y);
}

const std::vector<std::vector<double>>& TerrainEngine::data() const
{
    return map_.data();
}

LandscapeMap& TerrainEngine::map()
{
    return map_;
}

const LandscapeMap& TerrainEngine::map() const
{
    return map_;
}

void TerrainEngine::normalize()
{
    map_.normalize();
}
