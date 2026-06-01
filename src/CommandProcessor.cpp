
#include "CommandProcessor.h"
#include "TerrainEngine.h"
#include "Config.h"
#include "Scenarios.h"
#include "ClusterVisualizer.h"
#include "Analysis.h"
#include "Logger.h"
#include "GnuplotRenderer.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <algorithm>

static std::string makeOutputPath(const std::string& filename)
{
    std::filesystem::path path = "output";
    path /= filename;
    return path.string();
}

static void plotFieldData(const std::string& fieldFile, const std::string& outputFile)
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
    Logger::info("Seminar wireframe plot created: " + outputPath.string());
}

CommandProcessor::CommandProcessor(
    TerrainEngine& engine
)
    : engine_(engine)
{
}

CommandProcessor::CommandProcessor(
    TerrainEngine& engine,
    const Config& config
)
    : engine_(engine),
      slopeThreshold_(config.slopeThreshold),
      kmeansK_(std::min(config.kmeansK, config.maxK)),
      componentMinSize_(config.componentMinSize),
      emK_(config.emK)
{
}

void CommandProcessor::executeFile(
    const std::string& filename
)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Cannot open commands file"
        );
    }

    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        executeLine(line);
    }
}

std::string CommandProcessor::executeLine(
    const std::string& line
)
{
    std::stringstream ss(line);
    std::string command;
    ss >> command;

    if (command.empty() || command[0] == '#')
    {
        return "SKIPPED";
    }

    if (command == "GAUSS")
    {
        GaussianBell g{};
        if (!(ss >> g.sign >> g.cx >> g.cy >> g.sx >> g.sy >> g.rho))
        {
            throw std::runtime_error("Invalid GAUSS command");
        }
        engine_.addGaussian(g);
        Logger::info(
            "Added Gaussian bell at (" + std::to_string(g.cx) + ", " + std::to_string(g.cy) + ")"
        );
    }
    else if (command == "GENERATE")
    {
        engine_.generate();
    }
    else if (command == "SCAN")
    {
        engine_.saveRawTerrainData(makeOutputPath("field.dat"), 1);
        Logger::info("Raw field data saved: output/field.dat");
        engine_.generate();
    }
    else if (command == "BMP_WRITE")
    {
        std::string name;
        ss >> name;
        if (name.empty())
        {
            name = "my_landscape.bmp";
        }
        engine_.saveBMP(makeOutputPath(name));
    }
    else if (command == "PLOT")
    {
        std::string plotMode;
        std::string fieldFile;
        std::string outputFile;
        ss >> plotMode >> fieldFile >> outputFile;
        if (!fieldFile.empty())
        {
            plotFieldData(fieldFile, outputFile);
        }
        else
        {
            Scenarios::plot3D(engine_.map());
        }
    }
    else if (command == "PLOT2D")
    {
        Scenarios::plot2D(engine_.map());
    }
    else if (command == "ANALIZ")
    {
        Scenarios::slopeAnalysis(engine_.map());
    }
    else if (command == "SLOPE_CHECK")
    {
        double threshold = slopeThreshold_;
        ss >> threshold;
        Scenarios::slopeCheck(engine_.map(), threshold);
    }
    else if (command == "COMPONENT_SEARCH")
    {
        int k = kmeansK_;
        int min_size = componentMinSize_;
        ss >> k >> min_size;
        Scenarios::componentSearch(engine_.map(), k, min_size);
    }
    else if (command == "EM_CLUSTER")
    {
        int k = emK_;
        ss >> k;
        Analysis::em_cluster(engine_.map(), k, componentMinSize_, 50);
        ClusterVisualizer::visualize(makeOutputPath("em_overlay.png"));
        Logger::info("EM overlay saved: output/em_overlay.png");
    }
    else if (command == "GEOMETRY")
    {
        int min_size = componentMinSize_;
        ss >> min_size;
        Scenarios::geometryScenario(engine_.map(), min_size);
    }
    else
    {
        throw std::runtime_error("Unknown command: " + command);
    }

    return "OK";
}
