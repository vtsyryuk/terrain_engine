#include "LogManager.h"
#include "Logger.h"
#include "CommandProcessor.h"
#include "TerrainEngine.h"
#include "ClusterVisualizer.h"
#include <filesystem>
#include <iostream>

int main()
{
    // initialize logging
    log_mgr.setup("app");
    log_mgr.log_user("Terrain generator started");

    // Create engine and command processor
    TerrainEngine engine(900, 900);
    CommandProcessor processor(engine);

    // Ensure output directory exists and switch to it
    namespace fs = std::filesystem;
    fs::path outDir("output");
    try {
        if (!fs::exists(outDir)) fs::create_directory(outDir);
        fs::current_path(outDir);
        log_mgr.log_system(std::string("Output directory set to: ") + fs::current_path().string());
    } catch (const std::exception& ex) {
        log_mgr.log_system(std::string("ERROR: cannot set output dir: ") + ex.what());
    }

    // Execute commands
    processor.executeFile("commands.txt");

    // Try cluster visualization if clusters exist
    ClusterVisualizer::visualize("clusters.png");

    log_mgr.log_user("Execution completed successfully.");
    std::cout << "Program completed. Check output/ and logs/ for results.\n";
    return 0;
}

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <random>
#include <thread>
#include <atomic>
#include <filesystem>
#include <numeric>

using namespace std;

// ====================== ILandscapeOperations ======================
class ILandscapeOperations
{
public:
    virtual ~ILandscapeOperations() = default;
    virtual void addGauss(int sign, double cx, double cy, double sx, double sy, double rho) = 0;
    virtual void generate() = 0;
    virtual void plot() = 0;
    virtual void plot2D() = 0;
    virtual void saveBMP(const string& filename = "") = 0;
    virtual void analiz() = 0;
    virtual void slopeCheck() = 0;
    virtual void componentSearch() = 0;
    virtual void geometry() = 0;
};

#include "LogManager.h"
#include "Logger.h"
#include "GnuplotRenderer.h"
#include "Scenarios.h"
#include "Analysis.h"
#include "ClusterVisualizer.h"

void deleteIfExists(const string& filename)
{
    if (remove(filename.c_str()) == 0)
        log_mgr.log_system("Deleted old file: " + filename);
}

// Removed Windows named-pipe client/server. This file now runs as a single-process sequential tool.

// ====================== Settings ======================
struct Settings
{
    int width = 900, height = 900;
    int kmeans_k = 2;
    double slope_threshold = 2.0;
    double noise_level = 5.0;
    int min_cluster_size = 2;
    int max_k = 5;

    bool load(const string& path)
    {
        ifstream f(path);
        if (!f.is_open()) return false;
        string line;
        while (getline(f, line))
        {
            if (line.empty() || line[0] == '#') continue;
            size_t eq = line.find('=');
            if (eq == string::npos) continue;
            string key = line.substr(0, eq);
            string val = line.substr(eq + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);

            if (key == "WIDTH") width = stoi(val);
            else if (key == "HEIGHT") height = stoi(val);
            else if (key == "KMEANS_K") kmeans_k = stoi(val);
            else if (key == "SLOPE_THRESHOLD") slope_threshold = stod(val);
            else if (key == "NOISE_LEVEL") noise_level = stod(val);
            else if (key == "MIN_CLUSTER_SIZE") min_cluster_size = stoi(val);
            else if (key == "MAX_K") max_k = stoi(val);
        }
        return true;
    }
};

// ========================= GAUSSIAN & LANDSCAPE =========================
struct GaussianBell
{
    int sign;
    double cx, cy, sx, sy, rho;
    GaussianBell(int s, double x, double y, double vx, double vy, double r)
        : sign(s), cx(x), cy(y), sx(vx), sy(vy), rho(r) {}

    double value(double x, double y) const
    {
        double dx = x - cx, dy = y - cy;
        double nx = dx / sx, ny = dy / sy;
        double denom = 1 - rho * rho;
        if (fabs(denom) < 1e-10) return 0.0;
        double q = nx*nx + ny*ny - 2*rho*nx*ny;
        return sign * exp(-0.5 * q / denom);
    }
};

