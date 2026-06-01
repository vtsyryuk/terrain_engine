#pragma once
#include <string>

namespace ClusterVisualizer {
    // Create a PNG overlay 'clusters.png' using terrain_data_2d.txt and em_clusters.txt
    void visualize(const std::string& outFilename = "clusters.png");
}
