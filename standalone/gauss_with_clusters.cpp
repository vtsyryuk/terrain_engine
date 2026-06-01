// Standalone Code::Blocks version.
// Open this single file in Code::Blocks 20.03 and press Build/Run.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif

using namespace std;

struct Rgb
{
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
};

struct Point2D
{
    double x = 0.0;
    double y = 0.0;
};

struct Triangle
{
    int p1 = 0;
    int p2 = 0;
    int p3 = 0;
    Point2D center;
    double radius = 0.0;
};

struct Settings
{
    int width = 100;
    int height = 100;
    double noise_level = 0.0;

    void load(const string& path)
    {
        ifstream f(path);
        if (!f.is_open()) return;

        string line;
        while (getline(f, line))
        {
            if (line.empty() || line[0] == '#') continue;
            size_t eq = line.find('=');
            if (eq == string::npos) continue;

            string key = line.substr(0, eq);
            string val = line.substr(eq + 1);
            trim(key);
            trim(val);

            if (key == "WIDTH") width = stoi(val);
            else if (key == "HEIGHT") height = stoi(val);
            else if (key == "NOISE_LEVEL") noise_level = stod(val);
        }
    }

    static void trim(string& s)
    {
        size_t first = s.find_first_not_of(" \t\r\n");
        if (first == string::npos)
        {
            s.clear();
            return;
        }
        size_t last = s.find_last_not_of(" \t\r\n");
        s = s.substr(first, last - first + 1);
    }
};

struct GaussianBell
{
    int sign = 1;
    double cx = 0.0;
    double cy = 0.0;
    double sx = 1.0;
    double sy = 1.0;
    double rho = 0.0;

    double value(double x, double y) const
    {
        double dx = x - cx;
        double dy = y - cy;
        double nx = dx / sx;
        double ny = dy / sy;
        double denom = 1.0 - rho * rho;
        if (fabs(denom) < 1e-10) return 0.0;
        double q = nx * nx + ny * ny - 2.0 * rho * nx * ny;
        return sign * exp(-0.5 * q / denom);
    }
};

struct Logger
{
    ofstream system_log;
    ofstream user_log;

    void setup()
    {
        createDir("logs");
        system_log.open("logs/app_system.log", ios::app);
        user_log.open("logs/app_user.log", ios::app);
    }

    void system(const string& msg)
    {
        cout << "[SYSTEM] " << msg << "\n";
        if (system_log.is_open()) system_log << "[SYSTEM] " << msg << "\n";
    }

    void user(const string& msg)
    {
        cout << "[USER] " << msg << "\n";
        if (user_log.is_open()) user_log << "[USER] " << msg << "\n";
    }

    static void createDir(const char* name)
    {
#ifdef _WIN32
        _mkdir(name);
#else
        mkdir(name, 0755);
#endif
    }
};

Logger log_mgr;

static void writeRgbBMP(const string& filename, int width, int height, const vector<Rgb>& pixels)
{
    ofstream f(filename, ios::binary);
    if (!f.is_open())
    {
        log_mgr.system("ERROR: cannot open BMP file: " + filename);
        return;
    }

    const int rowBytes = width * 3;
    const int pad = (4 - (rowBytes % 4)) % 4;
    const uint32_t imageSize = static_cast<uint32_t>((rowBytes + pad) * height);
    const uint32_t fileSize = 54 + imageSize;

    uint8_t header[54] = {0x42, 0x4D};
    uint32_t offset = 54;
    uint32_t dibSize = 40;
    int32_t bw = width;
    int32_t bh = height;
    uint16_t planes = 1;
    uint16_t bpp = 24;

    memcpy(header + 2, &fileSize, 4);
    memcpy(header + 10, &offset, 4);
    memcpy(header + 14, &dibSize, 4);
    memcpy(header + 18, &bw, 4);
    memcpy(header + 22, &bh, 4);
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
        f.write(reinterpret_cast<const char*>(zero), pad);
    }

    log_mgr.system("BMP saved: " + filename);
}

static void setPixel(vector<Rgb>& pixels, int width, int height, int x, int y, Rgb color)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
        pixels[y * width + x] = color;
}

static void drawLine(vector<Rgb>& pixels, int width, int height, double ax, double ay, double bx, double by, Rgb color)
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

