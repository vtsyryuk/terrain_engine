#include "Scenarios.h"
#include "Logger.h"
#include "GnuplotRenderer.h"
#include "Analysis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <queue>
#include <random>
#include <string>

using namespace std;

namespace
{
struct Rgb
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

void writeRgbBmp(const string& filename, int width, int height, const vector<Rgb>& pixels)
{
    ofstream f(filename, ios::binary);
    if (!f.is_open()) return;

    const int rowBytes = width * 3;
    const int padding = (4 - (rowBytes % 4)) % 4;
    const uint32_t imageSize = static_cast<uint32_t>((rowBytes + padding) * height);
    const uint32_t fileSize = 54 + imageSize;
    uint8_t header[54] = {'B', 'M'};
    memcpy(header + 2, &fileSize, 4);
    uint32_t offset = 54;
    memcpy(header + 10, &offset, 4);
    uint32_t dibSize = 40;
    memcpy(header + 14, &dibSize, 4);
    int32_t w = width;
    int32_t h = height;
    memcpy(header + 18, &w, 4);
    memcpy(header + 22, &h, 4);
    uint16_t planes = 1;
    uint16_t bpp = 24;
    memcpy(header + 26, &planes, 2);
    memcpy(header + 28, &bpp, 2);
    memcpy(header + 34, &imageSize, 4);
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

void setPixel(vector<Rgb>& pixels, int width, int height, int x, int y, Rgb color)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        pixels[y * width + x] = color;
    }
}

void drawLine(vector<Rgb>& pixels, int width, int height, double ax, double ay, double bx, double by, Rgb color)
{
    int x0 = static_cast<int>(round(ax));
    int y0 = static_cast<int>(round(ay));
    const int x1 = static_cast<int>(round(bx));
    const int y1 = static_cast<int>(round(by));
    const int dx = abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true)
    {
        setPixel(pixels, width, height, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

string terminalFor(const string& filename, int width, int height)
{
    (void)filename;
    return "pngcairo size " + to_string(width) + "," + to_string(height);
}
}

struct Point2D {
    double x,y;
    double distance(const Point2D& o) const {
        return sqrt((x-o.x)*(x-o.x) + (y-o.y)*(y-o.y));
    }
};

struct Triangle {
    int p1,p2,p3;
    Point2D circumcenter;
    double circumradius;

    Triangle(int i1, int i2, int i3, const vector<Point2D>& pts)
        : p1(i1), p2(i2), p3(i3)
    {
        double x1 = pts[p1].x, y1 = pts[p1].y;
        double x2 = pts[p2].x, y2 = pts[p2].y;
        double x3 = pts[p3].x, y3 = pts[p3].y;
        double D = 2.0 * (x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2));
        if (fabs(D) < 1e-9) {
            circumcenter = {0,0};
            circumradius = 1e9;
            return;
        }
        circumcenter.x = ((x1*x1+y1*y1)*(y2-y3) + (x2*x2+y2*y2)*(y3-y1) + (x3*x3+y3*y3)*(y1-y2)) / D;
        circumcenter.y = ((x1*x1+y1*y1)*(x3-x2) + (x2*x2+y2*y2)*(x1-x3) + (x3*x3+y3*y3)*(x2-x1)) / D;
        circumradius = pts[p1].distance(circumcenter);
    }

    bool containsPoint(const Point2D& p) const {
        return circumcenter.distance(p) < circumradius - 1e-9;
    }
};

class GeometryHandler {
    vector<Point2D> points;
    vector<Triangle> triangles;
    int width, height;
public:
    GeometryHandler(const vector<Point2D>& centers, int w, int h)
        : points(centers), width(w), height(h)
    {
    }