// ========================= LANDSCAPE MAP (объявление) =========================
class LandscapeMap
{
public:
    int w, h;
    vector<vector<double>> data;

    LandscapeMap(int width, int height)
        : w(width), h(height)
    {
        if (width <= 0 || height <= 0)
        {
            log_mgr.log_system("ERROR: Invalid LandscapeMap size: "
                               + to_string(width) + "x" + to_string(height));
            w = 1;
            h = 1; // минимальная защита, чтобы не упало дальше
        }
        data = vector<vector<double>>(h, vector<double>(w, 0.0));
        log_mgr.log_system("Grid initialized " + to_string(w) + "x" + to_string(h));
    }

    void add(const GaussianBell& b);
    void normalize();
    pair<double, double> gradient(int x, int y) const;
    void saveBMP(const string& fn) const;
};

// ========================= BMP WRITER =========================
class BMPWriter
{
public:
    static void write(const LandscapeMap& map, const string& filename);
};

// ========================= LANDSCAPE MAP (реализация методов) =========================
void LandscapeMap::add(const GaussianBell& b)
{
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            data[y][x] += b.value(x, y);
}

void LandscapeMap::normalize()
{
    double mn = 1e9, mx = -1e9;
    for (auto& row : data)
        for (double v : row)
        {
            mn = min(mn, v);
            mx = max(mx, v);
        }
    double r = mx - mn;
    if (r < 1e-8) return;
    for (auto& row : data)
        for (double& v : row)
            v = (v - mn) / r * 255.0;
}

pair<double, double> LandscapeMap::gradient(int x, int y) const
{
    if (x <= 0 || x >= w - 1 || y <= 0 || y >= h - 1)
        return {0.0, 0.0};
    double dx = (data[y][x + 1] - data[y][x - 1]) / 2.0;
    double dy = (data[y + 1][x] - data[y - 1][x]) / 2.0;
    return {dx, dy};
}

void LandscapeMap::saveBMP(const string& fn) const
{
    BMPWriter::write(*this, fn);
}

// ========================= BMP WRITER (реализация) =========================
void BMPWriter::write(const LandscapeMap& map, const string& filename)
{
    ofstream f(filename, ios::binary);
    if (!f.is_open())
    {
        log_mgr.log_system("ERROR: Cannot open file for BMP: " + filename);
        return;
    }

    int pad = (4 - (map.w % 4)) % 4;
    uint32_t imgSz = static_cast<uint32_t>((map.w + pad) * map.h);
    uint32_t fileSize = 54 + 256 * 4 + imgSz;

    uint8_t hdr[54] = {0x42, 0x4D};
    memcpy(hdr + 2, &fileSize, 4);
    uint32_t offset = 54 + 256 * 4;
    memcpy(hdr + 10, &offset, 4);
    uint32_t headerSize = 40;
    memcpy(hdr + 14, &headerSize, 4);

    int32_t w = map.w;
    int32_t h = map.h;
    memcpy(hdr + 18, &w, 4);
    memcpy(hdr + 22, &h, 4);

    uint16_t planes = 1;
    uint16_t bpp = 8;
    memcpy(hdr + 26, &planes, 2);
    memcpy(hdr + 28, &bpp, 2);
    memcpy(hdr + 34, &imgSz, 4);

    f.write(reinterpret_cast<char*>(hdr), 54);

    for (int i = 0; i < 256; i++)
    {
        uint8_t c = static_cast<uint8_t>(i);
        f.write(reinterpret_cast<char*>(&c), 1);
        f.write(reinterpret_cast<char*>(&c), 1);
        f.write(reinterpret_cast<char*>(&c), 1);
        f.put(0);
    }

    for (int y = map.h - 1; y >= 0; y--)
    {
        for (int x = 0; x < map.w; x++)
        {
            uint8_t v = static_cast<uint8_t>(max(0.0, min(255.0, map.data[y][x])));
            f.write(reinterpret_cast<char*>(&v), 1);
        }
        for (int i = 0; i < pad; i++) f.put(0);
    }

    log_mgr.log_system("BMP saved: " + filename);
}