static Rgb colorForLabel(int label)
{
    static const Rgb colors[] = {
        {230, 25, 75}, {60, 180, 75}, {0, 130, 200}, {245, 130, 48},
        {145, 30, 180}, {70, 240, 240}, {240, 50, 230}, {210, 245, 60}
    };
    return colors[max(0, label) % (sizeof(colors) / sizeof(colors[0]))];
}

class LandscapeMap
{
public:
    int w;
    int h;
    vector<vector<double>> data;

    LandscapeMap(int width, int height)
        : w(width), h(height), data(height, vector<double>(width, 0.0))
    {
    }

    void clear()
    {
        for (auto& row : data)
            fill(row.begin(), row.end(), 0.0);
    }

    void normalize()
    {
        double mn = 1e100;
        double mx = -1e100;
        for (const auto& row : data)
            for (double v : row)
            {
                mn = min(mn, v);
                mx = max(mx, v);
            }

        double r = mx - mn;
        if (r < 1e-12) return;

        for (auto& row : data)
            for (double& v : row)
                v = (v - mn) / r * 255.0;
    }

    pair<double, double> gradient(int x, int y) const
    {
        if (x <= 0 || x >= w - 1 || y <= 0 || y >= h - 1)
            return {0.0, 0.0};
        double dx = (data[y][x + 1] - data[y][x - 1]) / 2.0;
        double dy = (data[y + 1][x] - data[y - 1][x]) / 2.0;
        return {dx, dy};
    }

    void saveBMP(const string& filename) const
    {
        ofstream f(filename, ios::binary);
        if (!f.is_open())
        {
            log_mgr.system("ERROR: cannot open BMP file: " + filename);
            return;
        }

        int pad = (4 - (w % 4)) % 4;
        uint32_t imageSize = static_cast<uint32_t>((w + pad) * h);
        uint32_t fileSize = 54 + 256 * 4 + imageSize;

        uint8_t header[54] = {0x42, 0x4D};
        uint32_t offset = 54 + 256 * 4;
        uint32_t dibSize = 40;
        int32_t bw = w;
        int32_t bh = h;
        uint16_t planes = 1;
        uint16_t bpp = 8;

        memcpy(header + 2, &fileSize, 4);
        memcpy(header + 10, &offset, 4);
        memcpy(header + 14, &dibSize, 4);
        memcpy(header + 18, &bw, 4);
        memcpy(header + 22, &bh, 4);
        memcpy(header + 26, &planes, 2);
        memcpy(header + 28, &bpp, 2);
        memcpy(header + 34, &imageSize, 4);

        f.write(reinterpret_cast<char*>(header), 54);
        for (int i = 0; i < 256; ++i)
        {
            uint8_t c = static_cast<uint8_t>(i);
            f.write(reinterpret_cast<char*>(&c), 1);
            f.write(reinterpret_cast<char*>(&c), 1);
            f.write(reinterpret_cast<char*>(&c), 1);
            f.put(0);
        }

        for (int y = h - 1; y >= 0; --y)
        {
            for (int x = 0; x < w; ++x)
            {
                uint8_t v = static_cast<uint8_t>(max(0.0, min(255.0, data[y][x])));
                f.write(reinterpret_cast<char*>(&v), 1);
            }
            for (int i = 0; i < pad; ++i) f.put(0);
        }

        log_mgr.system("BMP saved: " + filename);
    }
};

class TerrainApp
{
public:
    explicit TerrainApp(const Settings& settings)
        : cfg(settings), map(settings.width, settings.height)
    {
    }

    void addGauss(int sign, double cx, double cy, double sx, double sy, double rho)
    {
        if (sign == 0) sign = -1;
        if (rho <= -1.0 || rho >= 1.0)
        {
            log_mgr.system("WARNING: GAUSS rho out of range; using rho=0");
            rho = 0.0;
        }
        bells.push_back({sign, cx, cy, sx, sy, rho});
        log_mgr.system("Added Gaussian bell at (" + to_string(cx) + ", " + to_string(cy) + ")");
    }

    void scan()
    {
        writeRawField("output/field.dat");
        generate();
        log_mgr.system("SCAN completed");
    }

