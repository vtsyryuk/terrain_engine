#pragma once
#include "LandscapeMap.h"
#include "TerrainEngine.h"
#include <vector>

namespace Scenarios {
    void applyNoise(LandscapeMap& map, double percent);
    void smooth(LandscapeMap& map);
    void writeBMP(const LandscapeMap& map, const std::string& filename);
    void writeImageData(const LandscapeMap& map, const std::string& filename);

    std::vector<std::vector<std::pair<int,int>>> findConnectedComponents(const LandscapeMap& map, int min_size);

    void fieldGeneration(LandscapeMap& map, const std::vector<GaussianBell>& bells, double noise_percent);
    void plot3D(const LandscapeMap& map);
    void plot2D(const LandscapeMap& map);
    void slopeAnalysis(const LandscapeMap& map);
    void slopeCheck(const LandscapeMap& map, double threshold);
    void slopeCheck(const LandscapeMap& map, double threshold, const std::string& filename);
    void componentSearch(const LandscapeMap& map, int k, int min_size);
    void geometryScenario(const LandscapeMap& map, int min_size);
    void geometryScenario(const LandscapeMap& map, int min_size, const std::string& filename);
}