    void buildDelaunay()
    {
        if (points.size() < 3) return;
        triangles.clear();
        int n = points.size();
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j)
                for (int k = j + 1; k < n; ++k) {
                    Triangle tri(i, j, k, points);
                    bool isDelaunay = true;
                    for (int m = 0; m < n; ++m) {
                        if (m == i || m == j || m == k) continue;
                        if (tri.containsPoint(points[m])) {
                            isDelaunay = false;
                            break;
                        }
                    }
                    if (isDelaunay) triangles.push_back(tri);
                }
        Logger::info("Delaunay triangles built: " + to_string(triangles.size()));
    }

    void visualize(const string& filename)
    {
        if (filesystem::path(filename).extension() == ".bmp") {
            vector<Rgb> pixels(static_cast<size_t>(width * height), {255, 255, 255});
            for (const auto& t : triangles) {
                const Point2D& a = points[t.p1];
                const Point2D& b = points[t.p2];
                const Point2D& c = points[t.p3];
                drawLine(pixels, width, height, a.x, a.y, b.x, b.y, {0, 0, 255});
                drawLine(pixels, width, height, b.x, b.y, c.x, c.y, {0, 0, 255});
                drawLine(pixels, width, height, c.x, c.y, a.x, a.y, {0, 0, 255});
            }
            for (size_t i = 0; i < triangles.size(); ++i)
                for (size_t j = i + 1; j < triangles.size(); ++j) {
                    int shared = 0;
                    if (triangles[i].p1 == triangles[j].p1 || triangles[i].p1 == triangles[j].p2 || triangles[i].p1 == triangles[j].p3) shared++;
                    if (triangles[i].p2 == triangles[j].p1 || triangles[i].p2 == triangles[j].p2 || triangles[i].p2 == triangles[j].p3) shared++;
                    if (triangles[i].p3 == triangles[j].p1 || triangles[i].p3 == triangles[j].p2 || triangles[i].p3 == triangles[j].p3) shared++;
                    if (shared == 2) {
                        drawLine(
                            pixels,
                            width,
                            height,
                            triangles[i].circumcenter.x,
                            triangles[i].circumcenter.y,
                            triangles[j].circumcenter.x,
                            triangles[j].circumcenter.y,
                            {255, 0, 0}
                        );
                    }
                }
            for (const auto& p : points) {
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        setPixel(pixels, width, height, static_cast<int>(round(p.x)) + dx, static_cast<int>(round(p.y)) + dy, {0, 0, 0});
            }
            writeRgbBmp(filename, width, height, pixels);
            Logger::info("Geometry visualization saved: " + filename);
            return;
        }

        ofstream dFile("output/delaunay.txt");
        ofstream vFile("output/voronoi.txt");
        ofstream pFile("output/points_centers.txt");
        for (const auto& t : triangles) {
            dFile << points[t.p1].x << " " << points[t.p1].y << "\n";
            dFile << points[t.p2].x << " " << points[t.p2].y << "\n";
            dFile << points[t.p3].x << " " << points[t.p3].y << "\n";
            dFile << points[t.p1].x << " " << points[t.p1].y << "\n\n";
        }
        for (size_t i = 0; i < triangles.size(); ++i)
            for (size_t j = i + 1; j < triangles.size(); ++j) {
                int shared = 0;
                if (triangles[i].p1 == triangles[j].p1 || triangles[i].p1 == triangles[j].p2 || triangles[i].p1 == triangles[j].p3) shared++;
                if (triangles[i].p2 == triangles[j].p1 || triangles[i].p2 == triangles[j].p2 || triangles[i].p2 == triangles[j].p3) shared++;
                if (triangles[i].p3 == triangles[j].p1 || triangles[i].p3 == triangles[j].p2 || triangles[i].p3 == triangles[j].p3) shared++;
                if (shared == 2) {
                    vFile << triangles[i].circumcenter.x << " " << triangles[i].circumcenter.y << "\n";
                    vFile << triangles[j].circumcenter.x << " " << triangles[j].circumcenter.y << "\n\n";
                }
            }
        for (const auto& p : points) pFile << p.x << " " << p.y << "\n";

        dFile.close();
        vFile.close();
        pFile.close();

        ofstream script("output/plot_geometry.gnuplot");
        script << "set terminal " << terminalFor(filename, 1000, 1000) << "\n";
        script << "set output '" << filename << "'\n";
        script << "set xrange [0:" << width << "]\n";
        script << "set yrange [0:" << height << "]\n";
        script << "set size square\n";
        script << "plot 'output/delaunay.txt' with lines lc rgb 'blue', \\\n'output/voronoi.txt' with lines lc rgb 'red' lw 2, 'output/points_centers.txt' with points pt 7 ps 2 lc rgb 'black'\n";
        script.close();
        GnuplotRenderer::executeScript("output/plot_geometry.gnuplot");
        Logger::info("Geometry visualization saved: " + filename);
    }
};