    void generate()
    {
        map.clear();
        for (const auto& bell : bells)
            for (int y = 0; y < map.h; ++y)
                for (int x = 0; x < map.w; ++x)
                    map.data[y][x] += bell.value(x, y);

        map.normalize();
        smooth();
        smooth();
        log_mgr.system("GENERATE completed");
    }

    void gnuplotFile(const string& name)
    {
        writeMapData(outputPath(name.empty() ? "field.dat" : name));
    }

    void saveBMP(const string& name)
    {
        map.saveBMP(outputPath(name.empty() ? "landscape.bmp" : name));
    }

    void plot(const string& fieldFile, const string& outputFile)
    {
        string dataFile = outputPath(fieldFile.empty() ? "field.dat" : fieldFile);
        string pngFile = outputPath(outputFile.empty() ? "terrain_3d.png" : outputFile);

        ofstream gp("output/seminar_plot.gnuplot");
        gp << "set terminal pngcairo size 1600,900\n";
        gp << "set output '" << pngFile << "'\n";
        gp << "unset title\n";
        gp << "set view 66,225\n";
        gp << "set xrange [0:" << cfg.width << "]\n";
        gp << "set yrange [0:" << cfg.height << "]\n";
        gp << "set zrange [-1.2:1.2]\n";
        gp << "set key right top\n";
        gp << "set hidden3d\n";
        gp << "splot '" << dataFile << "' with lines lc rgb '#aa00ff' title '" << dataFile << "'\n";
        gp.close();

#ifdef _WIN32
        string cmd = "gnuplot \"output/seminar_plot.gnuplot\" 2>NUL";
#else
        string cmd = "gnuplot \"output/seminar_plot.gnuplot\" 2>/dev/null";
#endif
        int code = std::system(cmd.c_str());
        if (code != 0) log_mgr.system("WARNING: gnuplot failed. Check that gnuplot is in PATH.");
        else log_mgr.system("Plot created: " + pngFile);
    }

    void slopeCheck(double threshold)
    {
        slopeCheck(threshold, "steepness_map.bmp");
    }

    void slopeCheck(double threshold, const string& outputFile)
    {
        LandscapeMap check(map.w, map.h);
        for (int y = 1; y < map.h - 1; ++y)
            for (int x = 1; x < map.w - 1; ++x)
            {
                auto g = map.gradient(x, y);
                double steep = sqrt(g.first * g.first + g.second * g.second);
                check.data[y][x] = steep < threshold ? 255.0 : 0.0;
            }
        check.saveBMP(outputPath(outputFile));
    }

    void trajectories(const string& outputFile)
    {
        slopeCheck(2.0, outputFile.empty() ? "trajectories.bmp" : outputFile);
    }

    void delaunay(const string& outputFile)
    {
        vector<Point2D> centers = componentCenters(2);
        if (centers.size() < 3)
        {
            log_mgr.system("Not enough centers for Delaunay");
            return;
        }

        vector<Triangle> triangles = buildDelaunay(centers);
        vector<Rgb> pixels(static_cast<size_t>(map.w * map.h), {255, 255, 255});
        for (const Triangle& t : triangles)
        {
            const Point2D& a = centers[t.p1];
            const Point2D& b = centers[t.p2];
            const Point2D& c = centers[t.p3];
            drawLine(pixels, map.w, map.h, a.x, a.y, b.x, b.y, {0, 0, 255});
            drawLine(pixels, map.w, map.h, b.x, b.y, c.x, c.y, {0, 0, 255});
            drawLine(pixels, map.w, map.h, c.x, c.y, a.x, a.y, {0, 0, 255});
        }

        for (size_t i = 0; i < triangles.size(); ++i)
            for (size_t j = i + 1; j < triangles.size(); ++j)
            {
                int shared = 0;
                if (triangles[i].p1 == triangles[j].p1 || triangles[i].p1 == triangles[j].p2 || triangles[i].p1 == triangles[j].p3) shared++;
                if (triangles[i].p2 == triangles[j].p1 || triangles[i].p2 == triangles[j].p2 || triangles[i].p2 == triangles[j].p3) shared++;
                if (triangles[i].p3 == triangles[j].p1 || triangles[i].p3 == triangles[j].p2 || triangles[i].p3 == triangles[j].p3) shared++;
                if (shared == 2)
                {
                    drawLine(
                        pixels, map.w, map.h,
                        triangles[i].center.x, triangles[i].center.y,
                        triangles[j].center.x, triangles[j].center.y,
                        {255, 0, 0}
                    );
                }
            }

        for (const Point2D& p : centers)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    setPixel(pixels, map.w, map.h, static_cast<int>(round(p.x)) + dx, static_cast<int>(round(p.y)) + dy, {0, 0, 0});

        writeRgbBMP(outputPath(outputFile.empty() ? "delaunay.bmp" : outputFile), map.w, map.h, pixels);
    }

