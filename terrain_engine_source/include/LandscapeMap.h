#pragma once

#include <vector>
#include <string>
#include <utility>
#include "GaussianBell.h"

class LandscapeMap
{
public:
    LandscapeMap(
        int width,
        int height
    );

    double& at(
        int x,
        int y
    );

    double at(
        int x,
        int y
    ) const;

    int width() const;
    int height() const;

    void normalize();
    void add(const GaussianBell& b);
    void saveBMP(const std::string& fn) const;
    std::pair<double,double> gradient(int x, int y) const;

    std::vector<std::vector<double>>& data();
    const std::vector<std::vector<double>>& data() const;

private:
    int width_;
    int height_;

    std::vector<
        std::vector<double>
    > data_;
};