namespace Scenarios {

void applyNoise(LandscapeMap& map, double percent)
{
    if (percent <= 0.0) return;
    unsigned seed = 0;
    if (const char* envSeed = std::getenv("TERRAIN_NOISE_SEED"))
    {
        seed = static_cast<unsigned>(std::stoul(envSeed));
    }
    mt19937 gen(seed);
    uniform_real_distribution<double> dis(0.0, 1.0);
    int affected = 0;
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            if (dis(gen) < percent / 100.0) {
                map.at(x, y) += (dis(gen) - 0.5) * 40.0;
                map.at(x, y) = max(0.0, min(255.0, map.at(x, y)));
                affected++;
            }
    Logger::info("NoiseApplier: Applied " + to_string(percent) + "% noise (" + to_string(affected) + " pixels)");
}

void smooth(LandscapeMap& map)
{
    auto copy = map.data();
    double kernel[3][3] = {{1,2,1},{2,4,2},{1,2,1}};
    for (int y = 1; y < map.height() - 1; ++y)
        for (int x = 1; x < map.width() - 1; ++x) {
            double sum = 0;
            double wsum = 0;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    double w = kernel[dy + 1][dx + 1];
                    sum += copy[y + dy][x + dx] * w;
                    wsum += w;
                }
            map.at(x, y) = sum / wsum;
        }
}

void writeBMP(const LandscapeMap& map, const string& filename)
{
    ofstream f(filename, ios::binary);
    if (!f.is_open()) {
        Logger::info("ERROR: Cannot open file for BMP: " + filename);
        return;
    }

    int pad = (4 - (map.width() % 4)) % 4;
    uint32_t imgSz = static_cast<uint32_t>((map.width() + pad) * map.height());
    uint32_t fileSize = 54 + 256 * 4 + imgSz;
    uint8_t hdr[54] = {0x42,0x4D};
    memcpy(hdr + 2, &fileSize, 4);
    uint32_t offset = 54 + 256 * 4;
    memcpy(hdr + 10, &offset, 4);
    uint32_t headerSize = 40;
    memcpy(hdr + 14, &headerSize, 4);
    int32_t w = map.width();
    int32_t h = map.height();
    memcpy(hdr + 18, &w, 4);
    memcpy(hdr + 22, &h, 4);
    uint16_t planes = 1;
    uint16_t bpp = 8;
    memcpy(hdr + 26, &planes, 2);
    memcpy(hdr + 28, &bpp, 2);
    uint32_t imgBytes = imgSz;
    memcpy(hdr + 34, &imgBytes, 4);
    f.write(reinterpret_cast<char*>(hdr), 54);
    for (int i = 0; i < 256; ++i) {
        uint8_t c = static_cast<uint8_t>(i);
        f.write(reinterpret_cast<char*>(&c), 1);
        f.write(reinterpret_cast<char*>(&c), 1);
        f.write(reinterpret_cast<char*>(&c), 1);
        f.put(0);
    }
    for (int y = map.height() - 1; y >= 0; --y) {
        for (int x = 0; x < map.width(); ++x) {
            uint8_t v = static_cast<uint8_t>(max(0.0, min(255.0, map.data()[y][x])));
            f.write(reinterpret_cast<char*>(&v), 1);
        }
        for (int i = 0; i < pad; ++i) f.put(0);
    }
    Logger::info("BMP saved: " + filename);
}

vector<vector<pair<int,int>>> findConnectedComponents(const LandscapeMap& map, int min_size)
{
    vector<vector<pair<int,int>>> components;
    vector<vector<bool>> visited(map.height(), vector<bool>(map.width(), false));
    const int dx[8] = {-1,-1,-1,0,0,1,1,1};
    const int dy[8] = {-1,0,1,-1,1,-1,0,1};
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x) {
            if (map.at(x,y) > 200 && !visited[y][x]) {
                vector<pair<int,int>> comp;
                queue<pair<int,int>> q;
                q.push({x,y});
                visited[y][x] = true;
                while (!q.empty()) {
                    auto p = q.front(); q.pop();
                    comp.push_back(p);
                    for (int d = 0; d < 8; ++d) {
                        int nx = p.first + dx[d];
                        int ny = p.second + dy[d];
                        if (nx >= 0 && nx < map.width() && ny >= 0 && ny < map.height() && map.at(nx,ny) > 200 && !visited[ny][nx]) {
                            visited[ny][nx] = true;
                            q.push({nx,ny});
                        }
                    }
                }
                if (comp.size() >= static_cast<size_t>(min_size))
                    components.push_back(comp);
                else
                    Logger::info("Dust ignored: component size = " + to_string(comp.size()));
            }
        }
    return components;
}