    void kmeans(int k, const string& outputFile)
    {
        vector<int> labels = kmeansLabels(max(1, k));
        writeLabelBmp(labels, outputFile.empty() ? "landscape_kmeans.bmp" : outputFile);
    }

    void em(int k, const string& outputFile)
    {
        vector<int> labels = emLabels(max(1, k), 50);
        writeLabelBmp(labels, outputFile.empty() ? "field_em.bmp" : outputFile);
    }

private:
    Settings cfg;
    LandscapeMap map;
    vector<GaussianBell> bells;

    static string outputPath(const string& filename)
    {
        if (filename.rfind("output/", 0) == 0 || filename.rfind("output\\", 0) == 0)
            return filename;
        return "output/" + filename;
    }

    void writeRawField(const string& filename)
    {
        ofstream out(filename);
        for (int y = 0; y < cfg.height; ++y)
        {
            for (int x = 0; x < cfg.width; ++x)
            {
                double v = 0.0;
                for (const auto& bell : bells) v += bell.value(x, y);
                out << x << " " << y << " " << v << "\n";
            }
            out << "\n";
        }
        log_mgr.system("Raw field saved: " + filename);
    }

    void writeMapData(const string& filename)
    {
        ofstream out(filename);
        for (int y = 0; y < map.h; ++y)
        {
            for (int x = 0; x < map.w; ++x)
                out << x << " " << y << " " << map.data[y][x] << "\n";
            out << "\n";
        }
        log_mgr.system("Field data saved: " + filename);
    }

    vector<Point2D> componentCenters(int minSize) const
    {
        vector<Point2D> centers;
        vector<vector<bool>> visited(map.h, vector<bool>(map.w, false));
        const int dx[8] = {-1,-1,-1,0,0,1,1,1};
        const int dy[8] = {-1,0,1,-1,1,-1,0,1};

        for (int y = 0; y < map.h; ++y)
            for (int x = 0; x < map.w; ++x)
            {
                if (map.data[y][x] <= 200.0 || visited[y][x]) continue;

                vector<pair<int,int>> component;
                queue<pair<int,int>> q;
                q.push({x, y});
                visited[y][x] = true;

                while (!q.empty())
                {
                    pair<int,int> p = q.front();
                    q.pop();
                    component.push_back(p);
                    for (int d = 0; d < 8; ++d)
                    {
                        int nx = p.first + dx[d];
                        int ny = p.second + dy[d];
                        if (nx >= 0 && nx < map.w && ny >= 0 && ny < map.h && !visited[ny][nx] && map.data[ny][nx] > 200.0)
                        {
                            visited[ny][nx] = true;
                            q.push({nx, ny});
                        }
                    }
                }

                if (component.size() >= static_cast<size_t>(minSize))
                {
                    double sx = 0.0;
                    double sy = 0.0;
                    for (const auto& p : component)
                    {
                        sx += p.first;
                        sy += p.second;
                    }
                    centers.push_back({sx / component.size(), sy / component.size()});
                }
            }

        return centers;
    }

    static double distance(Point2D a, Point2D b)
    {
        const double dx = a.x - b.x;
        const double dy = a.y - b.y;
        return sqrt(dx * dx + dy * dy);
    }

