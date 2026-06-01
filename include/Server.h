#pragma once

#include "Config.h"
#include "ILandscapeOperations.h"
#include "TerrainEngine.h"

#include <string>

class Server : public ILandscapeOperations
{
public:
    Server();
    explicit Server(const std::string& configFile);

    void init();
    std::string processLine(const std::string& line);

    void addGauss(int sign, double cx, double cy, double sx, double sy, double rho) override;
    void generate() override;
    void plot() override;
    void plot2D() override;
    void saveBMP(const std::string& filename = "") override;
    void analiz() override;
    void slopeCheck() override;
    void componentSearch() override;
    void geometry() override;

private:
    static Config loadConfig(const std::string& configFile);
    std::string executeCommand(const std::string& line);

    Config cfg_;
    TerrainEngine engine_;
};

void executeCommandsFromFile(Server& server, const std::string& commandFile);
