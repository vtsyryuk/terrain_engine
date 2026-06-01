#pragma once

#include <string>

class ILandscapeOperations
{
public:
    virtual ~ILandscapeOperations() = default;

    virtual void addGauss(int sign, double cx, double cy, double sx, double sy, double rho) = 0;
    virtual void generate() = 0;
    virtual void plot() = 0;
    virtual void plot2D() = 0;
    virtual void saveBMP(const std::string& filename = "") = 0;
    virtual void analiz() = 0;
    virtual void slopeCheck() = 0;
    virtual void componentSearch() = 0;
    virtual void geometry() = 0;
};