    static Triangle makeTriangle(int a, int b, int c, const vector<Point2D>& points)
    {
        Triangle t;
        t.p1 = a;
        t.p2 = b;
        t.p3 = c;
        const double x1 = points[a].x, y1 = points[a].y;
        const double x2 = points[b].x, y2 = points[b].y;
        const double x3 = points[c].x, y3 = points[c].y;
        const double d = 2.0 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
        if (fabs(d) < 1e-9)
        {
            t.center = {0, 0};
            t.radius = 1e9;
            return t;
        }

        t.center.x = ((x1*x1 + y1*y1) * (y2-y3) + (x2*x2 + y2*y2) * (y3-y1) + (x3*x3 + y3*y3) * (y1-y2)) / d;
        t.center.y = ((x1*x1 + y1*y1) * (x3-x2) + (x2*x2 + y2*y2) * (x1-x3) + (x3*x3 + y3*y3) * (x2-x1)) / d;
        t.radius = distance(points[a], t.center);
        return t;
    }

    static vector<Triangle> buildDelaunay(const vector<Point2D>& points)
    {
        vector<Triangle> triangles;
        for (int i = 0; i < static_cast<int>(points.size()); ++i)
            for (int j = i + 1; j < static_cast<int>(points.size()); ++j)
                for (int k = j + 1; k < static_cast<int>(points.size()); ++k)
                {
                    Triangle t = makeTriangle(i, j, k, points);
                    bool ok = true;
                    for (int m = 0; m < static_cast<int>(points.size()); ++m)
                    {
                        if (m == i || m == j || m == k) continue;
                        if (distance(points[m], t.center) < t.radius - 1e-9)
                        {
                            ok = false;
                            break;
                        }
                    }
                    if (ok) triangles.push_back(t);
                }
        log_mgr.system("Delaunay triangles built: " + to_string(triangles.size()));
        return triangles;
    }

    vector<pair<int,int>> brightPoints() const
    {
        vector<pair<int,int>> points;
        for (int y = 0; y < map.h; ++y)
            for (int x = 0; x < map.w; ++x)
                if (map.data[y][x] >= 128.0)
                    points.push_back({x, y});
        return points;
    }

    vector<int> kmeansLabels(int k) const
    {
        vector<pair<int,int>> points = brightPoints();
        vector<int> labels(static_cast<size_t>(map.w * map.h), -1);
        if (points.empty()) return labels;
        k = min(k, static_cast<int>(points.size()));

        vector<Point2D> centroids(k);
        for (int i = 0; i < k; ++i)
            centroids[i] = {static_cast<double>(points[(i * points.size()) / k].first), static_cast<double>(points[(i * points.size()) / k].second)};

        for (int iter = 0; iter < 100; ++iter)
        {
            vector<double> sumX(k, 0.0), sumY(k, 0.0);
            vector<int> count(k, 0);
            bool changed = false;

            for (const auto& p : points)
            {
                int best = 0;
                double bestDist = 1e100;
                for (int c = 0; c < k; ++c)
                {
                    double dx = p.first - centroids[c].x;
                    double dy = p.second - centroids[c].y;
                    double dist = dx * dx + dy * dy;
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        best = c;
                    }
                }

                int idx = p.second * map.w + p.first;
                if (labels[idx] != best)
                {
                    labels[idx] = best;
                    changed = true;
                }
                sumX[best] += p.first;
                sumY[best] += p.second;
                count[best]++;
            }

            for (int c = 0; c < k; ++c)
                if (count[c] > 0)
                    centroids[c] = {sumX[c] / count[c], sumY[c] / count[c]};

            if (!changed) break;
        }