// ========================= NOISE & SMOOTH =========================
class NoiseApplier
{
public:
    static void apply(LandscapeMap& map, double percent)
    {
        if (percent <= 0.0) return;
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<double> dis(0.0, 1.0);
        int affected = 0;
        for (int y = 0; y < map.h; y++)
        {
            for (int x = 0; x < map.w; x++)
            {
                if (dis(gen) < percent / 100.0)
                {
                    map.data[y][x] += (dis(gen) - 0.5) * 40.0;
                    map.data[y][x] = max(0.0, min(255.0, map.data[y][x]));
                    affected++;
                }
            }
        }
        log_mgr.log_system("NoiseApplier: Applied " + to_string(percent) + "% noise (" + to_string(affected) + " pixels)");
    }
};

void smooth(LandscapeMap& map)
{
    auto copy = map.data;
    double kernel[3][3] = {{1,2,1},{2,4,2},{1,2,1}};
    for (int y = 1; y < map.h - 1; y++)
    {
        for (int x = 1; x < map.w - 1; x++)
        {
            double sum = 0, wsum = 0;
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    double w = kernel[dy + 1][dx + 1];
                    sum += copy[y + dy][x + dx] * w;
                    wsum += w;
                }
            }
            map.data[y][x] = sum / wsum;
        }
    }
}

// ========================= KMEANS =========================
class KMeansClusterer
{
public:
    static vector<int> cluster(const LandscapeMap& map, int k, int min_size)
    {
        int n = map.w * map.h;
        vector<int> labels(n, -1);
        vector<pair<double, double>> points;
        vector<int> point_idx;
        for (int y = 0; y < map.h; y++)
            for (int x = 0; x < map.w; x++)
                if (map.data[y][x] >= 128)
                {
                    points.emplace_back(x, y);
                    point_idx.push_back(y * map.w + x);
                }
        int np = points.size();
        if (np == 0) return labels;
        k = min(k, np);
        vector<pair<double, double>> centroids(k);
        for (int i = 0; i < k; i++)
        {
            int ri = rand() % np;
            centroids[i] = points[ri];
        }
        bool changed = true;
        int maxIter = 50;
        while (changed && maxIter--)
        {
            changed = false;
            vector<vector<int>> r(k, vector<int>(np, 0));
            for (int i = 0; i < np; i++)
            {
                int best = 0;
                double md = 1e9;
                for (int c = 0; c < k; c++)
                {
                    double d = pow(points[i].first - centroids[c].first, 2) +
                               pow(points[i].second - centroids[c].second, 2);
                    if (d < md)
                    {
                        md = d;
                        best = c;
                    }
                }
                r[best][i] = 1;
                int idx = point_idx[i];
                if (labels[idx] != best)
                {
                    changed = true;
                    labels[idx] = best;
                }
            }
            for (int c = 0; c < k; c++)
            {
                double sx = 0.0, sy = 0.0, sum_r = 0.0;
                for (int i = 0; i < np; i++)
                {
                    if (r[c][i])
                    {
                        sx += points[i].first;
                        sy += points[i].second;
                        sum_r += 1.0;
                    }
                }
                if (sum_r >= min_size && sum_r > 0)
                {
                    centroids[c] = {sx / sum_r, sy / sum_r};
                }
            }
        }
        return labels;
    }
};

