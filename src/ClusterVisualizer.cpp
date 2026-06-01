#include "ClusterVisualizer.h"
#include "GnuplotRenderer.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <vector>

namespace ClusterVisualizer {

namespace
{
struct Rgb
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

void writeBmp(const std::string& filename, int width, int height, const std::vector<Rgb>& pixels)
{
    std::ofstream f(filename, std::ios::binary);
    if (!f.is_open()) return;

    const int rowBytes = width * 3;
    const int padding = (4 - (rowBytes % 4)) % 4;
    const uint32_t imageSize = static_cast<uint32_t>((rowBytes + padding) * height);
    const uint32_t fileSize = 54 + imageSize;

    uint8_t header[54] = {'B', 'M'};
    std::memcpy(header + 2, &fileSize, 4);
    uint32_t offset = 54;
    std::memcpy(header + 10, &offset, 4);
    uint32_t dibSize = 40;
    std::memcpy(header + 14, &dibSize, 4);
    int32_t w = width;
    int32_t h = height;
    std::memcpy(header + 18, &w, 4);
    std::memcpy(header + 22, &h, 4);
    uint16_t planes = 1;
    uint16_t bpp = 24;
    std::memcpy(header + 26, &planes, 2);
    std::memcpy(header + 28, &bpp, 2);
    std::memcpy(header + 34, &imageSize, 4);
    f.write(reinterpret_cast<char*>(header), 54);

    const uint8_t zero[3] = {0, 0, 0};
    for (int y = height - 1; y >= 0; --y)
    {
        for (int x = 0; x < width; ++x)
        {
            const Rgb& p = pixels[y * width + x];
            f.put(static_cast<char>(p.b));
            f.put(static_cast<char>(p.g));
            f.put(static_cast<char>(p.r));
        }
        f.write(reinterpret_cast<const char*>(zero), padding);
    }
}

Rgb colorFor(int label)
{
    static const Rgb colors[] = {
        {230, 25, 75}, {60, 180, 75}, {0, 130, 200}, {245, 130, 48},
        {145, 30, 180}, {70, 240, 240}, {240, 50, 230}, {210, 245, 60}
    };
    return colors[std::max(0, label) % (sizeof(colors) / sizeof(colors[0]))];
}

void visualizeLabelsBmp(const std::string& labelsFile, const std::string& outFilename)
{
    std::ifstream terrain("output/terrain_data_2d.txt");
    if (!terrain.is_open()) return;

    struct Sample { int x; int y; double v; };
    std::vector<Sample> samples;
    int maxX = 0;
    int maxY = 0;
    double x = 0, y = 0, v = 0;
    while (terrain >> x >> y >> v)
    {
        const int ix = static_cast<int>(std::round(x));
        const int iy = static_cast<int>(std::round(y));
        samples.push_back({ix, iy, v});
        maxX = std::max(maxX, ix);
        maxY = std::max(maxY, iy);
    }

    const int width = maxX + 1;
    const int height = maxY + 1;
    if (width <= 0 || height <= 0) return;

    std::vector<Rgb> pixels(static_cast<std::size_t>(width * height), {255, 255, 255});
    for (const auto& sample : samples)
    {
        const uint8_t g = static_cast<uint8_t>(std::clamp(sample.v, 0.0, 255.0));
        pixels[sample.y * width + sample.x] = {g, g, g};
    }

    std::ifstream labels(labelsFile);
    int label = 0;
    while (labels >> x >> y >> label)
    {
        const int ix = static_cast<int>(std::round(x));
        const int iy = static_cast<int>(std::round(y));
        if (ix >= 0 && ix < width && iy >= 0 && iy < height)
        {
            pixels[iy * width + ix] = colorFor(label);
        }
    }

    writeBmp(outFilename, width, height, pixels);
}
}

void visualizeLabels(const std::string& labelsFile, const std::string& outFilename)
{
    if (std::filesystem::path(outFilename).extension() == ".bmp")
    {
        visualizeLabelsBmp(labelsFile, outFilename);
        return;
    }

    // Expect 'terrain_data_2d.txt' (image data) and a labels file with "x y label".
    std::ifstream fin(labelsFile);
    if (!fin.is_open()) return;

    // Create gnuplot datafile with cluster colors
    std::ofstream pdata("output/em_clusters_plot.txt");
    int label; double x,y;
    int maxLabel = 0;
    while (fin >> x >> y >> label) { pdata << x << " " << y << " " << label << "\n"; if (label>maxLabel) maxLabel=label; }
    pdata.close(); fin.close();

    std::ofstream gp("output/plot_clusters.gnuplot");
    gp << "set terminal pngcairo size 900,900\n";
    gp << "set output '" << outFilename << "'\n";
    gp << "unset key\nset size square\n";
    gp << "plot 'output/terrain_data_2d.txt' with image, \\\n'output/em_clusters_plot.txt' using 1:2:(int($3)+1) with points pt 7 ps 1 lc palette notitle\n";
    gp.close();

    GnuplotRenderer::executeScript("output/plot_clusters.gnuplot");
}

void visualize(const std::string& outFilename)
{
    visualizeLabels("output/em_clusters.txt", outFilename);
}

} // namespace
