#include "LandscapeMap.h"
#include "TerrainEngine.h"
#include <stdexcept>
#include <fstream>

LandscapeMap::LandscapeMap(int width, int height)
    : width_(width), height_(height), data_(height, std::vector<double>(width, 0.0))
{
    if (width_ <= 0 || height_ <= 0) throw std::runtime_error("Invalid map size");
}

double& LandscapeMap::at(int x, int y)
{
    return data_.at(y).at(x);
}

double LandscapeMap::at(int x, int y) const
{
    return data_.at(y).at(x);
}

int LandscapeMap::width() const { return width_; }
int LandscapeMap::height() const { return height_; }

void LandscapeMap::add(const GaussianBell& b)
{
    for (int y=0;y<height_;y++) for (int x=0;x<width_;x++) {
        double dx = x - b.cx; double dy = y - b.cy;
        double nx = dx / b.sx; double ny = dy / b.sy;
        double denom = 1 - b.rho*b.rho; if (fabs(denom)<1e-10) continue;
        double q = nx*nx + ny*ny - 2*b.rho*nx*ny;
        data_[y][x] += b.sign * exp(-0.5 * q / denom);
    }
}

void LandscapeMap::normalize()
{
    double mn = 1e9, mx = -1e9;
    for (const auto& row : data_)
        for (double v : row) { mn = std::min(mn, v); mx = std::max(mx, v); }
    double r = mx - mn; if (r < 1e-8) return;
    for (auto& row : data_)
        for (double& v : row) v = (v - mn) / r * 255.0;
}

std::vector<std::vector<double>>& LandscapeMap::data() { return data_; }
const std::vector<std::vector<double>>& LandscapeMap::data() const { return data_; }


void LandscapeMap::saveBMP(const std::string& filename) const
{
    std::ofstream f(filename, std::ios::binary);
    if (!f.is_open()) return;
    int pad = (4 - (width_ % 4)) % 4;
    uint32_t imgSz = static_cast<uint32_t>((width_ + pad) * height_);
    uint32_t fileSize = 54 + 256 * 4 + imgSz;
    uint8_t hdr[54] = {0x42,0x4D};
    memcpy(hdr+2,&fileSize,4);
    uint32_t offset = 54 + 256*4; memcpy(hdr+10,&offset,4);
    uint32_t headerSize = 40; memcpy(hdr+14,&headerSize,4);
    int32_t w = width_, h = height_; memcpy(hdr+18,&w,4); memcpy(hdr+22,&h,4);
    uint16_t planes=1, bpp=8; memcpy(hdr+26,&planes,2); memcpy(hdr+28,&bpp,2);
    uint32_t imgBytes = imgSz; memcpy(hdr+34,&imgBytes,4);
    f.write(reinterpret_cast<char*>(hdr),54);
    for (int i=0;i<256;i++){ uint8_t c=(uint8_t)i; f.write(reinterpret_cast<char*>(&c),1); f.write(reinterpret_cast<char*>(&c),1); f.write(reinterpret_cast<char*>(&c),1); f.put(0); }
    for (int y=height_-1;y>=0;y--){ for (int x=0;x<width_;x++){ uint8_t v = static_cast<uint8_t>(std::clamp(data_[y][x], 0.0, 255.0)); f.write(reinterpret_cast<char*>(&v),1);} for (int i=0;i<pad;i++) f.put(0);}    
}

std::pair<double,double> LandscapeMap::gradient(int x, int y) const
{
    if (x<=0||x>=width_-1||y<=0||y>=height_-1) return {0.0,0.0};
    double dx = (data_[y][x+1]-data_[y][x-1])/2.0;
    double dy = (data_[y+1][x]-data_[y-1][x])/2.0;
    return {dx,dy};
}