// ========================= EM (Gaussian Mixture) =========================
class EMClusterer
{
public:
    static vector<int> cluster(const LandscapeMap& map, int k, int min_size, int maxIter = 100)
    {
        vector<pair<double,double>> points;
        vector<int> point_idx;
        for (int y = 0; y < map.h; y++)
            for (int x = 0; x < map.w; x++)
                if (map.data[y][x] >= 128)
                {
                    points.emplace_back((double)x, (double)y);
                    point_idx.push_back(y * map.w + x);
                }
        int np = points.size();
        vector<int> labels(map.w * map.h, -1);
        if (np == 0) return labels;
        k = min(k, np);

        // Initialize: deterministic sample of first k points
        vector<pair<double,double>> mu(k);
        for (int i = 0; i < k; ++i) mu[i] = points[i % np];

        vector<double> weights(k, 1.0 / k);
        vector<array<array<double,2>,2>> cov(k);
        for (int i = 0; i < k; ++i)
        {
            cov[i] = {array<double,2>{100.0,0.0}, array<double,2>{0.0,100.0}};
        }

        vector<vector<double>> resp(np, vector<double>(k, 0.0));

        const double PI = 3.14159265358979323846;
        auto gaussian2d = [&](int c, const pair<double,double>& p)->double {
            double dx = p.first - mu[c].first;
            double dy = p.second - mu[c].second;
            double varx = cov[c][0][0];
            double vary = cov[c][1][1];
            double det = varx * vary;
            double denom = 2*M_PI*sqrt(det);
            double ex = (dx*dx/varx + dy*dy/vary) / 2.0;
            if (denom <= 0) return 1e-12;
            return exp(-ex) / denom;
        };

        for (int iter = 0; iter < maxIter; ++iter)
        {
            // E-step
            for (int i = 0; i < np; ++i)
            {
                double s = 0.0;
                for (int c = 0; c < k; ++c)
                {
                    resp[i][c] = weights[c] * gaussian2d(c, points[i]);
                    s += resp[i][c];
                }
                if (s <= 0) s = 1e-12;
                for (int c = 0; c < k; ++c) resp[i][c] /= s;
            }

            // M-step
            for (int c = 0; c < k; ++c)
            {
                double Nk = 0.0;
                double mx = 0.0, my = 0.0;
                for (int i = 0; i < np; ++i)
                {
                    Nk += resp[i][c];
                    mx += resp[i][c] * points[i].first;
                    my += resp[i][c] * points[i].second;
                }
                if (Nk <= 1e-12) continue;
                mu[c].first = mx / Nk;
                mu[c].second = my / Nk;

                double vx = 0.0, vy = 0.0;
                for (int i = 0; i < np; ++i)
                {
                    double dx = points[i].first - mu[c].first;
                    double dy = points[i].second - mu[c].second;
                    vx += resp[i][c] * dx * dx;
                    vy += resp[i][c] * dy * dy;
                }
                cov[c][0][0] = max(1e-3, vx / Nk);
                cov[c][1][1] = max(1e-3, vy / Nk);
                weights[c] = Nk / np;
            }
        }

        // Assign labels
        for (int i = 0; i < np; ++i)
        {
            int best = 0; double bestv = resp[i][0];
            for (int c = 1; c < k; ++c) if (resp[i][c] > bestv) { bestv = resp[i][c]; best = c; }
            labels[point_idx[i]] = best;
        }

        // Save clustering result
        ofstream out("em_clusters.txt");
        for (int i = 0; i < np; ++i)
            out << points[i].first << " " << points[i].second << " " << labels[point_idx[i]] << "\n";
        out.close();

        Logger::info("EM clustering completed (k=" + to_string(k) + ")");

        return labels;
    }
};

// ========================= CONNECTED COMPONENTS =========================
class ConnectedComponentFinder
{
public:
    static vector<vector<pair<int, int>>> find(const LandscapeMap& map, int min_size)
    {
        vector<vector<pair<int, int>>> components;
        vector<vector<bool>> visited(map.h, vector<bool>(map.w, false));
        const int dx[8] = {-1,-1,-1,0,0,1,1,1};
        const int dy[8] = {-1,0,1,-1,1,-1,0,1};
        for (int y = 0; y < map.h; y++)
        {
            for (int x = 0; x < map.w; x++)
            {
                if (map.data[y][x] > 200 && !visited[y][x])
                {
                    vector<pair<int, int>> comp;
                    queue<pair<int, int>> q;
                    q.push({x, y});
                    visited[y][x] = true;
                    while (!q.empty())
                    {
                        auto [cx, cy] = q.front();
                        q.pop();
                        comp.emplace_back(cx, cy);
                        for (int d = 0; d < 8; d++)
                        {
                            int nx = cx + dx[d], ny = cy + dy[d];
                            if (nx >= 0 && nx < map.w && ny >= 0 && ny < map.h &&
                                    map.data[ny][nx] > 200 && !visited[ny][nx])
                            {
                                visited[ny][nx] = true;
                                q.push({nx, ny});
                            }
                        }
                    }
                    if (comp.size() >= static_cast<size_t>(min_size))
                        components.push_back(comp);
                    else
                        log_mgr.log_system("Dust ignored: component size = " + to_string(comp.size()));
                }
            }
        }
        return components;
    }
};


