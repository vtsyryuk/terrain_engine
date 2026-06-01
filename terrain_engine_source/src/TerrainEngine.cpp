
#include "TerrainEngine.h"
#include "GnuplotRenderer.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>

TerrainEngine::TerrainEngine(int width, int height)
    : map_(width, height)
{
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

    map_.add(bell);
}

void TerrainEngine::generate()
{
    normalize();
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