void fieldGeneration(LandscapeMap& map, const vector<GaussianBell>& bells, double noise_percent)
{
    for (auto& b : bells) map.add(b);
    map.normalize();
    applyNoise(map, noise_percent);
    smooth(map);
    smooth(map);
    Logger::info("FieldGenerationScenario completed");
}

void plot3D(const LandscapeMap& map)
{
    ofstream data("output/terrain_data.txt");
    for (int y = 0; y < map.height(); y += 5) {
        for (int x = 0; x < map.width(); x += 5)
            data << x << " " << y << " " << map.at(x,y) << "\n";
        data << "\n";
    }
    data.close();

    ofstream gp("output/my_plot_script.gnuplot");
    gp << "set terminal pngcairo size 1200,900\n";
    gp << "set output 'output/terrain_3d.png'\n";
    gp << "set title 'Terrain Field (3D)'\n";
    gp << "set pm3d\n";
    gp << "splot 'output/terrain_data.txt' with pm3d\n";
    gp.close();
    GnuplotRenderer::executeScript("output/my_plot_script.gnuplot");
    Logger::info("3D Plot created: output/terrain_3d.png");
}

void writeImageData(const LandscapeMap& map, const string& filename)
{
    ofstream data(filename);
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x)
            data << x << " " << y << " " << map.at(x,y) << "\n";
        data << "\n";
    }
    data.close();
}

void plot2D(const LandscapeMap& map)
{
    writeImageData(map, "output/terrain_data_2d.txt");

    ofstream gp("output/my_plot_2d.gnuplot");
    gp << "set terminal pngcairo size 900,900\n";
    gp << "set output 'output/terrain_2d.png'\n";
    gp << "plot 'output/terrain_data_2d.txt' with image\n";
    gp.close();
    GnuplotRenderer::executeScript("output/my_plot_2d.gnuplot");
    Logger::info("2D Heatmap created: output/terrain_2d.png");
}

void slopeAnalysis(const LandscapeMap& map)
{
    ofstream slope("output/gradient_vectors.txt");
    int step = 20;
    for (int y = step; y < map.height() - step; y += step)
        for (int x = step; x < map.width() - step; x += step) {
            auto g = map.gradient(x,y);
            double ang = atan2(g.second, g.first);
            double mag = sqrt(g.first*g.first + g.second*g.second);
            slope << x << " " << y << " " << ang << " " << mag << "\n";
        }
    slope.close();
    Logger::info("SlopeAnalysisScenario completed");
}

void slopeCheck(const LandscapeMap& map, double threshold)
{
    slopeCheck(map, threshold, "output/steepness_map.bmp");
}

void slopeCheck(const LandscapeMap& map, double threshold, const string& filename)
{
    LandscapeMap check(map.width(), map.height());
    if (threshold <= 0.0) threshold = 1.0;
    for (int y = 1; y < map.height() - 1; ++y)
        for (int x = 1; x < map.width() - 1; ++x) {
            auto g = map.gradient(x,y);
            double steep = sqrt(g.first*g.first + g.second*g.second);
            check.at(x,y) = steep < threshold ? 255.0 : 0.0;
        }
    writeBMP(check, filename);
    Logger::info("Steepness map created (threshold = " + to_string(threshold) + ")");
}

void componentSearch(const LandscapeMap& map, int k, int min_size)
{
    Analysis::kmeans_cluster(map, k, min_size);
    Logger::info("ComponentSearchScenario completed (K=" + to_string(k) + ")");
}

void geometryScenario(const LandscapeMap& map, int min_size)
{
    geometryScenario(map, min_size, "output/my_delaunay_voronoi.png");
}

void geometryScenario(const LandscapeMap& map, int min_size, const string& filename)
{
    auto comps = findConnectedComponents(map, min_size);
    vector<Point2D> centers;
    for (auto& comp : comps) {
        double sx = 0, sy = 0;
        for (auto& p : comp) {
            sx += p.first;
            sy += p.second;
        }
        centers.push_back({sx / comp.size(), sy / comp.size()});
    }
    if (centers.size() < 3) {
        Logger::info("Not enough centers for Delaunay");
        return;
    }
    GeometryHandler gh(centers, map.width(), map.height());
    gh.buildDelaunay();
    gh.visualize(filename);
    Logger::info("GeometryScenario completed (" + to_string(centers.size()) + " centers)");
}

} // namespace Scenarios