// ========================= GEOMETRY =========================
struct Point2D
{
    double x, y;
    double distance(const Point2D& other) const
    {
        return sqrt((x - other.x)*(x - other.x) + (y - other.y)*(y - other.y));
    }
};

struct Triangle
{
    int p1, p2, p3;
    Point2D circumcenter;
    double circumradius;
    Triangle(int i1, int i2, int i3, const vector<Point2D>& pts) : p1(i1), p2(i2), p3(i3)
    {
        double x1 = pts[p1].x, y1 = pts[p1].y;
        double x2 = pts[p2].x, y2 = pts[p2].y;
        double x3 = pts[p3].x, y3 = pts[p3].y;
        double D = 2 * (x1*(y2 - y3) + x2*(y3 - y1) + x3*(y1 - y2));
        if (fabs(D) < 1e-9)
        {
            circumcenter = {0,0};
            circumradius = 1e9;
            return;
        }
        circumcenter.x = ((x1*x1 + y1*y1)*(y2 - y3) + (x2*x2 + y2*y2)*(y3 - y1) + (x3*x3 + y3*y3)*(y1 - y2)) / D;
        circumcenter.y = ((x1*x1 + y1*y1)*(x3 - x2) + (x2*x2 + y2*y2)*(x1 - x3) + (x3*x3 + y3*y3)*(x2 - x1)) / D;
        circumradius = pts[p1].distance(circumcenter);
    }
    bool containsPoint(const Point2D& p) const
    {
        return circumcenter.distance(p) < circumradius - 1e-9;
    }
};

class GeometryHandler
{
    vector<Point2D> points;
    vector<Triangle> triangles;
    int width, height;
public:
    GeometryHandler(const vector<Point2D>& centers, int w, int h)
        : points(centers), width(w), height(h) {}
    void buildDelaunay()
    {
        if (points.size() < 3) return;
        triangles.clear();
        int n = points.size();
        for (int i = 0; i < n; i++)
        {
            for (int j = i + 1; j < n; j++)
            {
                for (int k = j + 1; k < n; k++)
                {
                    Triangle tri(i, j, k, points);
                    bool isDelaunay = true;
                    for (int m = 0; m < n; m++)
                    {
                        if (m == i || m == j || m == k) continue;
                        if (tri.containsPoint(points[m]))
                        {
                            isDelaunay = false;
                            break;
                        }
                    }
                    if (isDelaunay) triangles.push_back(tri);
                }
            }
        }
        log_mgr.log_system("Delaunay triangles built: " + to_string(triangles.size()));
    }
    void visualize(const string& filename)
    {
        ofstream dFile("delaunay.txt"), vFile("voronoi.txt"), pFile("points_centers.txt");
        for (const auto& t : triangles)
        {
            dFile << points[t.p1].x << " " << points[t.p1].y << "\n";
            dFile << points[t.p2].x << " " << points[t.p2].y << "\n";
            dFile << points[t.p3].x << " " << points[t.p3].y << "\n";
            dFile << points[t.p1].x << " " << points[t.p1].y << "\n\n";
        }
        for (size_t i = 0; i < triangles.size(); i++)
        {
            for (size_t j = i + 1; j < triangles.size(); j++)
            {
                int shared = 0;
                if (triangles[i].p1 == triangles[j].p1 || triangles[i].p1 == triangles[j].p2 || triangles[i].p1 == triangles[j].p3) shared++;
                if (triangles[i].p2 == triangles[j].p1 || triangles[i].p2 == triangles[j].p2 || triangles[i].p2 == triangles[j].p3) shared++;
                if (triangles[i].p3 == triangles[j].p1 || triangles[i].p3 == triangles[j].p2 || triangles[i].p3 == triangles[j].p3) shared++;
                if (shared == 2)
                {
                    vFile << triangles[i].circumcenter.x << " " << triangles[i].circumcenter.y << "\n";
                    vFile << triangles[j].circumcenter.x << " " << triangles[j].circumcenter.y << "\n\n";
                }
            }
        }
        for (auto& p : points) pFile << p.x << " " << p.y << "\n";
        dFile.close();
        vFile.close();
        pFile.close();

        ofstream script("plot_geometry.gnuplot");
        script << "set terminal pngcairo size 1000,1000\nset output '" << filename << "'\n";
        script << "set xrange [0:" << width << "]\n";
        script << "set yrange [0:" << height << "]\n";
        script << "set size square\n";
        script << "plot 'delaunay.txt' with lines lc rgb 'blue' title 'Delaunay', \\\n";
        script << "'voronoi.txt' with lines lc rgb 'red' lw 2 title 'Voronoi', \\\n";
        script << "'points_centers.txt' with points pt 7 ps 2 lc rgb 'black' title 'Centers'\n";
        script.close();
        GnuplotRenderer::executeScript("plot_geometry.gnuplot");
        log_mgr.log_system("Geometry visualization saved: " + filename);
    }
};

