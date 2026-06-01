#pragma once

#include "NamedPipeTransport.h"

#include <string>

class ServerInterface
{
public:
    explicit ServerInterface(std::string pipeName);

    void gauss(int s, double x, double y, double sx, double sy, double r);
    void generate();
    void plot();
    void plot2D();
    void saveBMP(const std::string& name = "my_landscape.bmp");
    void analiz();
    void slopeCheck();
    void componentSearch();
    void geometry();
    void executeFile(const std::string& filename);
    std::string shutdown();

private:
    std::string send(const std::string& command);

    PipeClient pipeClient_;
};
