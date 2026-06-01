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
#else
#include <sys/stat.h>
#endif

using namespace std;

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
        LandscapeMap check(map.w, map.h);
        for (int y = 1; y < map.h - 1; ++y)
            for (int x = 1; x < map.w - 1; ++x)
            {
                auto g = map.gradient(x, y);
                double steep = sqrt(g.first * g.first + g.second * g.second);
                check.data[y][x] = steep < threshold ? 255.0 : 0.0;
            }
        check.saveBMP("output/steepness_map.bmp");
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
        Settings::trim(line);
        if (line.empty() || line[0] == '#') continue;

        log_mgr.user("Executing: " + line);
        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "GAUSS")
        {
            int s;
            double cx, cy, sx, sy, rho;
            if (iss >> s >> cx >> cy >> sx >> sy >> rho)
                app.addGauss(s, cx, cy, sx, sy, rho);
        }
        else if (cmd == "SCAN" || cmd == "GENERATE")
        {
            app.scan();
        }
        else if (cmd == "PLOT")
        {
            string mode, fieldFile, outputFile;
            iss >> mode >> fieldFile >> outputFile;
            app.plot(fieldFile, outputFile);
        }
        else if (cmd == "BMP_WRITE")
        {
            string name;
            iss >> name;
            app.saveBMP(name);
        }
        else if (cmd == "SLOPE_CHECK")
        {
            double threshold = 2.0;
            iss >> threshold;
            app.slopeCheck(threshold);
        }
        else
        {
            log_mgr.system("Command skipped in standalone build: " + cmd);
        }
    }
}

int main(int argc, char* argv[])
{
    Logger::createDir("output");
    log_mgr.setup();

    string commandFile = "commands.txt";
    if (argc > 1) commandFile = argv[1];

    string configFile = "config_codeblocks.txt";
    if (argc > 2) configFile = argv[2];

    Settings cfg;
    cfg.load(configFile);

    TerrainApp app(cfg);
    executeCommands(app, commandFile);

    cout << "\nDone. Check output/ folder.\n";
    return 0;
}