// ========================= SCENARIOS =========================
class FieldGenerationScenario
{
public:
    static void execute(LandscapeMap& map, const vector<GaussianBell>& bells, double noise_percent)
    {
        for (const auto& b : bells) map.add(b);
        map.normalize();
        Scenarios::applyNoise(map, noise_percent);
        Scenarios::smooth(map);
        Scenarios::smooth(map);
        log_mgr.log_system("FieldGenerationScenario completed");
    }
};

class PlotScenario
{
public:
    static void execute(const LandscapeMap& map)
    {
        ofstream data("terrain_data.txt");
        for (int y = 0; y < map.h; y += 5)
        {
            for (int x = 0; x < map.w; x += 5)
            {
                data << x << " " << y << " " << map.data[y][x] << "\n";
            }
            data << "\n";
        }
        data.close();
        ofstream gp("my_plot_script.gnuplot");
        gp << "set terminal pngcairo size 1200,900\nset output 'terrain_3d.png'\n";
        gp << "set title 'Terrain Field (3D)'\nset pm3d\nsplot 'terrain_data.txt' with pm3d\n";
        gp.close();
        GnuplotRenderer::executeScript("my_plot_script.gnuplot");
        log_mgr.log_system("3D Plot created: terrain_3d.png");
    }
};

class Plot2DScenario
{
public:
    static void execute(const LandscapeMap& map)
    {
        ofstream data("terrain_data_2d.txt");
        for (int y = 0; y < map.h; y++)
        {
            for (int x = 0; x < map.w; x++)
                data << x << " " << y << " " << map.data[y][x] << "\n";
            data << "\n";
        }
        data.close();
        ofstream gp("my_plot_2d.gnuplot");
        gp << "set terminal pngcairo size 900,900\nset output 'terrain_2d.png'\n";
        gp << "plot 'terrain_data_2d.txt' with image\n";
        gp.close();
        GnuplotRenderer::executeScript("my_plot_2d.gnuplot");
        log_mgr.log_system("2D Heatmap created: terrain_2d.png");
    }
};

class SlopeAnalysisScenario
{
public:
    static void execute(const LandscapeMap& map)
    {
        ofstream slope("gradient_vectors.txt");
        int step = 20;
        for (int y = step; y < map.h - step; y += step)
            for (int x = step; x < map.w - step; x += step)
            {
                auto grad = map.gradient(x, y);
                double ang = atan2(grad.second, grad.first);
                double mag = sqrt(grad.first * grad.first + grad.second * grad.second);
                slope << x << " " << y << " " << ang << " " << mag << "\n";
            }
        slope.close();
        log_mgr.log_system("SlopeAnalysisScenario completed");
    }
};