        return labels;
    }

    vector<int> emLabels(int k, int maxIter) const
    {
        vector<pair<int,int>> points = brightPoints();
        vector<int> labels(static_cast<size_t>(map.w * map.h), -1);
        if (points.empty()) return labels;
        k = min(k, static_cast<int>(points.size()));

        vector<Point2D> mu(k);
        vector<double> weights(k, 1.0 / k);
        vector<double> varX(k, 100.0), varY(k, 100.0);
        vector<vector<double>> resp(points.size(), vector<double>(k, 0.0));
        const double pi = 3.14159265358979323846;

        for (int i = 0; i < k; ++i)
            mu[i] = {static_cast<double>(points[(i * points.size()) / k].first), static_cast<double>(points[(i * points.size()) / k].second)};

        for (int iter = 0; iter < maxIter; ++iter)
        {
            for (size_t i = 0; i < points.size(); ++i)
            {
                double total = 0.0;
                for (int c = 0; c < k; ++c)
                {
                    double dx = points[i].first - mu[c].x;
                    double dy = points[i].second - mu[c].y;
                    double denom = 2.0 * pi * sqrt(varX[c] * varY[c]);
                    double value = exp(-0.5 * (dx * dx / varX[c] + dy * dy / varY[c])) / max(denom, 1e-12);
                    resp[i][c] = weights[c] * value;
                    total += resp[i][c];
                }
                total = max(total, 1e-12);
                for (int c = 0; c < k; ++c) resp[i][c] /= total;
            }

            for (int c = 0; c < k; ++c)
            {
                double nk = 0.0, sx = 0.0, sy = 0.0;
                for (size_t i = 0; i < points.size(); ++i)
                {
                    nk += resp[i][c];
                    sx += resp[i][c] * points[i].first;
                    sy += resp[i][c] * points[i].second;
                }
                if (nk <= 1e-12) continue;
                mu[c] = {sx / nk, sy / nk};

                double vx = 0.0, vy = 0.0;
                for (size_t i = 0; i < points.size(); ++i)
                {
                    double dx = points[i].first - mu[c].x;
                    double dy = points[i].second - mu[c].y;
                    vx += resp[i][c] * dx * dx;
                    vy += resp[i][c] * dy * dy;
                }
                varX[c] = max(1e-3, vx / nk);
                varY[c] = max(1e-3, vy / nk);
                weights[c] = nk / points.size();
            }
        }

        for (size_t i = 0; i < points.size(); ++i)
        {
            int best = 0;
            double bestValue = resp[i][0];
            for (int c = 1; c < k; ++c)
                if (resp[i][c] > bestValue)
                {
                    bestValue = resp[i][c];
                    best = c;
                }
            labels[points[i].second * map.w + points[i].first] = best;
        }

        return labels;
    }

    void writeLabelBmp(const vector<int>& labels, const string& outputFile) const
    {
        vector<Rgb> pixels(static_cast<size_t>(map.w * map.h), {255, 255, 255});
        for (int y = 0; y < map.h; ++y)
            for (int x = 0; x < map.w; ++x)
            {
                int idx = y * map.w + x;
                if (labels[idx] >= 0)
                    pixels[idx] = colorForLabel(labels[idx]);
                else
                {
                    uint8_t g = static_cast<uint8_t>(max(0.0, min(255.0, map.data[y][x])));
                    pixels[idx] = {g, g, g};
                }
            }
        writeRgbBMP(outputPath(outputFile), map.w, map.h, pixels);
    }

    void smooth()
    {
        auto copy = map.data;
        double kernel[3][3] = {{1,2,1}, {2,4,2}, {1,2,1}};
        for (int y = 1; y < map.h - 1; ++y)
            for (int x = 1; x < map.w - 1; ++x)
            {
                double sum = 0.0;
                double wsum = 0.0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                    {
                        double w = kernel[dy + 1][dx + 1];
                        sum += copy[y + dy][x + dx] * w;
                        wsum += w;
                    }
                map.data[y][x] = sum / wsum;
            }
    }
};

static string readOutputArgument(istream& input, const string& defaultName)
{
    vector<string> tokens;
    string token;
    while (input >> token)
        tokens.push_back(token);

    if (tokens.empty()) return defaultName;

    for (size_t i = 0; i + 1 < tokens.size(); ++i)
        if (tokens[i] == "to")
            return tokens[i + 1];

    return tokens.back();
}

