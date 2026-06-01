#include "Config.h"
#include <fstream>
#include <sstream>
#include <string>

void Config::load(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string key;
        if (!std::getline(ss, key, '='))
            continue;

        std::string value;
        if (!std::getline(ss, value))
            continue;

        auto trim = [](std::string& s) {
            const auto first = s.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
            {
                s.clear();
                return;
            }
            const auto last = s.find_last_not_of(" \t\r\n");
            s = s.substr(first, last - first + 1);
        };

        trim(key);
        trim(value);

        if (key == "WIDTH")
            width = std::stoi(value);
        else if (key == "HEIGHT")
            height = std::stoi(value);
        else if (key == "SLOPE_THRESHOLD")
            slopeThreshold = std::stod(value);
        else if (key == "NOISE_LEVEL")
            noiseLevel = std::stod(value);
        else if (key == "KMEANS_K")
            kmeansK = std::stoi(value);
        else if (key == "MIN_CLUSTER_SIZE")
            componentMinSize = std::stoi(value);
        else if (key == "EM_K")
            emK = std::stoi(value);
        else if (key == "MAX_K")
            maxK = std::stoi(value);
    }
}
