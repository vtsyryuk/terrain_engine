#include "Server.h"

#include "Analysis.h"
#include "ClusterVisualizer.h"
#include "LogManager.h"
#include "Scenarios.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{
std::string makeOutputPath(const std::string& filename)
{
    std::filesystem::path path = "output";
    path /= filename;
    return path.string();
}
}

Config Server::loadConfig()
{
    Config cfg;
    cfg.load("config.txt");
    return cfg;
}

Server::Server()
    : cfg_(loadConfig()),
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
        warnings += "WARNING: Rho out of range (-1.0, 1.0) ";
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
        addGauss(s, cx, cy, sx, sy, rho);
    }
    else if (token == "GENERATE") generate();
    else if (token == "PLOT") plot();
    else if (token == "PLOT2D") plot2D();
    else if (token == "BMP_WRITE")
    {
        std::string fn;
        std::getline(iss >> std::ws, fn);
        saveBMP(fn.empty() ? "my_landscape.bmp" : fn);
    }
    else if (token == "ANALIZ") analiz();
    else if (token == "SLOPE_CHECK") slopeCheck();
    else if (token == "COMPONENT_SEARCH") componentSearch();
    else if (token == "EM_CLUSTER")
    {
        int k = cfg_.emK;
        iss >> k;
        Analysis::em_cluster(engine_.map(), k, cfg_.componentMinSize, 50);
        ClusterVisualizer::visualize(makeOutputPath("em_overlay.png"));
        log_mgr.log_system("EM overlay saved: output/em_overlay.png");
    }
    else if (token == "GEOMETRY") geometry();
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
        server.processLine(line);
    }

    log_mgr.log_user("All commands from file executed.");
}
