#include "Server.h"

#include "Analysis.h"
#include "ClusterVisualizer.h"
#include "LogManager.h"
#include "Scenarios.h"
#include "GnuplotRenderer.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
std::string makeOutputPath(const std::string& filename)
{
    if (filename.rfind("output/", 0) == 0 || filename.rfind("output\\", 0) == 0)
    {
        return filename;
    }
    std::filesystem::path path = "output";
    path /= filename;
    return path.string();
}

std::string readOutputArgument(std::istream& input, const std::string& defaultName)
{
    std::vector<std::string> tokens;
    std::string token;
    while (input >> token)
    {
        tokens.push_back(token);
    }

    if (tokens.empty())
    {
        return defaultName;
    }

    for (std::size_t i = 0; i + 1 < tokens.size(); ++i)
    {
        if (tokens[i] == "to")
        {
            return tokens[i + 1];
        }
    }

    return tokens.back();
}

void writeClusterData(const TerrainEngine& engine)
{
    Scenarios::writeImageData(engine.map(), makeOutputPath("terrain_data_2d.txt"));
}

void plotFieldData(const std::string& fieldFile, const std::string& outputFile)
{
    std::filesystem::path dataPath = "output";
    dataPath /= fieldFile.empty() ? "field.dat" : fieldFile;

    std::filesystem::path outputPath = "output";
    outputPath /= outputFile.empty() ? "terrain_3d.png" : outputFile;

    std::ofstream gp("output/seminar_plot.gnuplot");
    gp << "set terminal pngcairo size 1600,900\n";
    gp << "set output '" << outputPath.string() << "'\n";
    gp << "unset title\n";
    gp << "set view 66,225\n";
    gp << "set xrange [0:100]\n";
    gp << "set yrange [0:100]\n";
    gp << "set zrange [-1.2:1.2]\n";
    gp << "set key right top\n";
    gp << "set hidden3d\n";
    gp << "splot '" << dataPath.string() << "' with lines lc rgb '#aa00ff' title '" << dataPath.string() << "'\n";
    gp.close();

    GnuplotRenderer::executeScript("output/seminar_plot.gnuplot");
    log_mgr.log_system("Seminar wireframe plot created: " + outputPath.string());
}
}

Config Server::loadConfig(const std::string& configFile)
{
    Config cfg;
    cfg.load(configFile);
    return cfg;
}

Server::Server()
    : Server("config.txt")
{
}

Server::Server(const std::string& configFile)
    : cfg_(loadConfig(configFile)),
      engine_(cfg_.width, cfg_.height)
{
    engine_.setNoisePercent(cfg_.noiseLevel);
}

void Server::init()
{
    log_mgr.log_system("Grid initialized " + std::to_string(cfg_.width) + "x" + std::to_string(cfg_.height));
    log_mgr.log_system("Server initialized");
}

std::string Server::processLine(const std::string& line)
{
    return executeCommand(line);
}

void Server::addGauss(int sign, double cx, double cy, double sx, double sy, double rho)
{
    std::string warnings;

    if (cx < 0 || cx >= cfg_.width || cy < 0 || cy >= cfg_.height)
    {
        warnings += "WARNING: Gaussian center out of bounds ("
            + std::to_string(cx) + ", " + std::to_string(cy) + ") ";
    }
    if (sx <= 0.0 || sy <= 0.0)
    {
        warnings += "WARNING: Invalid sigma (sx or sy <= 0) ";
    }
    if (rho <= -1.0 || rho >= 1.0)
    {
        warnings += "WARNING: Rho out of range (-1.0, 1.0); using rho=0 ";
        rho = 0.0;
    }

    if (!warnings.empty())
    {
        log_mgr.log_system(warnings);
    }

    engine_.addGaussian({sign, cx, cy, sx, sy, rho});

    std::string message = "Added Gaussian bell at ("
        + std::to_string(cx) + ", " + std::to_string(cy) + ")";
    if (!warnings.empty())
    {
        message += " [with warnings]";
    }
    log_mgr.log_system(message);
}

void Server::generate()
{
    engine_.generate();
}

void Server::plot()
{
    Scenarios::plot3D(engine_.map());
}

void Server::plot2D()
{
    Scenarios::plot2D(engine_.map());
}

void Server::saveBMP(const std::string& filename)
{
    const std::string name = filename.empty() ? "my_landscape.bmp" : filename;
    engine_.saveBMP(makeOutputPath(name));
    log_mgr.log_system("BMP saved: " + makeOutputPath(name));
}

void Server::analiz()
{
    Scenarios::slopeAnalysis(engine_.map());
}

void Server::slopeCheck()
{
    Scenarios::slopeCheck(engine_.map(), cfg_.slopeThreshold);
}

void Server::componentSearch()
{
    Scenarios::componentSearch(
        engine_.map(),
        std::min(cfg_.kmeansK, cfg_.maxK),
        cfg_.componentMinSize
    );
}

void Server::geometry()
{
    Scenarios::geometryScenario(engine_.map(), cfg_.componentMinSize);
}

