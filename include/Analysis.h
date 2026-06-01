#pragma once
#include <vector>
#include "LandscapeMap.h"

namespace Analysis {
    std::vector<int> kmeans_cluster(const LandscapeMap& map, int k, int min_size=1);
    std::vector<int> em_cluster(const LandscapeMap& map, int k, int min_size=1, int maxIter=100);
}
