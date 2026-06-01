
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
#include <vector>

static std::string makeOutputPath(const std::string& filename)
{
    if (filename.rfind("output/", 0) == 0 || filename.rfind("output\\", 0) == 0)
    {
        return filename;
    }
    std::filesystem::path path = "output";
    path /= filename;
    return path.string();
}

static std::string readOutputArgument(std::istream& input, const std::string& defaultName)
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

static void writeClusterData(const TerrainEngine& engine)
{
    Scenarios::writeImageData(engine.map(), makeOutputPath("terrain_data_2d.txt"));
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

        if (executeLine(line) == "EXIT")
        {
            break;
        }
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
        if (g.sign == 0)
        {
            g.sign = -1;
        }
        if (g.rho <= -1.0 || g.rho >= 1.0)
        {
            Logger::warn("GAUSS rho out of range; using rho=0 for PDF-compatible command");
            g.rho = 0.0;
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
    else if (command == "EXIT")
    {
        return "EXIT";
    }
    else if (command == "SCAN")
    {
        engine_.saveRawTerrainData(makeOutputPath("field.dat"), 1);
        Logger::info("Raw field data saved: output/field.dat");
        engine_.generate();
    }
    else if (command == "BMP_WRITE")
    {
        std::string name = readOutputArgument(ss, "my_landscape.bmp");
        engine_.saveBMP(makeOutputPath(name));
    }
    else if (command == "GNUPLOT_FILE")
    {
        std::string name = readOutputArgument(ss, "field.dat");
        engine_.saveRawTerrainData(makeOutputPath(name), 1);
        Logger::info("Gnuplot field data saved: " + makeOutputPath(name));
    }
    else if (command == "PLOT")
    {
        std::string plotMode;
        std::string fieldFile;
        std::string outputFile;
        ss >> plotMode >> fieldFile >> outputFile;
        if (fieldFile == "to")
        {
            const std::string name = outputFile.empty() ? "field.dat" : outputFile;
            engine_.saveRawTerrainData(makeOutputPath(name), 1);
            Logger::info("Gnuplot field data saved: " + makeOutputPath(name));
        }
        else if (!fieldFile.empty())
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
    else if (command == "TRAJECTORIES")
    {
        std::string outputFile = readOutputArgument(ss, "trajectories.bmp");
        Scenarios::slopeCheck(engine_.map(), slopeThreshold_, makeOutputPath(outputFile));
        Logger::info("Trajectories map saved: " + makeOutputPath(outputFile));
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
    else if (command == "KMEANS")
    {
        int k = kmeansK_;
        ss >> k;
        const std::string outputFile = readOutputArgument(ss, "landscape_kmeans.bmp");
        writeClusterData(engine_);
        Analysis::kmeans_cluster(engine_.map(), k, componentMinSize_);
        ClusterVisualizer::visualizeLabels(makeOutputPath("kmeans.txt"), makeOutputPath(outputFile));
        Logger::info("K-means visualization saved: " + makeOutputPath(outputFile));
    }
    else if (command == "EM")
    {
        int k = emK_;
        ss >> k;
        const std::string outputFile = readOutputArgument(ss, "field_em.bmp");
        writeClusterData(engine_);
        Analysis::em_cluster(engine_.map(), k, componentMinSize_, 50);
        ClusterVisualizer::visualize(makeOutputPath(outputFile));
        Logger::info("EM visualization saved: " + makeOutputPath(outputFile));
    }
    else if (command == "GEOMETRY")
    {
        int min_size = componentMinSize_;
        ss >> min_size;
        const std::string outputFile = readOutputArgument(ss, "my_delaunay_voronoi.png");
        Scenarios::geometryScenario(engine_.map(), min_size, makeOutputPath(outputFile));
    }
    else if (command == "DELONE" || command == "DELAUNAY")
    {
        const std::string outputFile = readOutputArgument(ss, "my_delaunay_voronoi.bmp");
        Scenarios::geometryScenario(engine_.map(), componentMinSize_, makeOutputPath(outputFile));
    }
    else
    {
        throw std::runtime_error("Unknown command: " + command);
    }

    return "OK";
}