class SlopeCheckScenario
{
public:
    static void execute(const LandscapeMap& map, double threshold)
    {
        LandscapeMap check(map.w, map.h);
        for (int y = 1; y < map.h - 1; y++)
            for (int x = 1; x < map.w - 1; x++)
            {
                auto grad = map.gradient(x, y);
                double steep = sqrt(grad.first * grad.first + grad.second * grad.second);
                check.data[y][x] = (steep < threshold) ? 255 : 0;
            }
        check.saveBMP("steepness_map.bmp");
        log_mgr.log_system("Steepness map created (threshold = " + to_string(threshold) + ")");
    }
};

class ComponentSearchScenario
{
public:
    static void execute(const LandscapeMap& map, int k, int min_size)
    {
        Analysis::kmeans_cluster(map, k, min_size);
        log_mgr.log_system("ComponentSearchScenario completed (K=" + to_string(k) + ")");
    }
};

class GeometryScenario
{
public:
    static void execute(const LandscapeMap& map, int min_size)
    {
        auto comps = Scenarios::findConnectedComponents(map, min_size);
        vector<Point2D> centers;
        for (auto& comp : comps)
        {
            double sx = 0, sy = 0;
            for (auto& p : comp)
            {
                sx += p.first;
                sy += p.second;
            }
            centers.push_back({sx / comp.size(), sy / comp.size()});
        }
        if (centers.size() < 3)
        {
            log_mgr.log_system("Not enough centers for Delaunay");
            return;
        }
        Scenarios::geometryScenario(map, min_size);
        log_mgr.log_system("GeometryScenario completed (" + to_string(centers.size()) + " centers)");
    }
};

// ====================== Server ======================
class Server : public ILandscapeOperations
{
    vector<GaussianBell> bells;
    LandscapeMap* map = nullptr;
    Settings cfg;

public:
    Server()
    {
        cfg.load("config.txt");
    }
    ~Server()
    {
        if (map) delete map;
    }

    void init()
    {
        map = new LandscapeMap(cfg.width, cfg.height);
        log_mgr.log_system("Server initialized");
    }

    // Process a single command line in-process (sequential execution)
    void processLine(const string& line)
    {
        executeCommand(line);
    }

       // Реализация интерфейса ILandscapeOperations
    void addGauss(int sign, double cx, double cy, double sx, double sy, double rho) override
    {
        // ====================== ВАЛИДАЦИЯ ПАРАМЕТРОВ ======================
        string warnings = "";

        if (cx < 0 || cx >= map->width() || cy < 0 || cy >= map->height()) {
            warnings += "WARNING: Gaussian center out of bounds ("
                     + to_string(cx) + ", " + to_string(cy) + ") ";
        }
        if (sx <= 0.0 || sy <= 0.0) {
            warnings += "WARNING: Invalid sigma (sx or sy <= 0) ";
        }
        if (rho < -1.0 || rho > 1.0) {
            warnings += "WARNING: Rho out of range [-1.0, 1.0] ";
        }

        if (!warnings.empty()) {
            log_mgr.log_system(warnings);
        }
        // ================================================================

        bells.emplace_back(sign, cx, cy, sx, sy, rho);

        if (map) {
            map->add(bells.back());
        }

        if (warnings.empty()) {
            log_mgr.log_system("Added Gaussian bell at (" + to_string(cx) + ", " + to_string(cy) + ")");
        } else {
            log_mgr.log_system("Added Gaussian bell at (" + to_string(cx) + ", " + to_string(cy) + ") [with warnings]");
        }
    }

