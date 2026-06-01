
#include "CommandProcessor.h"
#include "TerrainEngine.h"
#include "Scenarios.h"
#include "ClusterVisualizer.h"
#include "Analysis.h"
#include "Logger.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

static std::string makeOutputPath(const std::string& filename)
{
    std::filesystem::path path = "output";
    path /= filename;
    return path.string();
}

CommandProcessor::CommandProcessor(
    TerrainEngine& engine
)
    : engine_(engine)
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

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "GAUSS")
        {
            GaussianBell g{};
            ss >> g.sign
               >> g.cx
               >> g.cy
               >> g.sx
               >> g.sy
               >> g.rho;
            engine_.addGaussian(g);
        }
        else if (command == "GENERATE")
        {
            engine_.generate();
        }
        else if (command == "BMP_WRITE")
        {
            std::string name;
            ss >> name;
            engine_.saveBMP(makeOutputPath(name));
        }
        else if (command == "PLOT")
        {
            engine_.render3DGnuplot();
        }
        else if (command == "PLOT2D")
        {
            engine_.render2DGnuplot();
        }
        else if (command == "ANALIZ")
        {
            Scenarios::slopeAnalysis(engine_.map());
        }
        else if (command == "SLOPE_CHECK")
        {
            double threshold = 2.0;
            ss >> threshold;
            Scenarios::slopeCheck(engine_.map(), threshold);
        }
        else if (command == "COMPONENT_SEARCH")
        {
            int k = 2;
            int min_size = 2;
            ss >> k >> min_size;
            Scenarios::componentSearch(engine_.map(), k, min_size);
        }
        else if (command == "EM_CLUSTER")
        {
            int k = 3;
            ss >> k;
            Analysis::em_cluster(engine_.map(), k, 2, 50);
            ClusterVisualizer::visualize(makeOutputPath("em_overlay.png"));
            Logger::info("EM overlay saved: output/em_overlay.png");
        }
        else if (command == "GEOMETRY")
        {
            int min_size = 8;
            ss >> min_size;
            Scenarios::geometryScenario(engine_.map(), min_size);
        }
    }
}