string executeCommand(TerrainApp& app, const string& line)
{
    string cleaned = line;
    Settings::trim(cleaned);
    if (cleaned.empty() || cleaned[0] == '#') return "SKIPPED";

    log_mgr.user("Executing: " + cleaned);
    istringstream iss(cleaned);
    string cmd;
    iss >> cmd;

    if (cmd == "GAUSS")
    {
        int s;
        double cx, cy, sx, sy, rho;
        if (iss >> s >> cx >> cy >> sx >> sy >> rho)
        {
            app.addGauss(s, cx, cy, sx, sy, rho);
            return "OK";
        }
        return "ERROR: invalid GAUSS command";
    }
    else if (cmd == "SCAN")
    {
        app.scan();
    }
    else if (cmd == "GENERATE")
    {
        app.generate();
    }
    else if (cmd == "GNUPLOT_FILE")
    {
        app.gnuplotFile(readOutputArgument(iss, "field.dat"));
    }
    else if (cmd == "PLOT")
    {
        string mode, fieldFile, outputFile;
        iss >> mode >> fieldFile >> outputFile;
        if (fieldFile == "to")
            app.gnuplotFile(outputFile.empty() ? "field.dat" : outputFile);
        else
            app.plot(fieldFile, outputFile);
    }
    else if (cmd == "BMP_WRITE")
    {
        app.saveBMP(readOutputArgument(iss, "landscape.bmp"));
    }
    else if (cmd == "SLOPE_CHECK")
    {
        double threshold = 2.0;
        iss >> threshold;
        app.slopeCheck(threshold);
    }
    else if (cmd == "TRAJECTORIES")
    {
        app.trajectories(readOutputArgument(iss, "trajectories.bmp"));
    }
    else if (cmd == "DELONE" || cmd == "DELAUNAY")
    {
        app.delaunay(readOutputArgument(iss, "delaunay.bmp"));
    }
    else if (cmd == "KMEANS")
    {
        int k = 2;
        iss >> k;
        app.kmeans(k, readOutputArgument(iss, "landscape_kmeans.bmp"));
    }
    else if (cmd == "EM")
    {
        int k = 3;
        iss >> k;
        app.em(k, readOutputArgument(iss, "field_em.bmp"));
    }
    else if (cmd == "EXIT")
    {
        return "EXIT";
    }
    else if (cmd == "SHUTDOWN")
    {
        return "SHUTDOWN";
    }
    else
    {
        log_mgr.system("Command skipped in standalone build: " + cmd);
        return "ERROR: unknown command " + cmd;
    }

    return "OK";
}

void executeCommands(TerrainApp& app, const string& commandFile)
{
    ifstream file(commandFile);
    if (!file.is_open())
    {
        log_mgr.user("ERROR: Cannot open command file: " + commandFile);
        return;
    }

    string line;
    while (getline(file, line))
    {
        string result = executeCommand(app, line);
        if (result == "EXIT" || result == "SHUTDOWN")
            break;
    }
}

string executeCommandBlock(TerrainApp& app, const string& commands)
{
    istringstream input(commands);
    string line;
    string last = "OK";

    while (getline(input, line))
    {
        last = executeCommand(app, line);
        if (last == "EXIT" || last == "SHUTDOWN")
            break;
        if (last.rfind("ERROR:", 0) == 0)
            return last;
    }

    return last == "EXIT" ? "OK" : last;
}

struct AppOptions
{
#ifdef _WIN32
    string mode = "batch";
#else
    string mode = "batch";
#endif
    string commandFile = "seminar1_commands.txt";
    string configFile = "seminar_config.txt";
    string pipeName = R"(\\.\pipe\TerrainPipe)";
    bool shutdown = false;
    bool shutdownOnly = false;
};

AppOptions parseOptions(int argc, char* argv[])
{
    AppOptions options;

    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "--server" || arg == "server")
        {
            options.mode = "server";
        }
        else if (arg == "--client" || arg == "client")
        {
            options.mode = "client";
        }
        else if (arg == "--batch" || arg == "batch")
        {
            options.mode = "batch";
        }
        else if (arg == "--shutdown")
        {
            options.shutdown = true;
        }
        else if (arg == "--shutdown-only")
        {
            options.shutdownOnly = true;
            options.mode = "client";
        }
        else if (arg == "--config" && i + 1 < argc)
        {
            options.configFile = argv[++i];
        }
        else if (arg == "--pipe" && i + 1 < argc)
        {
            options.pipeName = argv[++i];
        }
        else
        {
            options.commandFile = arg;
        }
    }

    return options;
}