    void generate() override
    {
        if (map) FieldGenerationScenario::execute(*map, bells, cfg.noise_level);
    }
    void plot() override
    {
        if (map) Scenarios::plot3D(*map);
    }
    void plot2D() override
    {
        if (map) Scenarios::plot2D(*map);
    }
    void saveBMP(const string& fn = "") override
    {
        string filename = fn.empty() ? "my_landscape.bmp" : fn;
        if (map) map->saveBMP(filename);
    }
    void analiz() override
    {
        if (map) Scenarios::slopeAnalysis(*map);
    }
    void slopeCheck() override
    {
        if (map) Scenarios::slopeCheck(*map, cfg.slope_threshold);
    }
    void componentSearch() override
    {
        if (map) ComponentSearchScenario::execute(*map, min(cfg.kmeans_k, cfg.max_k), cfg.min_cluster_size);
    }
    void geometry() override
    {
        if (map) GeometryScenario::execute(*map, cfg.min_cluster_size);
    }

private:
    void executeCommand(const string& line)
    {
        istringstream iss(line);
        string token;
        iss >> token;
        if (token == "GAUSS")
        {
            int s;
            double cx, cy, sx, sy, rho;
            if (iss >> s >> cx >> cy >> sx >> sy >> rho)
                addGauss(s, cx, cy, sx, sy, rho);
        }
        else if (token == "GENERATE") generate();
        else if (token == "PLOT") plot();
        else if (token == "PLOT2D") plot2D();
        else if (token == "BMP_WRITE")
        {
            string fn;
            getline(iss >> ws, fn);
            saveBMP(fn.empty() ? "my_landscape.bmp" : fn);
        }
        else if (token == "ANALIZ") analiz();
        else if (token == "SLOPE_CHECK") slopeCheck();
        else if (token == "COMPONENT_SEARCH") componentSearch();
        else if (token == "EM_CLUSTER")
        {
            int k = cfg.kmeans_k;
            if (iss >> k) { /* parsed */ }
            if (map) Analysis::em_cluster(*map, k, cfg.min_cluster_size);
        }
        else if (token == "GEOMETRY") geometry();
        else log_mgr.log_system("Unknown command: " + line);
    }
};
// ====================== COMMAND FILE READER ======================
void executeCommandsFromFile(Server& server, const string& commandFile)
{
    namespace fs = std::filesystem;
    ifstream file(commandFile);
    if (!file.is_open())
    {
        // Try parent directory (useful when current path is 'output/')
        fs::path alt = fs::path("..") / commandFile;
        file.open(alt.string());
        if (!file.is_open())
        {
            log_mgr.log_user("ERROR: Cannot open command file: " + commandFile);
            return;
        }
    }

    log_mgr.log_user("Reading commands from: " + commandFile);

    string line;
    while (getline(file, line))
    {
        if (line.empty() || line[0] == '#' || line.find_first_not_of(" \t") == string::npos)
            continue;

        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        log_mgr.log_user("Executing: " + line);

        server.processLine(line);
    }

    file.close();
    log_mgr.log_user("All commands from file executed.");
}
// ====================== main ======================
int main(int argc, char* argv[])
{
    log_mgr.setup("app");
    log_mgr.log_user("Terrain generator started");

    Server server;
    server.init();

    // Ensure output directory exists and switch to it so all files are written there
    namespace fs = std::filesystem;
    fs::path outDir("output");
    if (!fs::exists(outDir))
    {
        try { fs::create_directory(outDir); }
        catch (const std::exception& ex) { log_mgr.log_system(string("ERROR: cannot create output dir: ") + ex.what()); }
    }
    try
    {
        fs::current_path(outDir);
        log_mgr.log_system(string("Output directory set to: ") + fs::current_path().string());
    }
    catch (const std::exception& ex)
    {
        log_mgr.log_system(string("ERROR: cannot set current path: ") + ex.what());
    }

    executeCommandsFromFile(server, "commands.txt");

    log_mgr.log_user("Execution completed successfully.");

    cout << "\n========================================\n";
    cout << "Program completed successfully!\n";
    cout << "Generated files (check output and logs):\n";
    cout << " - my_landscape.bmp\n";
    cout << " - terrain_3d.png\n";
    cout << " - terrain_2d.png\n";
    cout << " - steepness_map.bmp\n";
    cout << " - my_delaunay_voronoi.png\n";
    cout << "========================================\n";

    return 0;
}