std::string Server::executeCommand(const std::string& line)
{
    std::istringstream iss(line);
    std::string token;
    iss >> token;

    if (token.empty() || token[0] == '#')
    {
        return "SKIPPED";
    }

    if (token == "GAUSS")
    {
        int s;
        double cx, cy, sx, sy, rho;
        if (!(iss >> s >> cx >> cy >> sx >> sy >> rho))
        {
            throw std::runtime_error("Invalid GAUSS command");
        }
        if (s == 0)
        {
            s = -1;
        }
        addGauss(s, cx, cy, sx, sy, rho);
    }
    else if (token == "GENERATE") generate();
    else if (token == "EXIT") return "EXIT";
    else if (token == "SCAN")
    {
        engine_.saveRawTerrainData(makeOutputPath("field.dat"), 1);
        log_mgr.log_system("Raw field data saved: output/field.dat");
        generate();
    }
    else if (token == "GNUPLOT_FILE")
    {
        std::string outputFile = readOutputArgument(iss, "field.dat");
        engine_.saveRawTerrainData(makeOutputPath(outputFile), 1);
        log_mgr.log_system("Gnuplot field data saved: " + makeOutputPath(outputFile));
    }
    else if (token == "PLOT")
    {
        std::string plotMode;
        std::string fieldFile;
        std::string outputFile;
        iss >> plotMode >> fieldFile >> outputFile;
        if (fieldFile == "to")
        {
            const std::string name = outputFile.empty() ? "field.dat" : outputFile;
            engine_.saveRawTerrainData(makeOutputPath(name), 1);
            log_mgr.log_system("Gnuplot field data saved: " + makeOutputPath(name));
        }
        else if (!fieldFile.empty())
        {
            plotFieldData(fieldFile, outputFile);
        }
        else
        {
            plot();
        }
    }
    else if (token == "PLOT2D") plot2D();
    else if (token == "BMP_WRITE")
    {
        std::string fn = readOutputArgument(iss, "my_landscape.bmp");
        saveBMP(fn.empty() ? "my_landscape.bmp" : fn);
    }
    else if (token == "ANALIZ") analiz();
    else if (token == "SLOPE_CHECK") slopeCheck();
    else if (token == "TRAJECTORIES")
    {
        const std::string outputFile = readOutputArgument(iss, "trajectories.bmp");
        Scenarios::slopeCheck(engine_.map(), cfg_.slopeThreshold, makeOutputPath(outputFile));
        log_mgr.log_system("Trajectories map saved: " + makeOutputPath(outputFile));
    }
    else if (token == "COMPONENT_SEARCH") componentSearch();
    else if (token == "EM_CLUSTER")
    {
        int k = cfg_.emK;
        iss >> k;
        Analysis::em_cluster(engine_.map(), k, cfg_.componentMinSize, 50);
        ClusterVisualizer::visualize(makeOutputPath("em_overlay.png"));
        log_mgr.log_system("EM overlay saved: output/em_overlay.png");
    }
    else if (token == "KMEANS")
    {
        int k = std::min(cfg_.kmeansK, cfg_.maxK);
        iss >> k;
        const std::string outputFile = readOutputArgument(iss, "landscape_kmeans.bmp");
        writeClusterData(engine_);
        Analysis::kmeans_cluster(engine_.map(), k, cfg_.componentMinSize);
        ClusterVisualizer::visualizeLabels(makeOutputPath("kmeans.txt"), makeOutputPath(outputFile));
        log_mgr.log_system("K-means visualization saved: " + makeOutputPath(outputFile));
    }
    else if (token == "EM")
    {
        int k = cfg_.emK;
        iss >> k;
        const std::string outputFile = readOutputArgument(iss, "field_em.bmp");
        writeClusterData(engine_);
        Analysis::em_cluster(engine_.map(), k, cfg_.componentMinSize, 50);
        ClusterVisualizer::visualize(makeOutputPath(outputFile));
        log_mgr.log_system("EM visualization saved: " + makeOutputPath(outputFile));
    }
    else if (token == "GEOMETRY")
    {
        int minSize = cfg_.componentMinSize;
        iss >> minSize;
        const std::string outputFile = readOutputArgument(iss, "my_delaunay_voronoi.png");
        Scenarios::geometryScenario(engine_.map(), minSize, makeOutputPath(outputFile));
    }
    else if (token == "DELONE" || token == "DELAUNAY")
    {
        const std::string outputFile = readOutputArgument(iss, "my_delaunay_voronoi.bmp");
        Scenarios::geometryScenario(engine_.map(), cfg_.componentMinSize, makeOutputPath(outputFile));
    }
    else
    {
        log_mgr.log_system("Unknown command: " + line);
        throw std::runtime_error("Unknown command: " + token);
    }

    return "OK";
}

void executeCommandsFromFile(Server& server, const std::string& commandFile)
{
    std::ifstream file(commandFile);
    if (!file.is_open())
    {
        std::filesystem::path alt = std::filesystem::path("..") / commandFile;
        file.open(alt.string());
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open command file: " + commandFile);
        }
    }

    log_mgr.log_user("Reading commands from: " + commandFile);

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#' || line.find_first_not_of(" \t") == std::string::npos)
        {
            continue;
        }

        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        log_mgr.log_user("Executing: " + line);
        if (server.processLine(line) == "EXIT")
        {
            break;
        }
    }

    log_mgr.log_user("All commands from file executed.");
}