#ifdef _WIN32
string pipeRequest(const string& pipeName, const string& command)
{
    HANDLE pipe = CreateFileA(
        pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (pipe == INVALID_HANDLE_VALUE)
    {
        return "ERROR: cannot connect to pipe " + pipeName;
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(pipe, &mode, NULL, NULL);

    DWORD written = 0;
    string payload = command + "\n";
    BOOL ok = WriteFile(pipe, payload.c_str(), static_cast<DWORD>(payload.size()), &written, NULL);
    if (!ok)
    {
        CloseHandle(pipe);
        return "ERROR: cannot write to pipe";
    }

    char buffer[4096] = {};
    DWORD read = 0;
    ok = ReadFile(pipe, buffer, sizeof(buffer) - 1, &read, NULL);
    CloseHandle(pipe);

    if (!ok && GetLastError() != ERROR_MORE_DATA)
    {
        return "ERROR: cannot read from pipe";
    }

    return string(buffer, read);
}

void runServer(const AppOptions& options)
{
    Settings cfg;
    cfg.load(options.configFile);

    cout << "[INFO] Standalone server is listening on " << options.pipeName << "\n";

    bool running = true;
    while (running)
    {
        HANDLE pipe = CreateNamedPipeA(
            options.pipeName.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096,
            4096,
            0,
            NULL
        );

        if (pipe == INVALID_HANDLE_VALUE)
        {
            cout << "[ERROR] CreateNamedPipe failed: " << GetLastError() << "\n";
            return;
        }

        BOOL connected = ConnectNamedPipe(pipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected)
        {
            CloseHandle(pipe);
            continue;
        }

        char buffer[65536] = {};
        DWORD read = 0;
        string response = "OK";
        if (ReadFile(pipe, buffer, sizeof(buffer) - 1, &read, NULL))
        {
            string command(buffer, read);
            Settings::trim(command);
            if (command == "SHUTDOWN")
            {
                response = "SHUTDOWN";
                running = false;
            }
            else if (command.rfind("BATCH\n", 0) == 0 || command.rfind("BATCH\r\n", 0) == 0)
            {
                size_t pos = command.find('\n');
                TerrainApp sessionApp(cfg);
                response = executeCommandBlock(sessionApp, command.substr(pos + 1));
            }
            else
            {
                TerrainApp sessionApp(cfg);
                response = executeCommand(sessionApp, command);
                if (response == "SHUTDOWN")
                {
                    running = false;
                }
            }
        }
        else
        {
            response = "ERROR: cannot read command";
        }

        DWORD written = 0;
        WriteFile(pipe, response.c_str(), static_cast<DWORD>(response.size()), &written, NULL);
        FlushFileBuffers(pipe);
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }

    cout << "[INFO] Server stopped\n";
}

void runClient(const AppOptions& options)
{
    if (options.shutdownOnly)
    {
        cout << "[CLIENT] SHUTDOWN\n";
        cout << "[SERVER] " << pipeRequest(options.pipeName, "SHUTDOWN") << "\n";
        return;
    }

    ifstream file(options.commandFile);
    if (!file.is_open())
    {
        cout << "[ERROR] Cannot open command file: " << options.commandFile << "\n";
        return;
    }

    ostringstream batch;
    batch << "BATCH\n";

    string line;
    while (getline(file, line))
    {
        Settings::trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line == "EXIT") break;
        batch << line << "\n";
    }

    cout << "[CLIENT] Sending batch: " << options.commandFile << "\n";
    cout << "[SERVER] " << pipeRequest(options.pipeName, batch.str()) << "\n";

    if (options.shutdown)
    {
        cout << "[CLIENT] SHUTDOWN\n";
        cout << "[SERVER] " << pipeRequest(options.pipeName, "SHUTDOWN") << "\n";
    }
}
#else
void runServer(const AppOptions&)
{
    cout << "[ERROR] Windows Named Pipes are available only on Windows\n";
}

void runClient(const AppOptions&)
{
    cout << "[ERROR] Windows Named Pipes are available only on Windows\n";
}
#endif

int main(int argc, char* argv[])
{
    Logger::createDir("output");
    log_mgr.setup();

    AppOptions options = parseOptions(argc, argv);

    if (options.mode == "server")
    {
        runServer(options);
        return 0;
    }

    if (options.mode == "client")
    {
        runClient(options);
        return 0;
    }

    Settings cfg;
    cfg.load(options.configFile);

    TerrainApp app(cfg);
    executeCommands(app, options.commandFile);

    cout << "\nDone. Check output/ folder.\n";
    return 0;
}
